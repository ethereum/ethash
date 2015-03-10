package ethash

/*
#cgo CFLAGS: -std=gnu99 -Wall
#include "src/libethash/util.c"
#include "src/libethash/internal.c"
#include "src/libethash/sha3.c"
*/
import "C"

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"log"
	"math/big"
	"math/rand"
	"os"
	"path"
	"sync"
	"time"
	"unsafe"

	"github.com/ethereum/go-ethereum/ethutil"
	"github.com/ethereum/go-ethereum/logger"
	"github.com/ethereum/go-ethereum/pow"
)

var tt256 = new(big.Int).Exp(big.NewInt(2), big.NewInt(256), big.NewInt(0))

var powlogger = logger.NewLogger("POW")

type ParamsAndCache struct {
	params       *C.ethash_params
	cache        *C.ethash_cache
	SeedBlockNum uint64
}

type DAG struct {
	dag            unsafe.Pointer // full GB of memory for dag
	file           bool
	paramsAndCache *ParamsAndCache
}

type Ethash struct {
	turbo          bool
	HashRate       int64
	chainManager   pow.ChainManager
	dag            *DAG
	paramsAndCache *ParamsAndCache
	ret            *C.ethash_return_value
	dagMutex       *sync.RWMutex
	cacheMutex     *sync.RWMutex
}

func parseNonce(nonce []byte) (uint64, error) {
	nonceBuf := bytes.NewBuffer(nonce)
	nonceInt, err := binary.ReadUvarint(nonceBuf)
	if err != nil {
		return 0, err
	}
	return nonceInt, nil
}

const epochLength uint64 = 30000

func GetSeedBlockNum(blockNum uint64) uint64 {
	var seedBlockNum uint64 = 0
	if blockNum > epochLength {
		seedBlockNum = ((blockNum - 1) / epochLength) * epochLength
	}
	return seedBlockNum
}

func makeParamsAndCache(chainManager pow.ChainManager, blockNum uint64) (*ParamsAndCache, error) {
	if blockNum >= epochLength*2048 {
		return nil, fmt.Errorf("block number is out of bounds (value %v, limit is %v)", blockNum, epochLength*2048)
	}
	seedBlockNum := GetSeedBlockNum(blockNum)
	paramsAndCache := &ParamsAndCache{
		params:       new(C.ethash_params),
		cache:        new(C.ethash_cache),
		SeedBlockNum: seedBlockNum,
	}
	C.ethash_params_init(paramsAndCache.params, C.uint32_t(seedBlockNum))
	paramsAndCache.cache.mem = C.malloc(paramsAndCache.params.cache_size)

	// XXX This is wrong
	seedHash := chainManager.GetBlockByNumber(seedBlockNum).SeedHash()

	log.Println("Making Cache")
	start := time.Now()
	C.ethash_mkcache(paramsAndCache.cache, paramsAndCache.params, (*C.uint8_t)(unsafe.Pointer(&seedHash[0])))
	log.Println("Took:", time.Since(start))

	return paramsAndCache, nil
}

func (pow *Ethash) updateCache() error {
	pow.cacheMutex.Lock()
	seedNum := GetSeedBlockNum(pow.chainManager.CurrentBlock().NumberU64())
	if pow.paramsAndCache.SeedBlockNum != seedNum {
		var err error
		pow.paramsAndCache, err = makeParamsAndCache(pow.chainManager, pow.chainManager.CurrentBlock().NumberU64())
		if err != nil {
			panic(err)
		}
	}
	pow.cacheMutex.Unlock()
	return nil
}

func makeDAG(p *ParamsAndCache) *DAG {
	d := &DAG{
		dag:            C.malloc(p.params.full_size),
		file:           false,
		paramsAndCache: p,
	}
	C.ethash_compute_full_data(d.dag, p.params, p.cache)
	return d
}

func (pow *Ethash) writeDagToDisk(dag *DAG, seedNum uint64) *os.File {
	data := C.GoBytes(unsafe.Pointer(dag.dag), C.int(pow.paramsAndCache.params.full_size))
	file, err := os.Create("/tmp/dag")
	if err != nil {
		panic(err)
	}

	num := make([]byte, 8)
	binary.BigEndian.PutUint64(num, seedNum)

	file.Write(num)
	file.Write(data)

	return file
}

