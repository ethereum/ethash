#pragma once

#define __CL_ENABLE_EXCEPTIONS 
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <time.h>
#include <libethash/ethash.h>

class ethash_miner
{
};

class ethash_cl_miner : public ethash_miner
{
public:
	ethash_cl_miner();

	bool init(ethash_params const& params, const uint8_t seed[32]);

	clock_t hashes(uint8_t* ret, uint8_t const* header, uint64_t nonce, unsigned count, unsigned HASH_SIZE = 0, bool time_inner = false);

private:
	static unsigned const c_num_buffers = 2;
	static unsigned const c_batch_size = 1024*1024;

	ethash_params m_params;
	cl::Context m_context;
	cl::CommandQueue m_queue;
	cl::Kernel m_kernel;
	cl::Buffer m_dag;
	cl::Buffer m_header;
	cl::Buffer m_hashes[c_num_buffers];
};