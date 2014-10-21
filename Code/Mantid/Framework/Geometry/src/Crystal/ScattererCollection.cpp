#include "MantidGeometry/Crystal/ScattererCollection.h"

namespace Mantid
{
namespace Geometry
{
ScattererCollection::ScattererCollection() :
    IScatterer(),
    m_scatterers()
{
}

ScattererCollection_sptr ScattererCollection::create()
{
    return boost::make_shared<ScattererCollection>();
}

ScattererCollection_sptr ScattererCollection::create(const std::vector<IScatterer_sptr> &scatterers)
{
    ScattererCollection_sptr collection = ScattererCollection::create();

    for(auto it = scatterers.begin(); it != scatterers.end(); ++it) {
        collection->addScatterer(*it);
    }

    return collection;
}

IScatterer_sptr ScattererCollection::clone() const
{
    boost::shared_ptr<ScattererCollection> clone = boost::make_shared<ScattererCollection>();
    clone->setPosition(getPosition());
    clone->setCell(getCell());
    clone->setSpaceGroup(getSpaceGroup());

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        clone->addScatterer(*it);
    }

    return clone;
}

void ScattererCollection::setCell(const UnitCell &cell)
{
    IScatterer::setCell(cell);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setCell(cell);
    }
}

void ScattererCollection::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup)
{
    IScatterer::setSpaceGroup(spaceGroup);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setSpaceGroup(spaceGroup);
    }
}

void ScattererCollection::addScatterer(const IScatterer_sptr &scatterer)
{
    if(!scatterer) {
        throw std::invalid_argument("Cannot process null-scatterer.");
    }

    IScatterer_sptr localScatterer = scatterer->clone();

    setCommonProperties(localScatterer);

    m_scatterers.push_back(localScatterer);
}

size_t ScattererCollection::nScatterers() const
{
    return m_scatterers.size();
}

IScatterer_sptr ScattererCollection::getScatterer(size_t i) const
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    return m_scatterers[i];
}

void ScattererCollection::removeScatterer(size_t i)
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    m_scatterers.erase(m_scatterers.begin() + i);
}

StructureFactor ScattererCollection::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    StructureFactor sum(0.0, 0.0);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        sum += (*it)->calculateStructureFactor(hkl);
    }

    return sum;
}

void ScattererCollection::setCommonProperties(IScatterer_sptr &scatterer)
{
    scatterer->setCell(getCell());
    scatterer->setSpaceGroup(getSpaceGroup());
}



} // namespace Geometry
} // namespace Mantid
