//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include <map>
#include <boost/python/type_id.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
namespace // <anonymous>
    {
/// Typedef the map of type_info -> handler objects. We store
/// boost::python::type_info objects so that they work across DLL boundaries
/// unlike std::type_info objects
using TypeIDMap = std::map<const boost::python::type_info,
                           boost::shared_ptr<PropertyValueHandler>>;

/**
 * Returns a reference to the static type map
 * @return A reference to the type map
 */
TypeIDMap &typeRegistry() {
  static TypeIDMap typeHandlers;
  return typeHandlers;
}
} // end <anonymous>

//-------------------------------------------------------------------------------------------
// Public methods
//-------------------------------------------------------------------------------------------
/**
 */
void TypeRegistry::registerBuiltins() {
// -- Register a handler for each basic type and vector of each basic type +
// std::string --
// macro helps with keeping information in one place
#define SUBSCRIBE_HANDLER(Type)                                                \
  subscribe<TypedPropertyValueHandler<Type>>();                                \
  subscribe<SequenceTypeHandler<std::vector<Type>>>();

  // unsigned ints
  SUBSCRIBE_HANDLER(int);
  SUBSCRIBE_HANDLER(long);
  SUBSCRIBE_HANDLER(long long);
  // signed ints
  SUBSCRIBE_HANDLER(unsigned int);
  SUBSCRIBE_HANDLER(unsigned long);
  SUBSCRIBE_HANDLER(unsigned long long);
  // boolean
  SUBSCRIBE_HANDLER(bool);
  // double
  SUBSCRIBE_HANDLER(double);
  // string
  SUBSCRIBE_HANDLER(std::string);

#undef SUBSCRIBE_HANDLER
}

/**
 * Insert a new property handler for the given type_info
 * @param typeObject :: A reference to a type object
 * @param handler :: An object to handle to corresponding templated C++ type.
 * Ownership is transferred here
 * @throws std::invalid_argument if one already exists
 */
void TypeRegistry::subscribe(const std::type_info &typeObject,
                             PropertyValueHandler *handler) {
  TypeIDMap &typeHandlers = typeRegistry();
  boost::python::type_info typeInfo(typeObject);
  if (typeHandlers.find(typeInfo) == typeHandlers.end()) {
    typeHandlers.emplace(typeInfo,
                         boost::shared_ptr<PropertyValueHandler>(handler));
  } else {
    throw std::invalid_argument(
        std::string("TypeRegistry::subscribe() - A handler has already "
                    "registered for type '") +
        typeInfo.name() + "'");
  }
}

/**
 * Get a PropertyValueHandler if one exists
 * @param typeObject A pointer to a PyTypeObject
 * @returns A pointer to a PropertyValueHandler
 * @throws std::invalid_argument if one is not registered
 */
const PropertyValueHandler &
TypeRegistry::retrieve(const std::type_info &typeObject) {
  TypeIDMap &typeHandlers = typeRegistry();
  TypeIDMap::const_iterator itr =
      typeHandlers.find(boost::python::type_info(typeObject));
  if (itr != typeHandlers.end()) {
    return *(itr->second);
  } else {
    throw std::invalid_argument(
        std::string("TypeRegistry::retrieve(): No PropertyValueHandler "
                    "registered for type '") +
        boost::python::type_info(typeObject).name() + "'");
  }
}
}
}
}
