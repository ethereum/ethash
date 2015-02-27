package ethash

import (
	"bytes"
	"crypto/rand"
	"log"
	"math/big"
	"os"
	"path"
	"testing"

	"github.com/ethereum/go-ethereum/core"
	"github.com/ethereum/go-ethereum/core/types"
	"github.com/ethereum/go-ethereum/ethdb"
	"github.com/ethereum/go-ethereum/event"
	"github.com/ethereum/go-ethereum/rlp"
)

func loadChain(fn string, t *testing.T) (types.Blocks, error) {
	fh, err := os.OpenFile(path.Join(os.Getenv("GOPATH"), "src", "github.com", "ethereum", "c-ethash", "go-ethash", "_testdat", fn), os.O_RDONLY, os.ModePerm)
	if err != nil {
		return nil, err
	}
	defer fh.Close()

	var chain types.Blocks
	if err := rlp.Decode(fh, &chain); err != nil {
		return nil, err
	}

	blocks := ([]*types.Block)(chain)
	chain = types.Blocks(blocks[1:])

	return chain, nil
}

func insertChain(chainMan *core.ChainManager, chain types.Blocks, t *testing.T) {
	err := chainMan.InsertChain(chain)
	if err != nil {
		panic(err)
	}
}

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

	var eventMux event.TypeMux
	chainMan := core.NewChainManager(db, &eventMux)
	txPool := core.NewTxPool(&eventMux)
	blockMan := core.NewBlockProcessor(db, txPool, chainMan, &eventMux)
	chainMan.SetProcessor(blockMan)
	chain1, err := loadChain("chain1", t)
	if err != nil {
		panic(err)
	}
	insertChain(chainMan, chain1, t)
	log.Println("Block Number: ", chainMan.CurrentBlock().Number())

	e := New(chainMan, true)

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
