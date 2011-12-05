#include <boost/python/detail/wrap_python.hpp>
#include <dlfcn.h>


PyObject * loadLibrary(PyObject *, PyObject * args)
{
  const char *filename;
  if (!PyArg_ParseTuple(args, "s", &filename))
  {
    PyErr_SetString(PyExc_ValueError, "Invalid string object");
    return NULL;
  }

  void* handle = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);
  if (!handle)
  {
    PyErr_SetString(PyExc_RuntimeError, dlerror());
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}


void init_module_libdlopen();

static PyMethodDef dlopen_methods[] = {
    {"loadlibrary",  loadLibrary, METH_VARARGS,
     "Load a library with dlopen and RTLD flags"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

extern "C"  void initlibdlopen()
{
  PyObject *m;

  m = Py_InitModule("libdlopen", dlopen_methods);
  if (m == NULL) return;
}

