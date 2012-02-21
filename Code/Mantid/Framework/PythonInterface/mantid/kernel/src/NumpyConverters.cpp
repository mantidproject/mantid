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

      //--------------------------------------------------------------------------------------------
      // Creation of Mantid objects
      //--------------------------------------------------------------------------------------------

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
