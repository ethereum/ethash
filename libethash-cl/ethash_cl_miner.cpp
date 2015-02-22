#include <assert.h>
#include <queue>
#include "ethash_cl_miner.h"
#include "../libethash/util.h"
#include "../libethash/ethash.h"


#undef min
#undef max

#define HASH_SIZE 32
#define INIT_SIZE 64
#define GROUP_SIZE 64

static char const ethash_inner_code[] = R"(

#define DAG_SIZE	$DAG_SIZE
#define GROUP_SIZE	$GROUP_SIZE
#define INIT_SIZE	$INIT_SIZE
#define HASH_SIZE	$HASH_SIZE
#define MIX_SIZE	$MIX_SIZE
#define FNV_PRIME	0x01000193

__constant ulong const Keccak_f1600_RC[24] = {
	0x0000000000000001,
	0x0000000000008082,
	0x800000000000808a,
	0x8000000080008000,
	0x000000000000808b,
	0x0000000080000001,
	0x8000000080008081,
	0x8000000000008009,
	0x000000000000008a,
	0x0000000000000088,
	0x0000000080008009,
	0x000000008000000a,
	0x000000008000808b,
	0x800000000000008b,
	0x8000000000008089,
	0x8000000000008003,
	0x8000000000008002,
	0x8000000000000080,
	0x000000000000800a,
	0x800000008000000a,
	0x8000000080008081,
	0x8000000000008080,
	0x0000000080000001,
	0x8000000080008008,
};

void keccak_f1600_round(ulong* a, uint r)
{
	ulong b[25];
	ulong t;

	// Theta
	b[0] = a[0] ^ a[5] ^ a[10] ^ a[15] ^ a[20];
	b[1] = a[1] ^ a[6] ^ a[11] ^ a[16] ^ a[21];
	b[2] = a[2] ^ a[7] ^ a[12] ^ a[17] ^ a[22];
	b[3] = a[3] ^ a[8] ^ a[13] ^ a[18] ^ a[23];
	b[4] = a[4] ^ a[9] ^ a[14] ^ a[19] ^ a[24];
	t = b[4] ^ rotate(b[1], 1ul);
	a[0] ^= t;
	a[5] ^= t;
	a[10] ^= t;
	a[15] ^= t;
	a[20] ^= t;
	t = b[0] ^ rotate(b[2], 1ul);
	a[1] ^= t;
	a[6] ^= t;
	a[11] ^= t;
	a[16] ^= t;
	a[21] ^= t;
	t = b[1] ^ rotate(b[3], 1ul);
	a[2] ^= t;
	a[7] ^= t;
	a[12] ^= t;
	a[17] ^= t;
	a[22] ^= t;
	t = b[2] ^ rotate(b[4], 1ul);
	a[3] ^= t;
	a[8] ^= t;
	a[13] ^= t;
	a[18] ^= t;
	a[23] ^= t;
	t = b[3] ^ rotate(b[0], 1ul);
	a[4] ^= t;
	a[9] ^= t;
	a[14] ^= t;
	a[19] ^= t;
	a[24] ^= t;

	// Rho Pi
	b[0] = a[0];
	b[10] = rotate(a[1], 1ul);
	b[7] = rotate(a[10], 3ul);
	b[11] = rotate(a[7], 6ul);
	b[17] = rotate(a[11], 10ul);
	b[18] = rotate(a[17], 15ul);
	b[3] = rotate(a[18], 21ul);
	b[5] = rotate(a[3], 28ul);
	b[16] = rotate(a[5], 36ul);
	b[8] = rotate(a[16], 45ul);
	b[21] = rotate(a[8], 55ul);
	b[24] = rotate(a[21], 2ul);
	b[4] = rotate(a[24], 14ul);
	b[15] = rotate(a[4], 27ul);
	b[23] = rotate(a[15], 41ul);
	b[19] = rotate(a[23], 56ul);
	b[13] = rotate(a[19], 8ul);
	b[12] = rotate(a[13], 25ul);
	b[2] = rotate(a[12], 43ul);
	b[20] = rotate(a[2], 62ul);
	b[14] = rotate(a[20], 18ul);
	b[22] = rotate(a[14], 39ul);
	b[9] = rotate(a[22], 61ul);
	b[6] = rotate(a[9], 20ul);
	b[1] = rotate(a[6], 44ul);

	// Chi
	a[0] = b[0] ^ ~b[1] & b[2];
	a[1] = b[1] ^ ~b[2] & b[3];
	a[2] = b[2] ^ ~b[3] & b[4];
	a[3] = b[3] ^ ~b[4] & b[0];
	a[4] = b[4] ^ ~b[0] & b[1];
	a[5] = b[5] ^ ~b[6] & b[7];
	a[6] = b[6] ^ ~b[7] & b[8];
	a[7] = b[7] ^ ~b[8] & b[9];
	a[8] = b[8] ^ ~b[9] & b[5];
	a[9] = b[9] ^ ~b[5] & b[6];
	a[10] = b[10] ^ ~b[11] & b[12];
	a[11] = b[11] ^ ~b[12] & b[13];
	a[12] = b[12] ^ ~b[13] & b[14];
	a[13] = b[13] ^ ~b[14] & b[10];
	a[14] = b[14] ^ ~b[10] & b[11];
	a[15] = b[15] ^ ~b[16] & b[17];
	a[16] = b[16] ^ ~b[17] & b[18];
	a[17] = b[17] ^ ~b[18] & b[19];
	a[18] = b[18] ^ ~b[19] & b[15];
	a[19] = b[19] ^ ~b[15] & b[16];
	a[20] = b[20] ^ ~b[21] & b[22];
	a[21] = b[21] ^ ~b[22] & b[23];
	a[22] = b[22] ^ ~b[23] & b[24];
	a[23] = b[23] ^ ~b[24] & b[20];
	a[24] = b[24] ^ ~b[20] & b[21];

	// Iota
	a[0] ^= Keccak_f1600_RC[r];
}

