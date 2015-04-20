/*
###################################################################################
###################################################################################
####################                                           ####################
####################  EDIT AND YOU SHALL FEEL MY WRATH - jeff  ####################
####################                                           ####################
###################################################################################
###################################################################################
*/

package ethash

/*
#cgo CFLAGS: -std=gnu99 -Wall
#cgo LDFLAGS: -lm
#include "src/libethash/util.c"
#include "src/libethash/internal.c"
#include "src/libethash/sha3.c"
#include "src/libethash/io.c"
#ifdef _WIN32
#include "src/libethash/io_win32.c"
#include "src/libethash/mmap_win32.c"
#else
#include "src/libethash/io_posix.c"
#endif
*/
import "C"

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"math/big"
	"math/rand"
	"os"
	"path"
	"sync"
	"time"
	"unsafe"

	"github.com/ethereum/go-ethereum/common"
	"github.com/ethereum/go-ethereum/crypto"
	"github.com/ethereum/go-ethereum/logger"
	"github.com/ethereum/go-ethereum/logger/glog"
	"github.com/ethereum/go-ethereum/pow"
)

var minDifficulty = new(big.Int).Exp(big.NewInt(2), big.NewInt(256), big.NewInt(0))

type ParamsAndCache struct {
	params *C.ethash_params
	cache  *C.ethash_cache_t
	Epoch  uint64
}

