//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include "MantidPythonInterface/kernel/PythonTypeHandler.h"
#include "MantidPythonInterface/kernel/TypeRegistry.h"

#include "MantidAPI/MatrixWorkspace.h"
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace PropertyMarshal
    {

      namespace bpl = boost::python;
      using Kernel::IPropertyManager;

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
      void setProperty(IPropertyManager &self, const std::string & name, bpl::object value)
      {
        // Find type handler
        PyTypeObject *pytype = value.ptr()->ob_type;
        TypeRegistry::PythonTypeHandler *entry = TypeRegistry::getHandler(pytype);
        entry->set(&self, name, value);
      }

      /**
       * Assuming the given object is a sub class of DataItem, attempt to
       * upcast it to the most derived exported type
       * @param value :: An object derived from DataItem
       * @return A (possibly) upcasted pointer
       */
      void upcastFromDataItem(bpl::object value)
      {
        static const bpl::converter::registration *reg = bpl::converter::registry::query(typeid(Kernel::DataItem));
        PyTypeObject *cls = reg->get_class_object();
        if( PyObject_IsInstance(value.ptr(), (PyObject*)cls) ) // Is this a subtype of DataItem
        {
          PyTypeObject *derivedType = TypeRegistry::getDerivedType(value);
          if( derivedType )
          {
            PyObject_SetAttrString(value.ptr(), "__class__", (PyObject*)derivedType);
          }
        }
      }

      /**
      * Attempts to convert the value of the property to an appropriate type
      * e.g. for DataItem objects it tries to give back the most derived exported
      * type of the object
      * The first argument MUST be an object of type bpl::object that
      * provides access to the object that performed the method call.
      * @param self :: A reference to the calling object
      */
      bpl::object value(bpl::object self)
      {
        bpl::object value = bpl::object(bpl::handle<>(PyObject_GetAttrString(self.ptr(), "valueAsDeclared")));
        upcastFromDataItem(value);
        return value;
      }


    }
  }
}