void keccak_f1600(ulong* a, uint rounds)
{
	keccak_f1600_round(a, 0);
	for (uint r = 1; r != (rounds-1); ++r)
	{
		keccak_f1600_round(a, r);
	}
	keccak_f1600_round(a, rounds-1);
}

void keccak_f1600_terminate(ulong* a, uint in_size, uint out_size)
{
	for (uint i = in_size; i != 25; ++i)
	{
		a[i] = 0;
	}
#if __ENDIAN_LITTLE__
	a[in_size] ^= 0x0000000000000001;
	a[24-out_size*2] ^= 0x8000000000000000;
#else
	a[in_size] ^= 0x0100000000000000;
	a[24-out_size*2] ^= 0x0000000000000080;
#endif
}

__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1)))
__kernel  void ethash(
	__global ulong* g_hashes,
	__constant ulong const* g_header,
	__global uint const* g_dag,
	ulong start_nonce,
	uint accesses,
	uint keccak_rounds
	)
{
	uint const global_id = get_global_id(0);
	uint const local_id = get_local_id(0);

	// maybe spill init to LDS to lower register pressure?
	// not obviously better in this version
	//__local
	ulong l_init[INIT_SIZE/8 * GROUP_SIZE];

	// alias mix with keccak state
	union
	{
		ulong u64[25];
		uint u32[MIX_SIZE/4];
	} mix;
	
	// sha3_512(header .. nonce)
	for (uint i = 0; i != HASH_SIZE/8; ++i)
	{
		mix.u64[i] = g_header[i];
	}
	ulong nonce = start_nonce + global_id;
	mix.u64[4] = nonce;
	keccak_f1600_terminate(mix.u64, 5, INIT_SIZE/8);
	keccak_f1600(mix.u64, keccak_rounds);

	for (uint i = 0; i != INIT_SIZE/8; ++i)
	{
		l_init[i*GROUP_SIZE + local_id] = mix.u64[i];
	}
	for (uint i = 0; i != (MIX_SIZE-INIT_SIZE)/8; ++i)
	{
		mix.u64[i+INIT_SIZE/8] = mix.u64[i % (INIT_SIZE/8)];
	}

	uint mix_val = mix.u32[0];
	uint init0 = mix.u32[0];
	for (uint a = 0; a != accesses; ++a)
	{
		uint pi = ((init0 ^ a)*FNV_PRIME ^ mix_val) % (DAG_SIZE / MIX_SIZE);
		uint n = (a+1) % (MIX_SIZE / 4);

		#pragma unroll
		for (uint i = 0; i != (MIX_SIZE / 4); ++i)
		{
			mix.u32[i] = mix.u32[i]*FNV_PRIME ^ g_dag[pi*(MIX_SIZE/4) + i];
			mix_val = i == n ? mix.u32[i] : mix_val;
		}
	}

	// reduce mix
	for (uint i = 0; i != MIX_SIZE/4; i += 4)
	{
		uint reduction = mix.u32[i+0];
		reduction = reduction*FNV_PRIME ^ mix.u32[i+1];
		reduction = reduction*FNV_PRIME ^ mix.u32[i+2];
		reduction = reduction*FNV_PRIME ^ mix.u32[i+3];
		mix.u32[i/4] = reduction;
	}

	// keccak_256(keccak_512(header..nonce)..keccak_256(mix));
	for (uint i = 0; i != HASH_SIZE/8; ++i)
	{
		mix.u64[INIT_SIZE/8 + i] = mix.u64[i];
	}
	for (uint i = 0; i != INIT_SIZE/8; ++i)
	{
		mix.u64[i] = l_init[i*GROUP_SIZE + local_id];
	}
	keccak_f1600_terminate(mix.u64, (INIT_SIZE+HASH_SIZE)/8, HASH_SIZE/8);
	keccak_f1600(mix.u64, keccak_rounds);

	// store to output buffer
	for (uint i = 0; i != HASH_SIZE/8; ++i)
	{
		g_hashes[global_id*(HASH_SIZE/8) + i] = mix.u64[i];
	}
}
)";

