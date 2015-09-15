#include "MantidGeometry/Crystal/BraggScatterer.h"

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Default constructor.
BraggScatterer::BraggScatterer()
    : PropertyManager(), m_propagatingGroupName("PropagatingProperty"),
      m_isInitialized(false) {}

/// Initialization method that calls declareProperties() and sets initialized
/// state to true.
void BraggScatterer::initialize() {
  declareProperties();

  m_isInitialized = true;
}

/// Returns whether the instance has been initialized.
bool BraggScatterer::isInitialized() { return m_isInitialized; }

/// Returns |F(hkl)|^2
double BraggScatterer::calculateFSquared(const V3D &hkl) const {
  double modulusF = std::abs(calculateStructureFactor(hkl));

  return modulusF * modulusF;
}

/// Checks whether a property with the given name is exposed to
/// BraggScattererComposite.
bool BraggScatterer::isPropertyExposedToComposite(
    const std::string &propertyName) const {
  Property *property = getProperty(propertyName);

  return isPropertyExposedToComposite(property);
}

/// Checks if a property is exposed to BraggScattererComposite or throws
/// std::invalid_argument if a null-pointer is supplied.
bool BraggScatterer::isPropertyExposedToComposite(Property *property) const {
  if (!property) {
    throw std::invalid_argument(
        "Cannot determine propagation behavior of null-property.");
  }

  return property->getGroup() == getPropagatingGroupName();
}

/**
 * Exposes the property with the supplied name to BraggScattererComposite
 *
 * When a property is marked to be exposed to BraggScattererComposite, the
 * composite also declares this property and tries to propagate the value
 * assigned to the composite's property to all its members.
 *
 * @param propertyName :: Name of the parameter that should be exposed.
 */
void
BraggScatterer::exposePropertyToComposite(const std::string &propertyName) {
  setPropertyGroup(propertyName, m_propagatingGroupName);
}

/// Removes exposure to composite for specified property.
void
BraggScatterer::unexposePropertyFromComposite(const std::string &propertyName) {
  setPropertyGroup(propertyName, "");
}

/// Returns the group name that is used to mark properties that are propagated.
const std::string &BraggScatterer::getPropagatingGroupName() const {
  return m_propagatingGroupName;
}

} // namespace Geometry
} // namespace Mantid
