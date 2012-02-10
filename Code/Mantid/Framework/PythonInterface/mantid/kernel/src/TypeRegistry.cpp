//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/TypeRegistry.h"
#include "MantidPythonInterface/kernel/PythonTypeHandler.h"
#include <map>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace TypeRegistry
    {
      namespace // <anonymous>
      {
        /// Typedef the map of python types to C++ functions
        typedef std::map<PyTypeObject*, PythonTypeHandler*> PyTypeLookup;

        /**
         * Returns a reference to the static type map
         * @return A reference to the type map
         */
        PyTypeLookup & typeRegistry()
        {
          static PyTypeLookup typeHandlers;
          return typeHandlers;
        }
      } // end <anonymous>

      /**
       * Insert a new property handler, possibly overwriting an existing one
       * @param typeObject :: A pointer to a type object
       * @param handler :: An object to handle to corresponding templated C++ type
       */
      void registerHandler(PyTypeObject* typeObject, PythonTypeHandler* handler)
      {
        PyTypeLookup & typeHandlers = typeRegistry();
        typeHandlers.insert(std::make_pair(typeObject, handler));
      }

      /**
       * Get a TypeHandler, throws if one does not exist
       * @param typeObject A pointer to a PyTypeObject
       * @returns A pointer to a PythonTypeHandler
       */
      PythonTypeHandler *getHandler(PyTypeObject* typeObject)
      {
        PyTypeLookup & typeHandlers = typeRegistry();
        PyTypeLookup::const_iterator itr = typeHandlers.find(typeObject);
        if( itr == typeHandlers.end() )
        {
          throw std::invalid_argument("No handler registered for python type '" + std::string(typeObject->tp_name) + "'");
        }
        return itr->second;
      }

      /**
       * Attempts to find an upcasted PyTypeObject type for the given object from
       * all of the types registered.
       * @param value :: An object derived from DataItem
       * @return A pointer to an upcasted type or NULL if one cannot be found
       */
      PyTypeObject * getDerivedType(boost::python::object value)
      {
        // This has to be a search as it is at runtime.
        // Each of the registered type handlers is checked
        // to see if its type is a subclass the value type.
        // Each one is checked so that the most derived type
        // can be found

        PyTypeLookup & typeHandlers = typeRegistry();
        PyTypeLookup::const_iterator iend = typeHandlers.end();
        PyObject *valueType = (PyObject*)value.ptr()->ob_type;
        PyTypeObject *result(NULL);

        for(PyTypeLookup::const_iterator it = typeHandlers.begin(); it != iend; ++it)
        {
          if( PyObject_IsSubclass((PyObject*)it->first, valueType) )
          {
            if( !result && it->second->isInstance(value) )
            {
              result = it->first;
            }
            // Check if this match is further up the chain than the last
            if( result && PyObject_IsSubclass((PyObject*)it->first, (PyObject*)result)
                && it->second->isInstance(value) )
            {
              result = it->first;
            }
          }
        }
        return result;
      }

      /**
       * Attempts to find an upcasted PyTypeObject type for the given object from
       * all of the types registered, overloaded for bare PyObject pointer.
       * @param value :: An object derived from DataItem
       * @return A pointer to an upcasted type or NULL if one cannot be found
       */
      PyTypeObject * getDerivedType(PyObject *value)
      {
        return getDerivedType(boost::python::object(boost::python::borrowed(value)));
      }

    }
  }
}
