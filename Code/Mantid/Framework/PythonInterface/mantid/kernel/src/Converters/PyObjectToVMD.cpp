//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/PyObjectToVMD.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/extract.hpp>
#include <boost/python/numeric.hpp>

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

GCC_DIAG_OFF(strict-aliasing)

namespace Mantid
{
  namespace PythonInterface
  {
   namespace Converters
   {
     /**
      * Construct the converter object with the given Python object
      * @param p :: A boost::python object that should support
      * the __getitem__ and __len__ protocol or be a wrapped VMD object.
      * Throws std::invalid_argument if not
      * if that is not the case.
      */
     PyObjectToVMD::PyObjectToVMD(const boost::python::object & p)
       : m_obj(p), m_alreadyVMD(false)
     {
       // Is it an already wrapped VMD ?
       boost::python::extract<Kernel::VMD> converter(p);
       if( converter.check() )
       {
         m_alreadyVMD = true;
         return;
       }
       // Is it a sequence
       try
       {
         const size_t length = boost::python::len(p);
         if( length < 3 )
         {
           throw std::invalid_argument("Must be > 2 for conversion to VMD");
         }
         // Can we index the object
         p.attr("__getitem__")(0);
       }
       catch(boost::python::error_already_set&)
       {
         throw std::invalid_argument(std::string(
               "Cannot convert object to VMD. "
               "Expected a python sequence found: ")
               + p.ptr()->ob_type->tp_name);
       }
     }

     /**
      * Returns a VMD object from the Python object given
      * to the converter
      * @returns A newly constructed VMD object converted
      * from the PyObject.
      */
     Kernel::VMD PyObjectToVMD::operator ()()
     {
       using namespace boost::python;
       if(m_alreadyVMD)
       {
         return extract<Kernel::VMD>(m_obj)();
       }
       // Numpy arrays need to be forced to a double
       // as extract cannot convert from a int64->double
       boost::python::object obj = m_obj;
       if( PyArray_Check(obj.ptr()) )
       {
         obj = boost::python::numeric::array(obj).astype('d');
       }
       // Must be a sequence

       const size_t length = boost::python::len(obj);
       Kernel::VMD ret(length);

       for(size_t i = 0; i < length; ++i)
         ret[i] = extract<float>(obj[i])();

       return ret;
     }

   }
  }
}
