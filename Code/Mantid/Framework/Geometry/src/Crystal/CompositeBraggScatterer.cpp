#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{

using namespace Kernel;

/// Default constructor.
CompositeBraggScatterer::CompositeBraggScatterer() :
    BraggScatterer(),
    m_scatterers()
{
}

/// Static method that creates a new instance of CompositeBraggScatterer and returns it (wrapped by a smart pointer).
CompositeBraggScatterer_sptr CompositeBraggScatterer::create()
{
    CompositeBraggScatterer_sptr compositeScatterer = boost::make_shared<CompositeBraggScatterer>();
    compositeScatterer->initialize();

    return compositeScatterer;
}

/// Creates and empty CompositeBraggScatterer and adds all scatterers contained in the supplied vector.
CompositeBraggScatterer_sptr CompositeBraggScatterer::create(const std::vector<BraggScatterer_sptr> &scatterers)
{
    CompositeBraggScatterer_sptr collection = CompositeBraggScatterer::create();

    for(auto it = scatterers.begin(); it != scatterers.end(); ++it) {
        collection->addScatterer(*it);
    }

    return collection;
}

/// Recursively clones all contained scatterers and returns the resulting composite.
BraggScatterer_sptr CompositeBraggScatterer::clone() const
{
    CompositeBraggScatterer_sptr clone = boost::make_shared<CompositeBraggScatterer>();
    clone->initialize();    

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        clone->addScatterer(*it);
    }

    clone->setProperties(this->asString(false, ';'));

    return clone;
}

/// Clones the supplied scatterer, assigns the internal space group and unit cell to the clone and adds it to the composite.
void CompositeBraggScatterer::addScatterer(const BraggScatterer_sptr &scatterer)
{
    if(!scatterer) {
        throw std::invalid_argument("Cannot process null-scatterer.");
    }

    BraggScatterer_sptr localScatterer = scatterer->clone();

    m_scatterers.push_back(localScatterer);

    redeclareProperties();
}

/// Returns the number of scatterers contained in the composite.
size_t CompositeBraggScatterer::nScatterers() const
{
    return m_scatterers.size();
}

/// Returns the i-th scatterer or throws an std::out_of_range exception.
BraggScatterer_sptr CompositeBraggScatterer::getScatterer(size_t i) const
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    return m_scatterers[i];
}

/// Removes the i-th scatterer from the composite or throws an std::out_of_range exception.
void CompositeBraggScatterer::removeScatterer(size_t i)
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    m_scatterers.erase(m_scatterers.begin() + i);

    redeclareProperties();
}

/// Removes all scatterers.
void CompositeBraggScatterer::removeAllScatterers()
{
    while(nScatterers() > 0) {
        removeScatterer(0);
    }
}

/// Calculates the structure factor for the given HKL by summing all contributions from contained scatterers.
StructureFactor CompositeBraggScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    StructureFactor sum(0.0, 0.0);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        sum += (*it)->calculateStructureFactor(hkl);
    }

    return sum;
}

/// Makes sure that space group and unit cell are propagated to all stored scatterers.
void CompositeBraggScatterer::afterPropertySet(const std::string &propertyName)
{
    propagateProperty(propertyName);
}

/// Propagates the given property to all contained scatterers that have this property.
void CompositeBraggScatterer::propagateProperty(const std::string &propertyName)
{
    std::string propertyValue = getPropertyValue(propertyName);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        propagatePropertyToScatterer(*it, propertyName, propertyValue);
    }
}

void CompositeBraggScatterer::propagatePropertyToScatterer(BraggScatterer_sptr &scatterer, const std::string &propertyName, const std::string &propertyValue)
{
    try {
        scatterer->setPropertyValue(propertyName, propertyValue);
    } catch(Kernel::Exception::NotFoundError) {
        // do nothing.
    }
}

/**
 * Synchronize properties with scatterer members
 *
 * This method synchronizes the properties of CompositeBraggScatterer with the properties
 * of the contained BraggScatterer instances. It adds new properties if required
 * and removed properties that are no longer used (for example because the member that
 * introduced the property has been removed).
 */
void CompositeBraggScatterer::redeclareProperties()
{
    std::map<std::string, size_t> propertyUseCount = getPropertyCountMap();

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        // Check if any of the declared properties is in this scatterer (and set value if that's the case)
        for(auto prop = propertyUseCount.begin(); prop != propertyUseCount.end(); ++prop) {
            if((*it)->existsProperty(prop->first)) {
                prop->second += 1;

                propagatePropertyToScatterer(*it, prop->first, getPropertyValue(prop->first));
            }
        }

        // Use the properties of this scatterer which have been marked as exposed to composite
        std::vector<Property *> properties = (*it)->getPropertiesInGroup(getPropagatingGroupName());
        for(auto prop = properties.begin(); prop != properties.end(); ++prop) {
            std::string propertyName = (*prop)->name();
            if(!existsProperty(propertyName)) {
                declareProperty((*prop)->clone());
            }
        }
    }

    // Remove unused properties
    for(auto it = propertyUseCount.begin(); it != propertyUseCount.end(); ++it) {
        if(it->second == 0) {
            removeProperty(it->first);
        }
    }
}

/// Returns a map with all declared property names and 0.
std::map<std::string, size_t> CompositeBraggScatterer::getPropertyCountMap() const
{
    std::map<std::string, size_t> propertyUseCount;

    std::vector<Property *> compositeProperties = getProperties();
    for(auto it = compositeProperties.begin(); it != compositeProperties.end(); ++it) {
        propertyUseCount.insert(std::make_pair((*it)->name(), 0));
    }

    return propertyUseCount;
}

DECLARE_BRAGGSCATTERER(CompositeBraggScatterer)

} // namespace Geometry
} // namespace Mantid
