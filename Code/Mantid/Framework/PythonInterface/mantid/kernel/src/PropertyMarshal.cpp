//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
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

      /// Typedef the map of python types to C++ functions
      typedef std::map<PyTypeObject*, PropertyHandler*> PyTypeLookup;
      /// There is no interface other than insert defined in the header so
      /// it's isolated here
      static PyTypeLookup typeHandlers;

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
      void setProperty(bpl::object self, const std::string & name, bpl::object value)
      {
        // Find type handler
        PyTypeObject *pytype = value.ptr()->ob_type;
        PyTypeLookup::const_iterator itr = typeHandlers.find(pytype);
        if( itr != typeHandlers.end() )
        {
          itr->second->set(bpl::extract<IPropertyManager*>(self), name, value);
        }
        else
        {
          throw std::invalid_argument("PropertyMarshal::setProperty - No C++ function registered to handle python type '" + std::string(pytype->tp_name) + "'");
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
        bpl::object declaredType = bpl::object(bpl::handle<>(PyObject_GetAttrString(self.ptr(), "value_as_declared")));

        // Work in progress to upcast pointers

        //static const bpl::converter::registration *reg = bpl::converter::registry::query(typeid(Kernel::DataItem));
        //PyTypeObject *cls = reg->get_class_object();
        //if( !PyObject_IsInstance(declaredType.ptr(), (PyObject*)cls) )
        //{
        //  return declaredType;
        //}
        //// Try and deduce the actual object type rather than just returning the low-level interface
        //const bpl::converter::registration *mwReg = bpl::converter::registry::query(typeid(API::MatrixWorkspace));
        //if( reg )
        //{
        //  PyObject_SetAttrString(declaredType.ptr(), "__class__", (PyObject*)mwReg->get_class_object());
        //}

        return declaredType;
      }

      /**
      * Insert a new property handler
      * @param typeObject :: A pointer to a type object
      * @param handler :: An object to handle to corresponding templated C++ type
      */
      void registerHandler(PyTypeObject* typeObject, PropertyHandler* handler)
      {
        typeHandlers.insert(std::make_pair(typeObject, handler));
      }

    }
  }
}
