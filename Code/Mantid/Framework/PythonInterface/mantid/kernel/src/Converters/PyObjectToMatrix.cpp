//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"
#include <boost/python/extract.hpp>
#include <boost/python/numeric.hpp>

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {
      /**
       * Construct the converter object with the given Python object
       * @param p :: A boost::python object that should support
       * the __getitem__ and __len__ protocol or be a wrapped V3D object.
       * Throws std::invalid_argument if not
       * if that is not the case.
       */
      PyObjectToMatrix::PyObjectToMatrix(const boost::python::object & p)
        : m_obj(p), m_alreadyMatrix(false)
      {
        // Is it an already wrapped V3D ?
        boost::python::extract<Kernel::Matrix<double> > converter(p);
        if( converter.check() )
        {
         m_alreadyMatrix = true;
         return;
        }
        // Is it a 2D numpy array
        if( !PyArray_Check(p.ptr()) )
        {
          throw std::invalid_argument(std::string("Cannot convert object to Matrix. Expected a numpy array found ")
                                       + p.ptr()->ob_type->tp_name);
        }
      }

     /**
      * Returns a V3D object from the Python object given
      * to the converter
      * @returns A newly constructed V3D object converted
      * from the PyObject.
      */
      Kernel::Matrix<double> PyObjectToMatrix::operator ()()
      {
        using namespace boost::python;
        if(m_alreadyMatrix)
        {
         return extract<Kernel::Matrix<double> >(m_obj)();
        }
        numeric::array numarray = numeric::array(m_obj);
        numarray = (boost::python::numeric::array) numarray.astype('d'); // Force the array to be of double type (in case it was int)
        boost::python::tuple shape( numarray.attr("shape") );
        if( boost::python::len(shape) != 2 )
        {
          std::ostringstream msg;
          msg << "Error in conversion to Matrix. Expected an array with 2 dimensions but was given array with "
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
   }
  }
}