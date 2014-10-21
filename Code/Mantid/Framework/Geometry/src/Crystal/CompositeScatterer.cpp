#include "MantidGeometry/Crystal/CompositeScatterer.h"

namespace Mantid
{
namespace Geometry
{
CompositeScatterer::CompositeScatterer() :
    IScatterer(),
    m_scatterers()
{
}

CompositeScatterer_sptr CompositeScatterer::create()
{
    return boost::make_shared<CompositeScatterer>();
}

CompositeScatterer_sptr CompositeScatterer::create(const std::vector<IScatterer_sptr> &scatterers)
{
    CompositeScatterer_sptr collection = CompositeScatterer::create();

    for(auto it = scatterers.begin(); it != scatterers.end(); ++it) {
        collection->addScatterer(*it);
    }

    return collection;
}

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

void CompositeScatterer::setCell(const UnitCell &cell)
{
    IScatterer::setCell(cell);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setCell(cell);
    }
}

void CompositeScatterer::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup)
{
    IScatterer::setSpaceGroup(spaceGroup);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        (*it)->setSpaceGroup(spaceGroup);
    }
}

void CompositeScatterer::addScatterer(const IScatterer_sptr &scatterer)
{
    if(!scatterer) {
        throw std::invalid_argument("Cannot process null-scatterer.");
    }

    IScatterer_sptr localScatterer = scatterer->clone();

    setCommonProperties(localScatterer);

    m_scatterers.push_back(localScatterer);
}

size_t CompositeScatterer::nScatterers() const
{
    return m_scatterers.size();
}

IScatterer_sptr CompositeScatterer::getScatterer(size_t i) const
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    return m_scatterers[i];
}

void CompositeScatterer::removeScatterer(size_t i)
{
    if(i >= nScatterers()) {
        throw std::out_of_range("Index is out of range.");
    }

    m_scatterers.erase(m_scatterers.begin() + i);
}

StructureFactor CompositeScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    StructureFactor sum(0.0, 0.0);

    for(auto it = m_scatterers.begin(); it != m_scatterers.end(); ++it) {
        sum += (*it)->calculateStructureFactor(hkl);
    }

    return sum;
}

void CompositeScatterer::setCommonProperties(IScatterer_sptr &scatterer)
{
    scatterer->setCell(getCell());
    scatterer->setSpaceGroup(getSpaceGroup());
}



} // namespace Geometry
} // namespace Mantid
