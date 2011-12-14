#include "MantidPythonInterface/kernel/NumpyConverters.h"

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/ndarrayobject.h>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Numpy
    {

      /**
       *  Create a numpy array wrapper around existing data. This is only possible for contiguous data
       *  @param data :: A const reference to an existing contiguous array
       *  @return A numpy wrapper around the existing data, no copy is performed. It is left in a read-write state.
       */
      PyObject *wrapWithNumpy(const std::vector<double> & data)
      {
        npy_intp dims[1] = { data.size() };
        return PyArray_SimpleNewFromData(1, dims, NPY_DOUBLE,(void*)&(data[0]));
      }

      /**
       *  Create a read-only numpy array wrapper around existing data. This is only possible for contiguous data
       *  @param data :: A const reference to an existing contiguous array
       *  @return A numpy wrapper around the existing data, no copy is performed. It is marked read-only.
       */
      PyObject *wrapWithReadOnlyNumpy(const std::vector<double> & data)
      {
        PyArrayObject *nparray = (PyArrayObject*)wrapWithNumpy(data);
        nparray->flags &= ~NPY_WRITEABLE;
        return (PyObject*)nparray;
      }
      
      /**
       *  Create a numpy array wrapper around a Matrix.
       *  @param data :: A const reference to an existing Matrix
       *  @return A numpy wrapper around the existing data, no copy is performed. It is left in a read-write state.
       */
      PyObject *wrapWithNumpy(const Kernel::DblMatrix & data)
      {
        std::pair<size_t,size_t> matrixSize = data.size();
        npy_intp dims[2] =  {matrixSize.first, matrixSize.second};
        PyArrayObject * ndarray = 
          (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_DOUBLE,(void*)&(data[0][0]));
        return (PyObject*)ndarray;
      }

      /**
       *  Create a read-only numpy array wrapper around a Matrix
       *  @param data :: A const reference to an existing Matrix
       *  @return A numpy wrapper around the existing data, no copy is performed. It is marked read-only.
       */
      PyObject *wrapWithReadOnlyNumpy(const Kernel::DblMatrix& data)
      {
        PyArrayObject *nparray = (PyArrayObject*)wrapWithNumpy(data);
        nparray->flags &= ~NPY_WRITEABLE;
        return (PyObject*)nparray;
      }

    }
  }
}
