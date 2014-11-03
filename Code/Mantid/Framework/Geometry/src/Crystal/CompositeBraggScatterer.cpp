#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{

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
    clone->setProperties(this->asString(false, ';'));

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        clone->addScatterer(*it);
    }

    return clone;
}

/// Clones the supplied scatterer, assigns the internal space group and unit cell to the clone and adds it to the composite.
void CompositeBraggScatterer::addScatterer(const BraggScatterer_sptr &scatterer)
{
    if(!scatterer) {
        throw std::invalid_argument("Cannot process null-scatterer.");
    }

    BraggScatterer_sptr localScatterer = scatterer->clone();

    setCommonProperties(localScatterer);

    m_scatterers.push_back(localScatterer);
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
void CompositeBraggScatterer::afterScattererPropertySet(const std::string &propertyName)
{
    if(propertyName == "SpaceGroup" || propertyName == "UnitCell") {
        propagateProperty(propertyName);
    }
}

/// Propagates the given property to all contained scatterers.
void CompositeBraggScatterer::propagateProperty(const std::string &propertyName)
{
    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setProperty(propertyName, getPropertyValue(propertyName));
    }
}

/// Assigns the stored cell and space group to the supplied scatterer.
void CompositeBraggScatterer::setCommonProperties(BraggScatterer_sptr &scatterer)
{
    scatterer->setProperty("UnitCell", getPropertyValue("UnitCell"));

    if(getSpaceGroup()) {
        scatterer->setProperty("SpaceGroup", getPropertyValue("SpaceGroup"));
    }
}

DECLARE_BRAGGSCATTERER(CompositeBraggScatterer)

} // namespace Geometry
} // namespace Mantid
