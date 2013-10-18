//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/DowncastRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include <boost/python/extract.hpp>
#include <map>
#include <stdexcept>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      namespace // <anonymous>
      {
        /// Typedef the map of type_info -> handler objects
        typedef std::map<std::string, const PyTypeObject*> PyTypeMap;

        /**
         * Returns a reference to the static type map
         * @return A reference to the type map
         */
        PyTypeMap & downcastRegistry()
        {
          static PyTypeMap registry;
          return registry;
        }

        /**
         * Returns a registered converter
         * @param id :: The string id of class
         * @return A pointer to a downcastToPython struct
         * @throws std::invalid_argument if none exists
         */
        const PyTypeObject * queryDowncastRegistry(const std::string & id)
        {
          PyTypeMap & converters = downcastRegistry();
          PyTypeMap::const_iterator it = converters.find(id);
          if(it != converters.end())
          {
            return it->second;
          }
          else
          {
            throw std::runtime_error(std::string("An downcasted type cannot be found for \"") + id + "\".");
          }
        }

      } // end <anonymous>

      /**
       * Registers a Python type for the object with the given
       * type ID string to be represented as
       * @param id :: A string ID identifying the class
       * @param type:: A pointer to a converter type
       * @throws std::runtime_error if a converter for this id already exists
       */
      void registerIDForDowncasting(const std::string & id, const PyTypeObject * type)
      {
        PyTypeMap & converters = downcastRegistry();
        PyTypeMap::const_iterator it = converters.find(id);
        if(it == converters.end())
        {
          converters.insert(std::make_pair(id, type));
        }
        else
        {
          throw std::runtime_error(std::string("The plugin ID \"") + id +  "\" already exists, please ensure they are unique.");
        }
      }


      /**
       * Attempts to find an downcasted PyTypeObject type for the given object from
       * all of the types registered.
       * @param value :: An object derived from DataItem
       * @return A pointer to an downcasted type or NULL if one cannot be found
       */
      const PyTypeObject * getDerivedType(const boost::python::object & value)
      {
        // This has to be a search as it is at runtime.
        // Each of the registered type handlers is checked
        // to see if its type is a subclass the value type.
        // Each one is checked so that the most derived type
        // can be found. The Python object must have a "id()" method

        // There is a bug on the Mac where if a Python exception is thrown here
        // then while it is caught correctly the Python error handler is not
        // reset and future calls then produce garbage errors like
        // "RuntimeError: 'NoneType' object has no attribute 'id'".
        // Solution is to check for attribute explicitly
        const char * attrName("id");
        if(PyObject_HasAttrString(value.ptr(), attrName) > 0)
        {
          boost::python::object id_as_py = value.attr(attrName)();
          const std::string id = boost::python::extract<std::string>(id_as_py);
          const PyTypeObject *result(NULL);
          try
          {
            result = queryDowncastRegistry(id);
          }
          catch(std::runtime_error&)
          {
            result = findDerivedType(value);
            if( result ) registerIDForDowncasting(id, result);
          }
          return result;
        }
        else
          return NULL;
      }

      /**
       * Attempts to find an downcasted PyTypeObject type for the given object from
       * all of the types registered, overloaded for bare PyObject pointer.
       * @param value :: An object derived from DataItem
       * @return A pointer to an downcasted type or NULL if one cannot be found
       */
      const PyTypeObject * getDerivedType(PyObject *value)
      {
        return getDerivedType(boost::python::object(boost::python::borrowed(value)));
      }

    }
  }
}
