package ethash

/*
#cgo CFLAGS: -std=gnu99 -Wno-error
#cgo darwin CFLAGS: -I/usr/local/include
#cgo darwin LDFLAGS: -L/usr/local/lib
#include "../libethash/ethash.h"
#include "../libethash/util.c"
#include "../libethash/internal.c"
#include "../libethash/blum_blum_shub.c"
#include "../libethash/sha3.c"
*/
import "C"

import (
	"bytes"
	"encoding/binary"
	"github.com/ethereum/go-ethereum/ethutil"
	"github.com/ethereum/go-ethereum/logger"
	"github.com/ethereum/go-ethereum/pow"
	"log"
	"math/big"
	"time"
	"unsafe"
)

var powlogger = logger.NewLogger("POW")

type Ethash struct {
	turbo    bool
	HashRate int64
	params   *C.ethash_params
	cache    *C.ethash_cache
	mem      unsafe.Pointer // full GB of memory for dag
	hash     *C.uint8_t     // return from ethash
}

func New(seedHash []byte) *Ethash {
	params := new(C.ethash_params)
	C.ethash_params_init(params)
	log.Println("Params", params)

	var mem unsafe.Pointer
	mem = C.malloc(C.size_t(params.full_size)) //+ 4095 + 64)

	cache := new(C.ethash_cache)
	C.ethash_cache_init(cache, mem)

	log.Println("making full data")
	start := time.Now()
	C.ethash_compute_full_data(mem, params, (*C.uint8_t)(unsafe.Pointer(&seedHash)))
	log.Println("took:", time.Since(start))

	var hash *C.uint8_t
	hash = (*C.uint8_t)(C.malloc(32))

	return &Ethash{
		turbo:  false,
		params: params,
		cache:  cache,
		mem:    mem,
		hash:   hash,
	}
}

// TODO free everything
func (pow *Ethash) Stop() {
}

func (pow *Ethash) Search(block pow.Block, stop <-chan struct{}) []byte {
	//r := rand.New(rand.NewSource(time.Now().UnixNano()))
	miningHash := block.HashNoNonce()
	diff := block.Difficulty()
	//diff = big.NewInt(10000)
	log.Println("difficulty", diff)
	i := int64(0)
	start := time.Now().UnixNano()
	t := time.Now()

	nonce := uint64(0) //uint64(r.Int63())

	for {
		select {
		case <-stop:
			powlogger.Infoln("Breaking from mining")
			pow.HashRate = 0
			return nil
		default:
			i++

			if time.Since(t) > (1 * time.Second) {
				elapsed := time.Now().UnixNano() - start
				hashes := ((float64(1e9) / float64(elapsed)) * float64(i)) / 1000
				pow.HashRate = int64(hashes)
				powlogger.Infoln("Hashing @", pow.HashRate, "khash")

				t = time.Now()
			}

			cMiningHash := (*C.uint8_t)(unsafe.Pointer(&miningHash))
			cnonce := C.uint64_t(nonce)
			log.Println("seed hash, nonce:", miningHash, nonce)
			// pow.hash is the output/return of ethash_full
			C.ethash_full(pow.hash, pow.cache.mem, pow.params, cMiningHash, cnonce)
			ghash := C.GoBytes(unsafe.Pointer(pow.hash), 32)
			log.Println("ethhash full (on nonce):", ghash, nonce)

			if pow.verify(miningHash, diff, nonce) {
				return ghash
			}
			nonce += 1
		}

		if !pow.turbo {
			time.Sleep(20 * time.Microsecond)
		}
	}

	return nil
}

func (pow *Ethash) Verify(block pow.Block) bool {
	nonce := block.N()
	nonceBuf := bytes.NewBuffer(nonce)
	nonceInt, err := binary.ReadUvarint(nonceBuf)
	if err != nil {
		log.Println("nonce to int err:", err)
		return false
	}
	return pow.verify(block.HashNoNonce(), block.Difficulty(), nonceInt)
}

func (pow *Ethash) verify(hash []byte, diff *big.Int, nonce uint64) bool {
	chash := (*C.uint8_t)(unsafe.Pointer(&hash))
	cnonce := C.uint64_t(nonce)
	C.ethash_light(pow.hash, pow.cache, pow.params, chash, cnonce)
	verification := new(big.Int).Div(ethutil.BigPow(2, 256), diff)
	res := ethutil.U256(new(big.Int).SetUint64(nonce))
	ghash := C.GoBytes(unsafe.Pointer(pow.hash), 32)
	log.Println("ethash light (on nonce)", ghash, nonce)
	if res.Cmp(verification) <= 0 {
		return true
	}
	return false
}

func (pow *Ethash) GetHashrate() int64 {
	return pow.HashRate
}

func (pow *Ethash) Turbo(on bool) {
	pow.turbo = on
}

func (pow *Ethash) full(nonce uint64, miningHash []byte) []byte {
	cMiningHash := (*C.uint8_t)(unsafe.Pointer(&miningHash))
	cnonce := C.uint64_t(nonce)
	log.Println("seed hash, nonce:", miningHash, nonce)
	// pow.hash is the output/return of ethash_full
	C.ethash_full(pow.hash, pow.cache.mem, pow.params, cMiningHash, cnonce)
	ghash_full := C.GoBytes(unsafe.Pointer(pow.hash), 32)
	return ghash_full
}

func (pow *Ethash) light(nonce uint64, miningHash []byte) []byte {
	cMiningHash := (*C.uint8_t)(unsafe.Pointer(&miningHash))
	cnonce := C.uint64_t(nonce)
	var hashR *C.uint8_t
	hashR = (*C.uint8_t)(C.malloc(32))
	C.ethash_light(hashR, pow.cache, pow.params, cMiningHash, cnonce)
	ghash_light := C.GoBytes(unsafe.Pointer(hashR), 32)
	return ghash_light
}
