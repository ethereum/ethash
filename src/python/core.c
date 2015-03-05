#include <Python.h>
#include "../libethash/ethash.h"
 
static PyObject*
get_params(PyObject* self, PyObject* args)
{
    unsigned long block_number;
    ethash_params p;
 
    if (!PyArg_ParseTuple(args, "k", &block_number))
        Py_RETURN_NONE;

    ethash_params_init(&p, block_number);
 
    printf("\nBlock number: %lu\n", block_number);
    printf("DAG Size: %lu\n", p.full_size);
    printf("Cache Size: %lu\n", p.cache_size);
 
    return Py_BuildValue("{sisi}", 
	 "DAG Size", p.full_size, 
	 "Cache Size", p.cache_size);
}
 
static PyMethodDef CoreMethods[] =
{
     {"get_params", get_params, METH_VARARGS, "Gets the parameters for a given block"},
     {NULL, NULL, 0, NULL}
};
 
PyMODINIT_FUNC
initcore(void)
{
     (void) Py_InitModule("core", CoreMethods);
}
