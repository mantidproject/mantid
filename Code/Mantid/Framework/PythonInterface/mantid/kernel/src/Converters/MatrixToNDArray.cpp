//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"

#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid { namespace PythonInterface
{
  namespace Converters
  {
    namespace Impl
    {
      /**
       * Defines the wrapWithNDArray specialization for Matrix container types
       *
       * Wraps a vector in a numpy array structure without copying the data
       * @param cmatrix :: A reference to the Matrix wrap
       * @param mode :: A mode switch to define whether the final array is read only/read-write
       * @return A pointer to a numpy ndarray object
       */
      template<typename ContainerType>
      PyObject *wrapWithNDArray(const ContainerType & cmatrix, const NumpyWrapMode mode)
      {
        std::pair<size_t,size_t> matrixDims = cmatrix.size();
        npy_intp dims[2] =  {matrixDims.first, matrixDims.second};
        int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
        PyObject * ndarray = PyArray_SimpleNewFromData(2, dims, datatype,(void*)&(cmatrix[0][0]));
        if( mode == ReadOnly )
        {
          PyArrayObject * np = (PyArrayObject *)ndarray;
          np->flags &= ~NPY_WRITEABLE;
        }
        return ndarray;
      }
      //-----------------------------------------------------------------------
      // Explicit instantiations
      //-----------------------------------------------------------------------
      #define INSTANTIATE(ElementType) \
        template DLLExport PyObject * wrapWithNDArray<Kernel::Matrix<ElementType> >(const Kernel::Matrix<ElementType> &, const NumpyWrapMode);

      INSTANTIATE(int);
      INSTANTIATE(float);
      INSTANTIATE(double);

    }
  }
}}
