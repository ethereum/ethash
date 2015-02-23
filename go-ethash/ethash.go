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
	"log"
	"math/big"
	"math/rand"
	"sync"
	"time"
	"unsafe"

	"github.com/ethereum/go-ethereum/core"
	"github.com/ethereum/go-ethereum/ethutil"
	"github.com/ethereum/go-ethereum/logger"
	"github.com/ethereum/go-ethereum/pow"
)

var powlogger = logger.NewLogger("POW")

type Ethash struct {
	turbo        bool
	HashRate     int64
	params       *C.ethash_params
	cache        *C.ethash_cache
	chainManager *core.ChainManager
	SeedBlockNum uint64
	dag          unsafe.Pointer // full GB of memory for dag
	nextdag      unsafe.Pointer
	hash         *C.uint8_t // return from ethash
	mutex        *sync.Mutex
}

func blockNonce(block pow.Block) (uint64, error) {
	nonce := block.N()
	nonceBuf := bytes.NewBuffer(nonce)
	nonceInt, err := binary.ReadUvarint(nonceBuf)
	if err != nil {
		return 0, err
	}
	return nonceInt, nil
}

const epochLength uint64 = 1000

func getSeedBlockNum(blockNum uint64) uint64 {
	var seedBlockNum uint64 = 0
	if blockNum >= 2*epochLength {
		seedBlockNum = ((blockNum / epochLength) - 1) * epochLength
	}
	return seedBlockNum
}

func getSeedHash(cm *core.ChainManager, blockNum uint64) []byte {
	return cm.GetBlockByNumber(getSeedBlockNum(blockNum)).Header().Hash()
}

func makeCache(seedHash []byte, params *C.ethash_params) *C.ethash_cache {
	var cacheMem unsafe.Pointer
	cacheMem = C.malloc(params.cache_size)
	cache := new(C.ethash_cache)
	C.ethash_cache_init(cache, cacheMem)
	C.ethash_mkcache(cache, params, (*C.uint8_t)((unsafe.Pointer)(&seedHash[0])))
	return cache
}

func makeDAG(cache *C.ethash_cache, params *C.ethash_params) unsafe.Pointer {
	var dag unsafe.Pointer
	dag = C.malloc(params.full_size)
	C.ethash_compute_full_data(dag, params, cache)
	return dag
}

func New(cm *core.ChainManager) *Ethash {
	params := new(C.ethash_params)
	seedBlockNum := getSeedBlockNum(cm.CurrentBlock().NumberU64())
	seedHash := getSeedHash(cm, seedBlockNum)
	C.ethash_params_init(params, C.uint32_t(seedBlockNum))
	log.Println("Params", params)

	cache := makeCache(seedHash, params)

	log.Println("Making Dag")
	start := time.Now()
	dag := makeDAG(cache, params)
	log.Println("Took:", time.Since(start))

	return &Ethash{
		turbo:        false,
		params:       params,
		cache:        cache,
		chainManager: cm,
		dag:          dag,
		hash:         (*C.uint8_t)(C.malloc(32)),
		SeedBlockNum: seedBlockNum,
		mutex:        new(sync.Mutex),
	}
}

func (pow *Ethash) Stop() {
	C.free(pow.cache.mem)
	C.free(unsafe.Pointer(pow.hash))
	C.free(pow.dag)
}

func (pow *Ethash) Update() {
	seedBlockNum := getSeedBlockNum(pow.chainManager.CurrentBlock().NumberU64())
	if pow.SeedBlockNum != seedBlockNum {
		pow.mutex.Lock()
		pow.Stop()
		seedHash := getSeedHash(pow.chainManager, seedBlockNum)
		params := new(C.ethash_params)
		C.ethash_params_init(params, C.uint32_t(seedBlockNum))
		log.Println("New Params", params)
		cache := makeCache(seedHash, params)
		dag := makeDAG(cache, params)
		pow.params = params
		pow.cache = cache
		pow.dag = dag
		pow.SeedBlockNum = seedBlockNum
		pow.mutex.Unlock()
	}
}

func (pow *Ethash) Search(block pow.Block, stop <-chan struct{}) []byte {
	pow.Update()
	// Not very elegant, multiple mining instances are not supported
	pow.mutex.Lock()

	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	miningHash := block.HashNoNonce()
	diff := block.Difficulty()
	log.Println("difficulty", diff)
	i := int64(0)
	start := time.Now().UnixNano()
	t := time.Now()

	nonce := uint64(r.Int63())

	for {
		select {
		case <-stop:
			powlogger.Infoln("Breaking from mining")
			pow.HashRate = 0
			pow.mutex.Unlock()
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
			log.Printf("seed hash, nonce: %x %x\n", miningHash, nonce)
			// pow.hash is the output/return of ethash_full
			C.ethash_full(pow.hash, pow.dag, pow.params, cMiningHash, cnonce)
			ghash := C.GoBytes(unsafe.Pointer(pow.hash), 32)
			log.Printf("ethhash full (on nonce): %x %x\n", ghash, nonce)

			if pow.verify(miningHash, diff, nonce) {
				pow.mutex.Unlock()
				return ghash
			}
			nonce += 1
		}

		if !pow.turbo {
			time.Sleep(20 * time.Microsecond)
		}
	}
}

func (pow *Ethash) Verify(block pow.Block) bool {
	nonceInt, err := blockNonce(block)
	if err != nil {
		log.Println("nonce to int err:", err)
		return false
	}
	return pow.verify(block.HashNoNonce(), block.Difficulty(), nonceInt)
}

func (pow *Ethash) verify(hash []byte, diff *big.Int, nonce uint64) bool {
	pow.mutex.Lock()
	chash := (*C.uint8_t)(unsafe.Pointer(&hash))
	cnonce := C.uint64_t(nonce)
	C.ethash_light(pow.hash, pow.cache, pow.params, chash, cnonce)
	verification := new(big.Int).Div(ethutil.BigPow(2, 256), diff)
	res := ethutil.U256(new(big.Int).SetUint64(nonce))
	ghash := C.GoBytes(unsafe.Pointer(pow.hash), 32)
	log.Println("ethash light (on nonce)", ghash, nonce)
	pow.mutex.Unlock()
	return res.Cmp(verification) <= 0
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
	C.ethash_full(pow.hash, pow.dag, pow.params, cMiningHash, cnonce)
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
