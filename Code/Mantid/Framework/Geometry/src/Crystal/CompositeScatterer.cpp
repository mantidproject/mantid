#include "MantidGeometry/Crystal/CompositeScatterer.h"
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{

/// Default constructor.
CompositeScatterer::CompositeScatterer() :
    IScatterer(),
    m_scatterers()
{
}

/// Static method that creates a new instance of CompositeScatterer and returns it (wrapped by a smart pointer).
CompositeScatterer_sptr CompositeScatterer::create()
{
    return boost::make_shared<CompositeScatterer>();
}

/// Creates and empty CompositeScatterer and adds all scatterers contained in the supplied vector.
CompositeScatterer_sptr CompositeScatterer::create(const std::vector<IScatterer_sptr> &scatterers)
{
    CompositeScatterer_sptr collection = CompositeScatterer::create();

    for(auto it = scatterers.begin(); it != scatterers.end(); ++it) {
        collection->addScatterer(*it);
    }

    return collection;
}

/// Recursively clones all contained scatterers and returns the resulting composite.
IScatterer_sptr CompositeScatterer::clone() const
{
    boost::shared_ptr<CompositeScatterer> clone = boost::make_shared<CompositeScatterer>();
    clone->setPosition(getPosition());
    clone->setCell(getCell());
    clone->setSpaceGroup(getSpaceGroup());

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        clone->addScatterer(*it);
    }

    return clone;
}

/// Assigns the unit cell and propagates it to all contained scatterers.
void CompositeScatterer::setCell(const UnitCell &cell)
{
    IScatterer::setCell(cell);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setCell(cell);
    }
}

/// Assigns the space group and propagates it to all contained scatterers.
void CompositeScatterer::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup)
{
    IScatterer::setSpaceGroup(spaceGroup);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setSpaceGroup(spaceGroup);
    }
}

/// Clones the supplied scatterer, assigns the internal space group and unit cell to the clone and adds it to the composite.
void CompositeScatterer::addScatterer(const IScatterer_sptr &scatterer)
{
    if(!scatterer) {
        throw std::invalid_argument("Cannot process null-scatterer.");
    }

    IScatterer_sptr localScatterer = scatterer->clone();

    setCommonProperties(localScatterer);

    m_scatterers.push_back(localScatterer);
}

/// Returns the number of scatterers contained in the composite.
size_t CompositeScatterer::nScatterers() const
{
    return m_scatterers.size();
}

/// Returns the i-th scatterer or throws an std::out_of_range exception.
IScatterer_sptr CompositeScatterer::getScatterer(size_t i) const
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    return m_scatterers[i];
}

/// Removes the i-th scatterer from the composite or throws an std::out_of_range exception.
void CompositeScatterer::removeScatterer(size_t i)
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    m_scatterers.erase(m_scatterers.begin() + i);
}

/// Calculates the structure factor for the given HKL by summing all contributions from contained scatterers.
StructureFactor CompositeScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    StructureFactor sum(0.0, 0.0);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        sum += (*it)->calculateStructureFactor(hkl);
    }

    return sum;
}

/// Assigns the stored cell and space group to the supplied scatterer.
void CompositeScatterer::setCommonProperties(IScatterer_sptr &scatterer)
{
    scatterer->setCell(getCell());
    scatterer->setSpaceGroup(getSpaceGroup());
}



} // namespace Geometry
} // namespace Mantid
