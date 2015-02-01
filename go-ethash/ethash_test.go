package ethash

import (
	"bytes"
	"crypto/rand"
	"fmt"
	"log"
	"math/big"
	"testing"
)

func TestEthash(t *testing.T) {
	seedHash := make([]byte, 32)
	_, err := rand.Read(seedHash)
	if err != nil {
		panic(err)
	}

	e := New(seedHash)

	miningHash := make([]byte, 32)
	if _, err := rand.Read(miningHash); err != nil {
		panic(err)
	}
	diff := big.NewInt(10000)
	log.Println("difficulty", diff)

	nonce := uint64(0)

	ghash_full := e.full(nonce, miningHash)
	log.Println("ethhash full (on nonce):", ghash_full, nonce)

	ghash_light := e.light(nonce, miningHash)
	log.Println("ethash light (on nonce)", ghash_light, nonce)

	if bytes.Compare(ghash_full, ghash_light) != 0 {
		t.Fatal(fmt.Sprintf("full: %x, light: %x", ghash_full, ghash_light))
	}
}
