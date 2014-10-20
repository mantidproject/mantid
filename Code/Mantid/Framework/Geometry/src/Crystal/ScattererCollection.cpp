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

void ScattererCollection::addScatterer(const IScatterer_sptr &scatterer)
{
    m_scatterers.push_back(scatterer);
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



} // namespace Geometry
} // namespace Mantid
