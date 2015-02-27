package ethash

/*
#cgo CFLAGS: -std=gnu99 -Wall
#include "../libethash/ethash.h"
#include "../libethash/util.c"
#include "../libethash/internal.c"
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

type DAG struct {
	SeedBlockNum uint64
	dag          unsafe.Pointer // full GB of memory for dag
}

type ParamsAndCache struct {
	params       *C.ethash_params
	cache        *C.ethash_cache
	SeedBlockNum uint64
}

type Ethash struct {
	turbo          bool
	HashRate       int64
	chainManager   *core.ChainManager
	dag            *DAG
	paramsAndCache *ParamsAndCache
	nextdag        unsafe.Pointer
	ret            *C.ethash_return_value
	dagMutex       *sync.Mutex
	cacheMutex     *sync.Mutex
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

const epochLength uint64 = 30000

func getSeedBlockNum(blockNum uint64) uint64 {
	var seedBlockNum uint64 = 0
	if blockNum >= 2*epochLength {
		seedBlockNum = ((blockNum / epochLength) - 1) * epochLength
	}
	return seedBlockNum
}

func makeParamsAndCache(chainManager *core.ChainManager, seedBlockNum uint64) *ParamsAndCache {
	paramsAndCache := &ParamsAndCache{
		params:       new(C.ethash_params),
		cache:        new(C.ethash_cache),
		SeedBlockNum: seedBlockNum,
	}
	C.ethash_params_init(paramsAndCache.params, C.uint32_t(seedBlockNum))
	cacheMem := C.malloc(paramsAndCache.params.cache_size)
	seedHash := chainManager.GetBlockByNumber(getSeedBlockNum(seedBlockNum)).Header().Hash()
	C.ethash_mkcache(paramsAndCache.cache, paramsAndCache.params, (*C.uint8_t)((unsafe.Pointer)(&seedHash[0])))
	log.Println("Params", paramsAndCache.params)

	log.Println("Making Cache")
	start := time.Now()
	C.ethash_mkcache(paramsAndCache.cache, paramsAndCache.params, (*C.uint8_t)((unsafe.Pointer)(&seedHash[0])))
	log.Println("Took:", time.Since(start))
	return paramsAndCache
}

func (pow *Ethash) updateCache() {
	pow.cacheMutex.Lock()
	pow.paramsAndCache = makeParamsAndCache(pow.chainManager, pow.chainManager.CurrentBlock().NumberU64())
	pow.cacheMutex.Unlock()
}

func makeDAG(p *ParamsAndCache) *DAG {
	d := &DAG{
		dag:          C.malloc(p.params.full_size),
		SeedBlockNum: p.SeedBlockNum,
	}
	C.ethash_compute_full_data(d.dag, p.params, p.cache)
	return d
}

func (pow *Ethash) updateDAG() {
	pow.cacheMutex.Lock()
	pow.dagMutex.Lock()
	pow.dag = nil
	log.Println("Making Dag")
	start := time.Now()
	pow.dag = makeDAG(pow.paramsAndCache)
	log.Println("Took:", time.Since(start))
	pow.dagMutex.Unlock()
	pow.cacheMutex.Unlock()
}

func New(chainManager *core.ChainManager) *Ethash {
	return &Ethash{
		turbo:          false,
		paramsAndCache: makeParamsAndCache(chainManager, getSeedBlockNum(chainManager.CurrentBlock().NumberU64())),
		chainManager:   chainManager,
		dag:            nil,
		ret:            new(C.ethash_return_value),
		cacheMutex:     new(sync.Mutex),
		dagMutex:       new(sync.Mutex),
	}
}

func (pow *Ethash) DAGSize() uint64 {
	return uint64(pow.paramsAndCache.params.full_size)
}

func (pow *Ethash) CacheSize() uint64 {
	return uint64(pow.paramsAndCache.params.cache_size)
}

func (pow *Ethash) Stop() {
	pow.cacheMutex.Lock()
	pow.dagMutex.Lock()
	if pow.paramsAndCache.cache != nil {
		C.free(pow.paramsAndCache.cache.mem)
	}
	if pow.dag != nil {
		C.free(pow.dag.dag)
	}
	pow.dagMutex.Unlock()
	pow.cacheMutex.Unlock()
}

func (pow *Ethash) Search(block pow.Block, stop <-chan struct{}) []byte {
	// Not very elegant, multiple mining instances are not supported
	pow.dagMutex.Lock()

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
			pow.dagMutex.Unlock()
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
			C.ethash_full(pow.ret, pow.dag.dag, pow.paramsAndCache.params, cMiningHash, cnonce)
			ghash := C.GoBytes(unsafe.Pointer(&pow.ret.result[0]), 32)
			log.Printf("ethhash full (on nonce): %x %x\n", ghash, nonce)

			if pow.verify(miningHash, diff, nonce) {
				pow.dagMutex.Unlock()
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
	pow.dagMutex.Lock()
	chash := (*C.uint8_t)(unsafe.Pointer(&hash))
	cnonce := C.uint64_t(nonce)
	C.ethash_light(pow.ret, pow.paramsAndCache.cache, pow.paramsAndCache.params, chash, cnonce)
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
