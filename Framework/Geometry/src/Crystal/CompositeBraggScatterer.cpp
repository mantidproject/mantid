// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include <stdexcept>

namespace Mantid::Geometry {

using namespace Kernel;

/// Default constructor.
CompositeBraggScatterer::CompositeBraggScatterer() : BraggScatterer(), m_scatterers() {}

/// Static method that creates a new instance of CompositeBraggScatterer and
/// returns it (wrapped by a smart pointer).
CompositeBraggScatterer_sptr CompositeBraggScatterer::create() {
  CompositeBraggScatterer_sptr compositeScatterer = std::make_shared<CompositeBraggScatterer>();
  compositeScatterer->initialize();

  return compositeScatterer;
}

/// Creates and empty CompositeBraggScatterer and adds all scatterers contained
/// in the supplied vector.
CompositeBraggScatterer_sptr CompositeBraggScatterer::create(const std::vector<BraggScatterer_sptr> &scatterers) {
  CompositeBraggScatterer_sptr collection = CompositeBraggScatterer::create();

  collection->setScatterers(scatterers);

  return collection;
}

/// Recursively clones all contained scatterers and returns the resulting
/// composite.
BraggScatterer_sptr CompositeBraggScatterer::clone() const {
  CompositeBraggScatterer_sptr clone = std::make_shared<CompositeBraggScatterer>();
  clone->initialize();

  clone->setScatterers(m_scatterers);

  clone->setProperties(this->asString(false));

  return clone;
}

/// Clones the supplied scatterer, assigns the internal space group and unit
/// cell to the clone and adds it to the composite.
void CompositeBraggScatterer::addScatterer(const BraggScatterer_sptr &scatterer) {
  addScattererImplementation(scatterer);
  redeclareProperties();
}

/// Adds scatterers and assigns clones of the supplied ones.
void CompositeBraggScatterer::addScatterers(const std::vector<BraggScatterer_sptr> &scatterers) {
  for (const auto &scatterer : scatterers) {
    addScattererImplementation(scatterer);
  }
  redeclareProperties();
}

/// Clears all scatterers and assigns clones of the supplied ones.
void CompositeBraggScatterer::setScatterers(const std::vector<BraggScatterer_sptr> &scatterers) {
  removeAllScatterers();
  addScatterers(scatterers);
}

/// Returns the number of scatterers contained in the composite.
size_t CompositeBraggScatterer::nScatterers() const { return m_scatterers.size(); }

/// Returns the i-th scatterer or throws an std::out_of_range exception.
BraggScatterer_sptr CompositeBraggScatterer::getScatterer(size_t i) const {
  if (i >= nScatterers()) {
    throw std::out_of_range("Index is out of range.");
  }

  return m_scatterers[i];
}

/// Returns the scatterers.
const std::vector<BraggScatterer_sptr> &CompositeBraggScatterer::getScatterers() const { return m_scatterers; }

/// Removes the i-th scatterer from the composite or throws an std::out_of_range
/// exception.
void CompositeBraggScatterer::removeScatterer(size_t i) {
  removeScattererImplementation(i);

  redeclareProperties();
}

/// This method performs the actual removal of the i-th scatterer.
void CompositeBraggScatterer::removeScattererImplementation(size_t i) {
  if (i >= nScatterers()) {
    throw std::out_of_range("Index is out of range.");
  }

  m_scatterers.erase(m_scatterers.begin() + i);
}

/// Removes all scatterers.
void CompositeBraggScatterer::removeAllScatterers() {
  while (nScatterers() > 0) {
    removeScattererImplementation(0);
  }

  redeclareProperties();
}

/// Calculates the structure factor for the given HKL by summing all
/// contributions from contained scatterers.
StructureFactor CompositeBraggScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const {
  return std::accumulate(
      m_scatterers.cbegin(), m_scatterers.cend(), StructureFactor(0., 0.),
      [&hkl](const auto &sum, const auto &scatterer) { return sum + scatterer->calculateStructureFactor(hkl); });
  ;
}

/// Makes sure that space group and unit cell are propagated to all stored
/// scatterers.
void CompositeBraggScatterer::afterPropertySet(const std::string &propertyName) { propagateProperty(propertyName); }

/// Propagates the given property to all contained scatterers that have this
/// property.
void CompositeBraggScatterer::propagateProperty(const std::string &propertyName) {
  std::string propertyValue = getPropertyValue(propertyName);

  for (auto &scatterer : m_scatterers) {
    propagatePropertyToScatterer(scatterer, propertyName, propertyValue);
  }
}

void CompositeBraggScatterer::propagatePropertyToScatterer(BraggScatterer_sptr &scatterer,
                                                           const std::string &propertyName,
                                                           const std::string &propertyValue) {
  try {
    scatterer->setPropertyValue(propertyName, propertyValue);
  } catch (const Kernel::Exception::NotFoundError &) {
    // do nothing.
  }
}

/// This method performs the actual cloning and adding of a new scatterer.
void CompositeBraggScatterer::addScattererImplementation(const BraggScatterer_sptr &scatterer) {
  if (!scatterer) {
    throw std::invalid_argument("Cannot process null-scatterer.");
  }

  BraggScatterer_sptr localScatterer = scatterer->clone();
  m_scatterers.emplace_back(localScatterer);
}

/**
 * Synchronize properties with scatterer members
 *
 * This method synchronizes the properties of CompositeBraggScatterer with the
 * properties of the contained BraggScatterer instances. It adds new properties
 * if required and removed properties that are no longer used (for example
 * because the member that introduced the property has been removed).
 */
void CompositeBraggScatterer::redeclareProperties() {
  std::map<std::string, size_t> propertyUseCount = getPropertyCountMap();

  for (auto &scatterer : m_scatterers) {
    // Check if any of the declared properties is in this scatterer (and set
    // value if that's the case)
    for (auto &prop : propertyUseCount) {
      if (scatterer->existsProperty(prop.first)) {
        prop.second += 1;

        propagatePropertyToScatterer(scatterer, prop.first, getPropertyValue(prop.first));
      }
    }

    // Use the properties of this scatterer which have been marked as exposed to
    // composite
    std::vector<Property *> properties = scatterer->getPropertiesInGroup(getPropagatingGroupName());
    for (auto &property : properties) {
      const std::string &propertyName = property->name();
      if (!existsProperty(propertyName)) {
        declareProperty(std::unique_ptr<Property>(property->clone()));
      }
    }
  }

  // Remove unused properties
  for (auto &property : propertyUseCount) {
    if (property.second == 0) {
      removeProperty(property.first);
    }
  }
}

/// Returns a map with all declared property names and 0.
std::map<std::string, size_t> CompositeBraggScatterer::getPropertyCountMap() const {
  std::map<std::string, size_t> propertyUseCount;

  std::vector<Property *> compositeProperties = getProperties();
  for (auto &compositeProperty : compositeProperties) {
    propertyUseCount.emplace(compositeProperty->name(), 0);
  }
  return propertyUseCount;
}

DECLARE_BRAGGSCATTERER(CompositeBraggScatterer)

} // namespace Mantid::Geometry
