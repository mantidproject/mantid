#include "MantidPythonInterface/kernel/NumpyConverters.h"

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <boost/python/tuple.hpp>
#include <boost/python/numeric.hpp>
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Numpy
    {
      /**
       * Returns a pointer to the PyArray_Type object
       * @return
       */
      PyTypeObject * getNDArrayType()
      {
        return &PyArray_Type;
      }


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

      //--------------------------------------------------------------------------------------------
      // Creation of Mantid objects
      //--------------------------------------------------------------------------------------------
      /**
       * Try and create a Mantid V3D object from the given PyObject.
       * @param data A pointer to a Python object representing either a V3D, a list or a numpy array
       * @return A new V3D object
       */
      Kernel::V3D createV3D(PyObject *data)
      {
        using namespace boost::python;
        extract<Kernel::V3D> converter(data);
        // Is it an already wrapped V3D ?
        if( converter.check() ) return converter();

        Kernel::V3D result;
        // Python list
        if( PyList_Check(data) )
        {
          boost::python::list pyList = extract<boost::python::list>(data);
          if( len(pyList) == 3 )
          {
            result[0] = boost::python::extract<double>(pyList[0]);
            result[1] = boost::python::extract<double>(pyList[1]);
            result[2] = boost::python::extract<double>(pyList[2]);
          }
          else
          {
            std::ostringstream msg;
            msg << "createV3D - Expected Python list to be of length 3, length=" << len(pyList);
            throw std::invalid_argument(msg.str());
          }

        }
        // Numpy array
        else if( PyArray_Check(data) )
        {
          numeric::array ndarray = extract<numeric::array>(data);
          if( PyArray_Size(ndarray.ptr()) == 3)
          {
            //force the array to be of double type (in case it was int)
            numeric::array doubleArray = (boost::python::numeric::array)ndarray.astype('d');
            result[0] = extract<double>(doubleArray[0]);
            result[1] = extract<double>(doubleArray[1]);
            result[2] = extract<double>(doubleArray[2]);
          }
          else
          {
            std::ostringstream msg;
            msg << "createV3D - Expected numpy array to be of length 3, length=" << PyArray_Size(ndarray.ptr());
            throw std::invalid_argument(msg.str());
          }
        }
        else
        {
          throw std::invalid_argument(std::string("createV3D - Expected a V3D, list or numpy array but found a ") + data->ob_type->tp_name);
        }

        return result;
      }

      /**
       * Create a Matrix of doubles from a 2D numpy array
       * @param data A python object that points to a 2D numpy array
       * @return A Matrix<double> created from the input array
       */
      Kernel::DblMatrix createDoubleMatrix(PyObject *data)
      {
        using namespace boost::python;
        if( PyArray_Check(data) )
        {
          numeric::array numarray = extract<numeric::array>(data);
          numarray = (boost::python::numeric::array) numarray.astype('d'); // Force the array to be of double type (in case it was int)
          boost::python::tuple shape( numarray.attr("shape") );
          if( boost::python::len(shape) != 2 )
          {
            std::ostringstream msg;
            msg << "createDoubleMatrix - Expected an array with 2 dimensions but was given array with "
                << boost::python::len(shape) << " dimensions.";
            throw std::invalid_argument(msg.str());
          }
          size_t nx = boost::python::extract<size_t>(shape[0]);
          size_t ny = boost::python::extract<size_t>(shape[1]);
          Kernel::Matrix<double> matrix(nx,ny);
          for( size_t i = 0; i < nx; i++ )
          {
            for( size_t j = 0; j < ny; j++ )
            {
              matrix[i][j] = extract<double>( numarray[boost::python::make_tuple( i, j )]);
            }
          }
          return matrix;
        }
        else
        {
          throw std::invalid_argument(std::string("createDoubleMatrix - Expected numpy array as input, found ") + data->ob_type->tp_name);
        }
      }

    } // Numpy
  }
}
