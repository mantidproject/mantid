//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
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
     PyObjectToV3D::PyObjectToV3D(const boost::python::object & p)
       : m_obj(p), m_alreadyV3D(false)
     {
       // Is it an already wrapped V3D ?
       boost::python::extract<Kernel::V3D> converter(p);
       if( converter.check() )
       {
         m_alreadyV3D = true;
         return;
       }
       // Is it a sequence
       try
       {
         const size_t length = boost::python::len(p);
         if( length != 3 )
         {
           throw std::invalid_argument("Incorrect length for conversion to V3D");
         }
         // Can we index the object
         p.attr("__getitem__")(0);
       }
       catch(boost::python::error_already_set&)
       {
         throw std::invalid_argument(std::string("Cannot convert object to V3D. Expected a python sequence found ")
                                       + p.ptr()->ob_type->tp_name);
       }
     }

     /**
      * Returns a V3D object from the Python object given
      * to the converter
      * @returns A newly constructed V3D object converted
      * from the PyObject.
      */
     Kernel::V3D PyObjectToV3D::operator ()()
     {
       using namespace boost::python;
       if(m_alreadyV3D)
       {
         return extract<Kernel::V3D>(m_obj)();
       }
       // Numpy arrays need to be forced to a double
       // as extract cannot convert from a int64->double
       boost::python::object obj = m_obj;
       if( PyArray_Check(obj.ptr()) )
       {
         obj = boost::python::numeric::array(obj).astype('d');
       }
       // Must be a sequence
       return Kernel::V3D(extract<double>(obj[0])(),
                          extract<double>(obj[1])(),
                          extract<double>(obj[2])());
     }

   }
  }
}
