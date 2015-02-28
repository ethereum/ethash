package ethash

import (
	"bytes"
	"crypto/rand"
	"log"
	"math/big"
	"testing"

	"github.com/ethereum/go-ethereum/core"
	"github.com/ethereum/go-ethereum/ethdb"
)

func TestEthash(t *testing.T) {
	seedHash := make([]byte, 32)
	_, err := rand.Read(seedHash)
	if err != nil {
		panic(err)
	}

	db, err := ethdb.NewMemDatabase()
	if err != nil {
		panic(err)
	}

	blockProcessor, err := core.NewCanonical(5, db)
	if err != nil {
		panic(err)
	}

	log.Println("Block Number: ", blockProcessor.ChainManager().CurrentBlock().Number())

	e := New(blockProcessor.ChainManager(), true)

	miningHash := make([]byte, 32)
	if _, err := rand.Read(miningHash); err != nil {
		panic(err)
	}
	diff := big.NewInt(10000)
	log.Println("difficulty", diff)

	nonce := uint64(0)

	ghash_full := e.full(nonce, miningHash)
	log.Printf("ethhash full (on nonce): %x %x\n", ghash_full, nonce)

	ghash_light := e.light(nonce, miningHash)
	log.Printf("ethash light (on nonce): %x %x\n", ghash_light, nonce)

	if bytes.Compare(ghash_full, ghash_light) != 0 {
		t.Errorf("full: %x, light: %x", ghash_full, ghash_light)
	}
}
