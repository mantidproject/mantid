//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace bpl = boost::python;
    using Kernel::IPropertyManager;


    //------------------------------------------------------------------------------------------
    // Property Marshal
    //------------------------------------------------------------------------------------------
    /// Intialize static type lookup
    std::map<PyTypeObject*, PropertyHandler*> PropertyMarshal::g_handlers;

    /**
     * This static function allows a call to a method on an IAlgorithm object
     * in Python to get routed here.
     * The first argument MUST be an object of type bpl::object that
     * provides access to the object that performed the method call.
     * It is equivalent to a python method that starts with 'self'
     * @param self :: A reference to the calling object
     * @param name :: The name of the property
     * @param value :: The value of the property as a bpl object
     */
    void PropertyMarshal::setProperty(bpl::object self, const std::string & name,
        bpl::object value)
    {
      // Find type handler
      PyTypeObject *pytype = value.ptr()->ob_type;
      PyTypeLookup::const_iterator itr = g_handlers.find(pytype);
      if( itr != g_handlers.end() )
      {
        itr->second->set(bpl::extract<IPropertyManager*>(self), name, value);
      }
      else
      {
        throw std::invalid_argument("PropertyMarshal::setProperty - No C++ function registered to handle python type '" + std::string(pytype->tp_name) + "'");
      }
    }

    /**
     * Insert a new property handler
     * @param typeObject :: A pointer to a type object
     * @param handler :: An object to handle to corresponding templated C++ type
     */
    void PropertyMarshal::insert(PyTypeObject* typeObject, PropertyHandler* handler)
    {
      g_handlers.insert(std::make_pair(typeObject, handler));
    }
  }
}
