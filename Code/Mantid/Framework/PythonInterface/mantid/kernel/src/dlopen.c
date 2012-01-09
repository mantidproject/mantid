#include <Python.h>
#include <dlfcn.h>


PyObject * loadLibrary(PyObject *self, PyObject * args)
{
  (void)self;
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

void init_dlopen()
{
  PyObject *m;

  m = Py_InitModule("_dlopen", dlopen_methods);
  if (m == NULL) return;
}

