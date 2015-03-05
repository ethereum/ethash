#include <Python.h>
#include <alloca.h>
#include <stdint.h>
#include "../libethash/ethash.h"

static PyObject*
get_params(PyObject* self, PyObject* args)
{
    unsigned long block_number;
    ethash_params p;

    if (!PyArg_ParseTuple(args, "k", &block_number))
      return 0;

    ethash_params_init(&p, block_number);

    // TODO: Do I memory leak?
    return Py_BuildValue("{sisi}",
     "DAG Size", p.full_size,
     "Cache Size", p.cache_size);
}

static PyObject*
get_cache(PyObject* self, PyObject* args)
{
    PyObject * py_params = 0;
    uint8_t * seed;
    int seed_len;

    if (!PyArg_ParseTuple(args, "Os#", &py_params, &seed, &seed_len))
      return 0;
    
    printf("Seed length: %i\n", seed_len);
    if (!PyDict_Check(py_params))
    {
      PyErr_SetString( PyExc_TypeError,
                       "Parameters must be a dictionary");
      return 0;
    }

    PyObject * dag_size_obj = PyDict_GetItemString(py_params, "DAG Size");
    if ( !PyInt_Check(dag_size_obj))
    {
      PyErr_SetString( PyExc_TypeError,
                       "The DAG size must be an integer");
      return 0;
    }

    PyObject * cache_size_obj = PyDict_GetItemString(py_params, "Cache Size");
    if ( !PyInt_Check(cache_size_obj))
    {
      PyErr_SetString( PyExc_TypeError,
                       "The cache size must be an integer");
      return 0;
    }

    const unsigned long cache_size = PyInt_AsUnsignedLongMask(cache_size_obj);
    const unsigned long dag_size = PyInt_AsUnsignedLongMask(dag_size_obj);
    printf("cache size: %lu\n", cache_size);
    printf("dag size: %lu\n", dag_size);
    ethash_params params;
    params.cache_size = cache_size;
    params.full_size = dag_size;
    ethash_cache cache;
    cache.mem = alloca(cache_size);
    Py_RETURN_NONE;
}


static PyMethodDef CoreMethods[] =
{
     {"get_params", get_params, METH_VARARGS, "Gets the parameters for a given block number"},
     {"get_cache", get_cache, METH_VARARGS, "Gets the cache for given parameters and seed hash"},
     {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initcore(void)
{
     (void) Py_InitModule("core", CoreMethods);
}