type DAG struct {
	full           *C.ethash_full_t
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

func makeParamsAndCache(chainManager pow.ChainManager, blockNum uint64) (*ParamsAndCache, error) {
	if blockNum >= epochLength*2048 {
		return nil, fmt.Errorf("block number is out of bounds (value %v, limit is %v)", blockNum, epochLength*2048)
	}
	paramsAndCache := &ParamsAndCache{
		params: new(C.ethash_params),
		cache:  nil, // gets filled in below by C.ethash_cache_new
		Epoch:  blockNum / epochLength,
	}
	C.ethash_params_init(paramsAndCache.params, C.uint32_t(uint32(blockNum)))
	seedHash, err := GetSeedHash(blockNum)
	if err != nil {
		return nil, err
	}



	glog.V(logger.Info).Infof("Making cache for epoch: %d (%v) (%x)\n", paramsAndCache.Epoch, blockNum, seedHash)
	start := time.Now()
	paramsAndCache.cache = C.ethash_cache_new(
		paramsAndCache.params.cache_size,
		(*C.ethash_h256_t)(unsafe.Pointer(&seedHash[0])))
	if paramsAndCache.cache == nil {
		return nil, err;
	}
	if glog.V(logger.Info) {
		glog.Infoln("Took:", time.Since(start))
	}

	return paramsAndCache, nil
}

func (pow *Ethash) UpdateCache(blockNum uint64, force bool) error {
	pow.cacheMutex.Lock()
	defer pow.cacheMutex.Unlock()

	thisEpoch := blockNum / epochLength
	if force || pow.paramsAndCache.Epoch != thisEpoch {
		var err error
		if pow.paramsAndCache.cache != nil {
			ethash_cache_destroy(pow.paramsAndCache.cache)
			pow.paramsAndCache.cache = nil
		}
		pow.paramsAndCache, err = makeParamsAndCache(pow.chainManager, blockNum)
		if err != nil {
			panic(err)
		}
	}

	return nil
}

func makeDAG(p *ParamsAndCache, goString dirPath, blockNum uint64) *DAG {
	seedHash, err := GetSeedHash(blockNum)
	if err != nil {
		panic(err)
	}
	d := &DAG{
		full: C.ethash_full_new(
			C.Cstring(dirPath),
			(*C.ethash_h256_t)(unsafe.Pointer(&seedHash[0])),
			p.params.full_size,
			p.params.cache,
			nil),
	}

	// TODO: This should move to a callback which we should give as argument to ethash_full_new()
	// donech := make(chan string)
	// go func() {
	// 	t := time.NewTicker(5 * time.Second)
	// 	tstart := time.Now()
	// done:
	// 	for {
	// 		select {
	// 		case <-t.C:
	// 			glog.V(logger.Info).Infof("... still generating DAG (%v) ...\n", time.Since(tstart).Seconds())
	// 		case str := <-donech:
	// 			glog.V(logger.Info).Infof("... %s ...\n", str)
	// 			break done
	// 		}
	// 	}
	// }()
	// donech <- "DAG generation completed"
	return d
}

func destroyDAG(dag *DAG) {
	ethash_full_delete(dag.full)
	dag.full = nil
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

func GetSeedHash(blockNum uint64) ([]byte, error) {
	if blockNum >= epochLength*2048 {
		return nil, fmt.Errorf("block number is out of bounds (value %v, limit is %v)", blockNum, epochLength*2048)
	}

	epoch := blockNum / epochLength
	seedHash := make([]byte, 32)
	var i uint64
	for i = 0; i < 32; i++ {
		seedHash[i] = 0
	}
	for i = 0; i < epoch; i++ {
		seedHash = crypto.Sha3(seedHash)
	}
	return seedHash, nil
}

func (pow *Ethash) Stop() {
	pow.cacheMutex.Lock()
	pow.dagMutex.Lock()
	defer pow.dagMutex.Unlock()
	defer pow.cacheMutex.Unlock()

	if pow.dag.paramsAndCache != nil && pow.paramsAndCache.cache != nil {
		C.ethash_cache_delete(pow.paramsAndCache.cache)
		pow.paramsAndCache.cache = nil
	}
	if pow.dag.full != nil {
		destroyDAG(pow.dag)
	}
}

func (pow *Ethash) Search(block pow.Block, stop <-chan struct{}) (uint64, []byte, []byte) {
	pow.dag = makeDAG(pow.paramsAndCache, "/tmp", pow.chainManager.CurrentBlock().NumberU64())

	pow.dagMutex.RLock()
	defer pow.dagMutex.RUnlock()

	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	miningHash := block.HashNoNonce()
	diff := block.Difficulty()

	i := int64(0)
	starti := i
	start := time.Now().UnixNano()

	nonce := uint64(r.Int63())
	cMiningHash := (*C.ethash_h256_t)(unsafe.Pointer(&miningHash[0]))
	target := new(big.Int).Div(minDifficulty, diff)

	var ret C.ethash_return_value
	for {
		select {
		case <-stop:
			pow.HashRate = 0
			destroyDAG(pow.dag)
			return 0, nil, nil
		default:
			i++

			elapsed := time.Now().UnixNano() - start
			hashes := ((float64(1e9) / float64(elapsed)) * float64(i-starti)) / 1000
			pow.HashRate = int64(hashes)

			// C.ethash_full(&ret, pow.dag.dag, pow.dag.paramsAndCache.params, cMiningHash, C.uint64_t(nonce))
			C.ethash_full_compute(&ret, pow.dag.full, cMiningHash, C.uint64_t(nonce))
			result := common.Bytes2Big(C.GoBytes(unsafe.Pointer(&ret.result), C.int(32)))

			// TODO: disagrees with the spec https://github.com/ethereum/wiki/wiki/Ethash#mining
			if result.Cmp(target) <= 0 {
				mixDigest := C.GoBytes(unsafe.Pointer(&ret.mix_hash), C.int(32))
				seedHash, err := GetSeedHash(block.NumberU64()) // This seedhash is useless
				if err != nil {
					panic(err)
				}
				destroyDAG(pow.dag)
				return nonce, mixDigest, seedHash
			}

			nonce += 1
		}

		if !pow.turbo {
			time.Sleep(20 * time.Microsecond)
		}
	}
}

func (pow *Ethash) Verify(block pow.Block) bool {

	return pow.verify(block.HashNoNonce(), block.MixDigest(), block.Difficulty(), block.NumberU64(), block.Nonce())
}

func (pow *Ethash) verify(hash common.Hash, mixDigest common.Hash, difficulty *big.Int, blockNum uint64, nonce uint64) bool {
	// Make sure the block num is valid
	if blockNum >= epochLength*2048 {
		glog.V(logger.Info).Infoln(fmt.Sprintf("Block number exceeds limit, invalid (value is %v, limit is %v)",
			blockNum, epochLength*2048))
		return false
	}

	// First check: make sure header, mixDigest, nonce are correct without hitting the cache
	// This is to prevent DOS attacks
	chash := (*C.ethash_h256_t)(unsafe.Pointer(&hash[0]))
	cnonce := C.uint64_t(nonce)
	target := new(big.Int).Div(minDifficulty, difficulty)

	var pAc *ParamsAndCache
	// If its an old block (doesn't use the current cache)
	// get the cache for it but don't update (so we don't need the mutex)
	// Otherwise, it's the current block or a future block.
	// If current, updateCache will do nothing.
	if blockNum/epochLength < pow.paramsAndCache.Epoch {
		var err error
		// If we can't make the params for some reason, this block is invalid
		pAc, err = makeParamsAndCache(pow.chainManager, blockNum)
		if err != nil {
			glog.V(logger.Info).Infoln("big fucking eror", err)
			return false
		}
	} else {
		pow.UpdateCache(blockNum, false)
		pow.cacheMutex.RLock()
		defer pow.cacheMutex.RUnlock()
		pAc = pow.paramsAndCache
	}

	ret := new(C.ethash_return_value)

	C.ethash_light(ret, pAc.cache, pAc.params, chash, cnonce)

	result := common.Bytes2Big(C.GoBytes(unsafe.Pointer(&ret.result), C.int(32)))
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
	cMiningHash := (*C.ethash_h256_t)(unsafe.Pointer(&miningHash[0]))
	cnonce := C.uint64_t(nonce)
	ret := new(C.ethash_return_value)
	// pow.hash is the output/return of ethash_full
	C.ethash_full(ret, pow.dag.dag, pow.paramsAndCache.params, cMiningHash, cnonce)
	ghash_full := C.GoBytes(unsafe.Pointer(&ret.result), 32)
	return ghash_full
}

func (pow *Ethash) LightHash(nonce uint64, miningHash []byte) []byte {
	cMiningHash := (*C.ethash_h256_t)(unsafe.Pointer(&miningHash[0]))
	cnonce := C.uint64_t(nonce)
	ret := new(C.ethash_return_value)
	C.ethash_light(ret, pow.paramsAndCache.cache, pow.paramsAndCache.params, cMiningHash, cnonce)
	ghash_light := C.GoBytes(unsafe.Pointer(&ret.result), 32)
	return ghash_light
}
