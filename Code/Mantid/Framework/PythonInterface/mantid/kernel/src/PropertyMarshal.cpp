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
       * Insert a new property handler
       * @param typeObject :: A pointer to a type object
       * @param handler :: An object to handle to corresponding templated C++ type
       */
      void registerHandler(PyTypeObject* typeObject, PropertyHandler* handler)
      {
        typeHandlers.insert(std::make_pair(typeObject, handler));
      }

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
       * Attempts to find an upcasted pointer type for the given object that is
       * assumed to be of type DataItem
       * @param value :: An object derived from DataItem
       * @return A pointer to an upcasted type or NULL if one cannot be found
       */
      PyTypeObject * getUpcastedType(bpl::object value)
      {
        // The search proceeds by first finding all possible candidates
        // that the value can be converted to. The most derived type
        // is then found among these.
        PyTypeObject *result(NULL);

        PyTypeLookup::const_iterator iend = typeHandlers.end();
        PyObject *valueType = (PyObject*)value.ptr()->ob_type;
        for(PyTypeLookup::const_iterator it = typeHandlers.begin(); it != iend; ++it)
        {
          if( PyObject_IsSubclass((PyObject*)it->first, valueType) )
          {
            if( !result && it->second->isInstance(value) ) // First one
            {
              result = it->first;
            }
            // Check if this match is further up the chain than the last
            else if(PyObject_IsSubclass((PyObject*)it->first, (PyObject*)result) && it->second->isInstance(value) )
            {
              result = it->first;
            }
          }
        }
        return result;
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
          PyTypeObject *derivedType = getUpcastedType(value);
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
        bpl::object value = bpl::object(bpl::handle<>(PyObject_GetAttrString(self.ptr(), "value_as_declared")));
        upcastFromDataItem(value);
        return value;
      }


    }
  }
}
