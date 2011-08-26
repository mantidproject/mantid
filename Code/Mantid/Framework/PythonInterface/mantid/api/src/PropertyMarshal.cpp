//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PropertyMarshal.h"
#include "MantidAPI/IAlgorithm.h"

#include <boost/python/extract.hpp>
#include <typeinfo>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace python = boost::python;
    using API::IAlgorithm;
    using Kernel::Property;

    namespace
    {
      /**
       * Extracts a pointer to an algorithm from a python object
       * @param obj :: A boost::python object
       * @returns A pointer to an IAlgorithm
       */
      IAlgorithm * getAlgorithm(python::object obj)
      {
        return python::extract<IAlgorithm*>(obj); // Throws TypeError if incorrect
      }
    }

    /**
     * Set a named property's value
     * @param self :: A reference to the calling object
     * @param name :: The name of the property
     * @param value :: The value of the property as a boost::python object
     */
    template<typename CType>
    void PropertyMarshal<CType>::setProperty(python::object self, const std::string & name,
        CType value)
    {
      IAlgorithm * alg = getAlgorithm(self);
      alg->setProperty(name, value);
    }

    //---------------------------------------------------------------------------
    // Concrete implementations.
    //---------------------------------------------------------------------------

    ///@cond
    // Concrete implementations.
    template DLLExport struct PropertyMarshal<int>;
    template DLLExport struct PropertyMarshal<bool>;
    template DLLExport struct PropertyMarshal<double>;
    template DLLExport struct PropertyMarshal<std::string>;
    ///@endcond

  }
}
