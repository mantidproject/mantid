//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/DowncastRegistry.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <stdexcept>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
namespace // <anonymous>
    {
/// Typedef the map of type_info -> Downcaster objects
typedef boost::unordered_map<
    std::string, boost::shared_ptr<const DowncastDataItem>> RegistryType;
// typedef std::map<std::string, boost::shared_ptr<const DowncastDataItem>>
// RegistryType;

/**
 * Returns a reference to the static type map. Creates on first call to the
 * function
 * @return A reference to the type map
 */
RegistryType &downcastRegistry() {
  static RegistryType registry;
  return registry;
}
} // end <anonymous>

//-----------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------
/**
 * Throws std::invalid_argument if the item does not exist
 * @param id A string ID from a concrete DataItem type
 * @return The object responsible for casting and creating a Python object
 *         from it
 */
const DowncastDataItem &DowncastRegistry::retrieve(const std::string &id) {
  auto &registry = downcastRegistry();
  auto entry = registry.find(id);
  if (entry != registry.cend()) {
    return *(entry->second);
  } else {
    throw std::invalid_argument(
        "DowncastRegistry::retrieve - Unable to find registered "
        "object with id=" +
        id);
  }
}

//-----------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------
/**
 * Subscribe a caster object with a given ID
 * @param id A string ID that will map to the object
 * @param caster A pointer to a DowncastDataItem object. Ownership of the
 *               object is transferred here
 */
void DowncastRegistry::subscribe(const std::string &id,
                                 const DowncastDataItem *caster) {
  auto &registry = downcastRegistry();
  if (registry.find(id) == registry.cend()) {
    typedef boost::shared_ptr<const DowncastDataItem> DowncastDataItemPtr;
    registry.insert(std::make_pair(id, DowncastDataItemPtr(caster)));
  } else {
    throw std::invalid_argument(
        "DowncastRegistry::subscribe - object with ID=" + id +
        " has already been registered");
  }
}
}
}
}
