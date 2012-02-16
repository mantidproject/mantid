//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/UpcastRegistry.h"
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
        PyTypeMap & upcastRegistry()
        {
          static PyTypeMap registry;
          return registry;
        }

        /**
         * Returns a registered converter
         * @param id :: The string id of class
         * @return A pointer to a UpcastToPython struct
         * @throws std::invalid_argument if none exists
         */
        const PyTypeObject * queryUpcastRegistry(const std::string & id)
        {
          PyTypeMap & converters = upcastRegistry();
          PyTypeMap::const_iterator it = converters.find(id);
          if(it != converters.end())
          {
            return it->second;
          }
          else
          {
            throw std::runtime_error(std::string("An upcasted type cannot be found for \"") + id + "\".");
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
      void registerIDForUpcasting(const std::string & id, const PyTypeObject * type)
      {
        PyTypeMap & converters = upcastRegistry();
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
       * Attempts to find an upcasted PyTypeObject type for the given object from
       * all of the types registered.
       * @param value :: An object derived from DataItem
       * @return A pointer to an upcasted type or NULL if one cannot be found
       */
      const PyTypeObject * getDerivedType(boost::python::object value)
      {
        // This has to be a search as it is at runtime.
        // Each of the registered type handlers is checked
        // to see if its type is a subclass the value type.
        // Each one is checked so that the most derived type
        // can be found. The Python object must have a "id()" method
        boost::python::object id_as_py;
        try
        {
          id_as_py = value.attr("id")();
        }
        catch(boost::python::error_already_set&)
        {
          return NULL;
        }
        const std::string id = boost::python::extract<std::string>(id_as_py);
        const PyTypeObject *result(NULL);
        try
        {
          result = queryUpcastRegistry(id);
        }
        catch(std::runtime_error&)
        {
          result = findDerivedType(value);
          if( result ) registerIDForUpcasting(id, result);
        }
        return result;
      }

      /**
       * Attempts to find an upcasted PyTypeObject type for the given object from
       * all of the types registered, overloaded for bare PyObject pointer.
       * @param value :: An object derived from DataItem
       * @return A pointer to an upcasted type or NULL if one cannot be found
       */
      const PyTypeObject * getDerivedType(PyObject *value)
      {
        return getDerivedType(boost::python::object(boost::python::borrowed(value)));
      }

    }
  }
}
