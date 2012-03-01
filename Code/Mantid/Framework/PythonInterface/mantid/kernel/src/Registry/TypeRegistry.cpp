//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include <map>
#include <boost/python/type_id.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      namespace // <anonymous>
      {
        /// Typedef the map of type_info -> handler objects
        typedef std::map<const boost::python::type_info, PropertyValueHandler*> TypeIDMap;

        /**
         * Returns a reference to the static type map
         * @return A reference to the type map
         */
        TypeIDMap & typeRegistry()
        {
          static TypeIDMap typeHandlers;
          return typeHandlers;
        }
      } // end <anonymous>

      /**
       * Register the built-in type handlers into the registry
       */
      void registerBuiltins()
      {
        // Register a handler for each basic type in IPropertyManager.cpp
        REGISTER_SINGLEVALUE_HANDLER(int32_t);
        REGISTER_SINGLEVALUE_HANDLER(uint32_t);
        REGISTER_SINGLEVALUE_HANDLER(int64_t);
        REGISTER_SINGLEVALUE_HANDLER(uint64_t);
        REGISTER_SINGLEVALUE_HANDLER(unsigned int);
        REGISTER_SINGLEVALUE_HANDLER(bool);
        REGISTER_SINGLEVALUE_HANDLER(double);
        REGISTER_SINGLEVALUE_HANDLER(std::string);

       #define REGISTER_ARRAYPROPERTY_HANDLER(TYPE) \
         registerHandler(typeid(TYPE), new SequenceTypeHandler<TYPE>());

        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<int16_t>);
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<uint16_t>);
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<int32_t>);
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<uint32_t>);
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<int64_t>);
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<uint64_t>);
#ifdef __APPLE__
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<unsigned long>);
#endif
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<double>);
        REGISTER_ARRAYPROPERTY_HANDLER(std::vector<std::string>);

      }

      /**
       * Insert a new property handler, possibly overwriting an existing one
       * @param typeObject :: A pointer to a type object
       * @param handler :: An object to handle to corresponding templated C++ type
       */
      void registerHandler(const std::type_info& typeObject, PropertyValueHandler* handler)
      {
        TypeIDMap & typeHandlers = typeRegistry();
        typeHandlers.insert(std::make_pair(boost::python::type_info(typeObject), handler));
      }

      /**
       * Get a TypeHandler, throws if one does not exist
       * @param typeObject A pointer to a PyTypeObject
       * @returns A pointer to a PropertyValueHandler
       */
      PropertyValueHandler *getHandler(const std::type_info& typeObject)
      {
        TypeIDMap & typeHandlers = typeRegistry();
        TypeIDMap::const_iterator itr = typeHandlers.find(boost::python::type_info(typeObject));
        if( itr == typeHandlers.end() )
        {
          throw std::invalid_argument(std::string("No handler registered for property value type '") +
               boost::python::type_info(typeObject).name() + "'");
        }
        return itr->second;
      }

      /**
        * Attempts to find a derived type for the given object amongst the
        * known converters (This could be slow)
        */
      const PyTypeObject * findDerivedType(boost::python::object value)
       {
         TypeIDMap & typeHandlers = typeRegistry();
         TypeIDMap::const_iterator iend = typeHandlers.end();
         const PyTypeObject *result(NULL);

         for(TypeIDMap::const_iterator it = typeHandlers.begin(); it != iend; ++it)
         {
           if( it->second->checkExtract(value) )
           {
             const PyTypeObject *derivedType = it->second->pythonType();
             if( !result )
             {
               result = derivedType;
             }
             // Further down the chain
             else if( PyObject_IsSubclass((PyObject*)derivedType, (PyObject*)result) )
             {
               result = derivedType;
             }
           }
         }
         return result;
       }

    }
  }
}