func (pow *Ethash) UpdateDAG() {
	blockNum := pow.chainManager.CurrentBlock().NumberU64()
	if blockNum >= epochLength*2048 {
		panic(fmt.Errorf("Current block number is out of bounds (value %v, limit is %v)", blockNum, epochLength*2048))
	}

	pow.dagMutex.Lock()
	defer pow.dagMutex.Unlock()

	seedNum := GetSeedBlockNum(blockNum)
	if pow.dag == nil || pow.dag.paramsAndCache.SeedBlockNum != seedNum {
		if pow.dag != nil && pow.dag.dag != nil {
			C.free(pow.dag.dag)
			pow.dag.dag = nil
		}

		if pow.dag != nil && pow.dag.paramsAndCache.cache.mem != nil {
			C.free(pow.dag.paramsAndCache.cache.mem)
			pow.dag.paramsAndCache.cache.mem = nil
		}

		// Make the params and cache for the DAG
		paramsAndCache, err := makeParamsAndCache(pow.chainManager, blockNum)
		if err != nil {
			panic(err)
		}
		pow.paramsAndCache = paramsAndCache
		path := path.Join("/", "tmp", "dag")
		pow.dag = nil
		log.Println("Retrieving dag")
		start := time.Now()

		file, err := os.Open(path)
		if err != nil {
			log.Printf("No dag found. Generating new dag in '%s' (this takes a while)...", file.Name())
			pow.dag = makeDAG(paramsAndCache)
			file = pow.writeDagToDisk(pow.dag, seedNum)
			pow.dag.file = true
		} else {
			data, err := ioutil.ReadAll(file)
			if err != nil {
				panic(err)
			}

			num := binary.BigEndian.Uint64(data[0:8])
			if num < seedNum {
				log.Printf("DAG in '%s' is stale. Generating new dag (this takes a while)...", file.Name())
				pow.dag = makeDAG(paramsAndCache)
				file = pow.writeDagToDisk(pow.dag, seedNum)
				pow.dag.file = true
			} else {
				// XXX Check the DAG is not corrupted
				data = data[8:]
				pow.dag = &DAG{
					dag:            unsafe.Pointer(&data[0]),
					file:           true,
					paramsAndCache: paramsAndCache,
				}
			}
		}
		log.Println("Took:", time.Since(start))

		file.Close()
	}
}

func New(chainManager pow.ChainManager) *Ethash {
	paramsAndCache, err := makeParamsAndCache(chainManager, chainManager.CurrentBlock().NumberU64())
	if err != nil {
		panic(err)
	}

	return &Ethash{
		turbo:          true,
		paramsAndCache: paramsAndCache,
		chainManager:   chainManager,
		dag:            nil,
		cacheMutex:     new(sync.RWMutex),
		dagMutex:       new(sync.RWMutex),
	}
}

func (pow *Ethash) DAGSize() uint64 {
	return uint64(pow.dag.paramsAndCache.params.full_size)
}

func (pow *Ethash) CacheSize() uint64 {
	return uint64(pow.paramsAndCache.params.cache_size)
}

func (pow *Ethash) GetSeedHash(blockNum uint64) []byte {
	seednum := GetSeedBlockNum(blockNum)
	// XXX This is totally wrong
	return pow.chainManager.GetBlockByNumber(seednum).SeedHash()
}

func (pow *Ethash) Stop() {
	pow.cacheMutex.Lock()
	pow.dagMutex.Lock()
	defer pow.dagMutex.Unlock()
	defer pow.cacheMutex.Unlock()

	if pow.paramsAndCache.cache != nil {
		C.free(pow.paramsAndCache.cache.mem)
	}
	if pow.dag.dag != nil && !pow.dag.file {
		C.free(pow.dag.dag)
	}
	if pow.dag != nil && pow.dag.paramsAndCache != nil && pow.dag.paramsAndCache.cache.mem != nil {
		C.free(pow.dag.paramsAndCache.cache.mem)
		pow.dag.paramsAndCache.cache.mem = nil
	}
	pow.dag.dag = nil
}

