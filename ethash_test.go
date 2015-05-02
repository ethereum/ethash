package ethash

import (
	"bytes"
	"crypto/rand"
	"encoding/hex"
	"log"
	"math/big"
	"os"
	"sync"
	"testing"

	"github.com/ethereum/go-ethereum/common"
)

func init() {
	// glog.SetV(6)
	// glog.SetToStderr(true)
}

type testBlock struct {
	difficulty  *big.Int
	hashNoNonce common.Hash
	nonce       uint64
	mixDigest   common.Hash
	number      uint64
}

func (b *testBlock) Difficulty() *big.Int     { return b.difficulty }
func (b *testBlock) HashNoNonce() common.Hash { return b.hashNoNonce }
func (b *testBlock) Nonce() uint64            { return b.nonce }
func (b *testBlock) MixDigest() common.Hash   { return b.mixDigest }
func (b *testBlock) NumberU64() uint64        { return b.number }

func TestEthashConcurrentVerify(t *testing.T) {
	eth, err := NewForTesting()
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(eth.Full.Dir)
	defer eth.Stop()

	block := &testBlock{difficulty: big.NewInt(10)}
	nonce, _ := eth.Search(block, nil)
	block.nonce = nonce

	// Verify the block concurrently to check for data races.
	var wg sync.WaitGroup
	wg.Add(100)
	for i := 0; i < 100; i++ {
		go func() {
			if !eth.Verify(block) {
				t.Error("Block could not be verified")
			}
			wg.Done()
		}()
	}
	wg.Wait()
}

func TestEthashConcurrentSearch(t *testing.T) {
	eth, err := NewForTesting()
	if err != nil {
		t.Fatal(err)
	}
	eth.Turbo(true)
	defer os.RemoveAll(eth.Full.Dir)
	defer eth.Stop()

	// launch n searches concurrently.
	var (
		block   = &testBlock{difficulty: big.NewInt(35000)}
		nsearch = 10
		wg      = new(sync.WaitGroup)
		found   = make(chan uint64)
		stop    = make(chan struct{})
	)
	rand.Read(block.hashNoNonce[:])
	wg.Add(nsearch)
	for i := 0; i < nsearch; i++ {
		go func() {
			nonce, _ := eth.Search(block, stop)
			select {
			case found <- nonce:
			case <-stop:
			}
			wg.Done()
		}()
	}

	// wait for one of them to find the nonce
	nonce := <-found
	// stop the others
	close(stop)
	wg.Wait()

	if block.nonce = nonce; !eth.Verify(block) {
		t.Error("Block could not be verified")
	}
}

func TestEthashSearchAcrossEpoch(t *testing.T) {
	eth, err := NewForTesting()
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(eth.Full.Dir)
	defer eth.Stop()

	for i := epochLength - 40; i < epochLength+40; i++ {
		block := &testBlock{number: i, difficulty: big.NewInt(90)}
		rand.Read(block.hashNoNonce[:])
		nonce, _ := eth.Search(block, nil)
		block.nonce = nonce
		if !eth.Verify(block) {
			t.Fatalf("Block could not be verified")
		}
	}
}

func TestGetSeedHash(t *testing.T) {
	seed0, err := GetSeedHash(0)
	if err != nil {
		t.Errorf("Failed to get seedHash for block 0: %v", err)
	}
	if bytes.Compare(seed0, make([]byte, 32)) != 0 {
		log.Printf("seedHash for block 0 should be 0s, was: %v\n", seed0)
	}
	seed1, err := GetSeedHash(30000)
	if err != nil {
		t.Error(err)
	}

	// From python:
	// > from pyethash import get_seedhash
	// > get_seedhash(30000)
	expectedSeed1, err := hex.DecodeString("290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563")
	if err != nil {
		t.Error(err)
	}

	if bytes.Compare(seed1, expectedSeed1) != 0 {
		log.Printf("seedHash for block 1 should be: %v,\nactual value: %v\n", expectedSeed1, seed1)
	}

}