static void replace(std::string& subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

#pragma warning(disable: 4996)
static void replace(std::string& subject, const std::string& search, unsigned num)
{
	char buf[64];
	sprintf(buf, "%u", num);
	replace(subject, search, buf);
}

ethash_cl_miner::ethash_cl_miner()
{
}

bool ethash_cl_miner::init(ethash_params const& params, const uint8_t seed[32])
{
	// store params
	m_params = params;

	// get all platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
	if (platforms.empty())
	{
		debugf("No OpenCL platforms found.\n");
		return false;
	}

	// use default platform
	debugf("Using platform: %s\n", platforms[0].getInfo<CL_PLATFORM_NAME>().c_str());

    // get GPU device of the default platform
    std::vector<cl::Device> devices;
    platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if (devices.empty())
	{
		debugf("No OpenCL devices found.\n");
		return false;
	}

	// use default device
	cl::Device& device = devices[0];
	debugf("Using device: %s\n", device.getInfo<CL_DEVICE_NAME>().c_str());

	// create context
	m_context = cl::Context({device});
	m_queue = cl::CommandQueue(m_context, device);

	// patch source code
	std::string code = ethash_inner_code;
	replace(code, "$GROUP_SIZE", GROUP_SIZE);
	replace(code, "$INIT_SIZE", INIT_SIZE);
	replace(code, "$MIX_SIZE", MIX_BYTES);
	replace(code, "$HASH_SIZE", 32);
	replace(code, "$DAG_SIZE", (unsigned)params.full_size);
	//debugf("%s", code.c_str());

	// create miner OpenCL program
	cl::Program::Sources sources;
	sources.push_back({code.c_str(), code.size()});

	cl::Program program(m_context, sources);
	try
	{
		program.build({device});
	}
	catch (cl::Error err)
	{
		debugf("%s\n", program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str());
		return false;
	}
	m_kernel = cl::Kernel(program, "ethash");

	// create buffer for dag
	m_dag = cl::Buffer(m_context, CL_MEM_READ_ONLY, params.full_size);
	
	// create buffer for header
	m_header = cl::Buffer(m_context, CL_MEM_READ_ONLY, 32);

	// compute dag on CPU
	{
		void* cache_mem = malloc(params.cache_size + 63);
		ethash_cache cache;
		ethash_cache_init(&cache, (void*)(((uintptr_t)cache_mem + 63) & ~63));
		ethash_mkcache(&cache, &params, seed);

		// if this throws then it's because we probably need to subdivide the dag uploads for compatibility
		void* dag_ptr = m_queue.enqueueMapBuffer(m_dag, true, CL_MAP_WRITE_INVALIDATE_REGION, 0, params.full_size);
		ethash_compute_full_data(dag_ptr, &params, &cache);
		m_queue.enqueueUnmapMemObject(m_dag, dag_ptr);

		free(cache_mem);
	}

	// create mining buffers
	for (unsigned i = 0; i != c_num_buffers; ++i)
	{
		m_hashes[i] = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, 32*c_batch_size);
	}
	return true;
}