func (pow *Ethash) Search(block pow.Block, stop <-chan struct{}) (uint64, []byte, []byte) {
	pow.UpdateDAG()

	pow.dagMutex.RLock()
	defer pow.dagMutex.RUnlock()

	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	miningHash := block.HashNoNonce()
	diff := block.Difficulty()

	i := int64(0)
	starti := i
	start := time.Now().UnixNano()

	nonce := uint64(r.Int63())
	cMiningHash := (*C.uint8_t)(unsafe.Pointer(&miningHash[0]))
	target := new(big.Int).Div(tt256, diff)

	var ret C.ethash_return_value
	for {
		select {
		case <-stop:
			powlogger.Infoln("Breaking from mining")
			pow.HashRate = 0
			return 0, nil, nil
		default:
			i++

			elapsed := time.Now().UnixNano() - start
			hashes := ((float64(1e9) / float64(elapsed)) * float64(i-starti)) / 1000
			pow.HashRate = int64(hashes)

			C.ethash_full(&ret, pow.dag.dag, pow.dag.paramsAndCache.params, cMiningHash, C.uint64_t(nonce))
			result := ethutil.Bytes2Big(C.GoBytes(unsafe.Pointer(&ret.result[0]), C.int(32)))

			if result.Cmp(target) <= 0 {
				mixDigest := C.GoBytes(unsafe.Pointer(&ret.mix_hash[0]), C.int(32))

				return nonce, mixDigest, pow.GetSeedHash(block.NumberU64())

			}

			nonce += 1
		}

		if !pow.turbo {
			time.Sleep(20 * time.Microsecond)
		}
	}

}

func (pow *Ethash) Verify(block pow.Block) bool {

	// Make sure the SeedHash is set correctly
	if bytes.Compare(block.SeedHash(), pow.GetSeedHash(block.NumberU64())) != 0 {
		return false
	}

	return pow.verify(block.HashNoNonce(), block.MixDigest(), block.Difficulty(), block.NumberU64(), block.Nonce())
}

func (pow *Ethash) verify(hash []byte, mixDigest []byte, difficulty *big.Int, blockNum uint64, nonce uint64) bool {
	// Make sure the block num is valid
	if blockNum >= epochLength*2048 {
		log.Println(fmt.Sprintf("Block number exceeds limit, invalid (value is %v, limit is %v)",
			blockNum, epochLength*2048))
		return false
	}

	// First check: make sure header, mixDigest, nonce are correct without hitting the cache
	// This is to prevent DOS attacks
	chash := (*C.uint8_t)(unsafe.Pointer(&hash[0]))
	cnonce := C.uint64_t(nonce)
	target := new(big.Int).Div(tt256, difficulty)

	var pAc *ParamsAndCache
	// If its an old block (doesn't use the current cache)
	// get the cache for it but don't update (so we don't need the mutex)
	// Otherwise, it's the current block or a future block.
	// If current, updateCache will do nothing.
	if GetSeedBlockNum(blockNum) < pow.paramsAndCache.SeedBlockNum {
		var err error
		// If we can't make the params for some reason, this block is invalid
		pAc, err = makeParamsAndCache(pow.chainManager, blockNum)
		if err != nil {
			log.Println(err)
			return false
		}
	} else {
		pow.updateCache()
		pow.cacheMutex.RLock()
		defer pow.cacheMutex.RUnlock()
		pAc = pow.paramsAndCache
	}

	ret := new(C.ethash_return_value)

	C.ethash_light(ret, pAc.cache, pAc.params, chash, cnonce)

	result := ethutil.Bytes2Big(C.GoBytes(unsafe.Pointer(&ret.result[0]), C.int(32)))
	return result.Cmp(target) <= 0
}

func (pow *Ethash) GetHashrate() int64 {
	return pow.HashRate
}

func (pow *Ethash) Turbo(on bool) {
	pow.turbo = on
}

func (pow *Ethash) FullHash(nonce uint64, miningHash []byte) []byte {
	pow.UpdateDAG()
	pow.dagMutex.Lock()
	defer pow.dagMutex.Unlock()
	cMiningHash := (*C.uint8_t)(unsafe.Pointer(&miningHash[0]))
	cnonce := C.uint64_t(nonce)
	ret := new(C.ethash_return_value)
	// pow.hash is the output/return of ethash_full
	C.ethash_full(ret, pow.dag.dag, pow.paramsAndCache.params, cMiningHash, cnonce)
	ghash_full := C.GoBytes(unsafe.Pointer(&ret.result), 32)
	return ghash_full
}

func (pow *Ethash) LightHash(nonce uint64, miningHash []byte) []byte {
	cMiningHash := (*C.uint8_t)(unsafe.Pointer(&miningHash[0]))
	cnonce := C.uint64_t(nonce)
	ret := new(C.ethash_return_value)
	C.ethash_light(ret, pow.paramsAndCache.cache, pow.paramsAndCache.params, cMiningHash, cnonce)
	ghash_light := C.GoBytes(unsafe.Pointer(&ret.result), 32)
	return ghash_light
}