clock_t ethash_cl_miner::hashes(uint8_t* ret, uint8_t const* header, uint64_t nonce, unsigned count, unsigned hash_read_size, bool time_inner)
{
	clock_t time = 0;

	struct pending_batch
	{
		unsigned base;
		unsigned count;
		unsigned buf;
	};
	std::queue<pending_batch> pending;
	
	// update header constant buffer
	m_queue.enqueueWriteBuffer(m_header, true, 0, 32, header);

	/*
	__kernel  void ethash(
		__global ulong* g_hashes,
		__constant ulong const* g_header,
		__global uint const* g_dag,
		ulong start_nonce,
		uint accesses,
		uint keccak_rounds
		);
	*/
	m_kernel.setArg(1, m_header);
	m_kernel.setArg(2, m_dag);
	m_kernel.setArg(3, nonce);
	m_kernel.setArg(4, (hash_read_size ? hash_read_size : m_params.hash_read_size) / MIX_BYTES);
	m_kernel.setArg(5, 24); // have to pass this to stop the compile unrolling the loop

	unsigned buf = 0;
	for (unsigned i = 0; i < count || !pending.empty(); )
	{
		// how many this batch
		if (i < count)
		{
			unsigned const c_min_batch_size = 1; // smallest batch
			unsigned const this_count = std::min(count - i, c_batch_size);
			unsigned const batch_count = std::max(this_count, c_min_batch_size);

			// supply output hash buffer to kernel
			m_kernel.setArg(0, m_hashes[buf]);

			if (time_inner)
				m_queue.finish();

			// execute it!
			clock_t start_time = clock();
			m_queue.enqueueNDRangeKernel(
				m_kernel,
				cl::NullRange,
				cl::NDRange(batch_count),
				cl::NDRange(GROUP_SIZE)
				);
			m_queue.flush();
			if (time_inner)
				m_queue.finish();
			time += clock() - start_time;
		
			pending.push({i, this_count, buf});
			i += this_count;
			buf = (buf + 1) % c_num_buffers;
		}

		// read results
		if (i == count || pending.size() == c_num_buffers)
		{
			pending_batch const& batch = pending.front();

			// could use pinned host pointer instead, but this path isn't that important.
			uint8_t* hashes = (uint8_t*)m_queue.enqueueMapBuffer(m_hashes[batch.buf], true, CL_MAP_READ, 0, batch.count * HASH_SIZE);
			memcpy(ret + batch.base*HASH_SIZE, hashes, batch.count*HASH_SIZE);
			m_queue.enqueueUnmapMemObject(m_hashes[batch.buf], hashes);

			pending.pop();
		}
	}

	return time;
}
