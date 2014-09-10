#include "MantidGeometry/Crystal/PointGroupFactory.h"

#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Geometry
{

/// Creates a PointGroup object from its Hermann-Mauguin symbol.
PointGroup_sptr PointGroupFactoryImpl::createPointGroup(const std::string &hmSymbol) const
{
    return create(hmSymbol);
}

/// Returns the Hermann-Mauguin symbols of all registered point groups.
std::vector<std::string> PointGroupFactoryImpl::getAllPointGroupSymbols() const
{
    std::vector<std::string> pointGroups;

    for(auto it = m_crystalSystemMap.begin(); it != m_crystalSystemMap.end(); ++it) {
        pointGroups.push_back(it->first);
    }

    return pointGroups;
}

/// Returns the Hermann-Mauguin symbols of all point groups that belong to a certain crystal system.
std::vector<std::string> PointGroupFactoryImpl::getPointGroupSymbols(const PointGroup::CrystalSystem &crystalSystem) const
{
    std::vector<std::string> pointGroups;

    for(auto it = m_crystalSystemMap.begin(); it != m_crystalSystemMap.end(); ++it) {
        if(it->second == crystalSystem) {
            pointGroups.push_back(it->first);
        }
    }

    return pointGroups;
}

/// Private default constructor.
PointGroupFactoryImpl::PointGroupFactoryImpl() : Kernel::DynamicFactory<PointGroup>(),
    m_crystalSystemMap()
{
    Kernel::LibraryManager::Instance();
}

/// Adds a point group to a map that stores pairs of Hermann-Mauguin symbol and crystal system.
void PointGroupFactoryImpl::addToCrystalSystemMap(const PointGroup::CrystalSystem &crystalSystem, const std::string &hmSymbol)
{
    m_crystalSystemMap.insert(std::make_pair(hmSymbol, crystalSystem));
}

/// Removes point group from internal crystal system map.
void PointGroupFactoryImpl::removeFromCrystalSystemMap(const std::string &hmSymbol)
{
    auto it = m_crystalSystemMap.find(hmSymbol);
    m_crystalSystemMap.erase(it);
}



} // namespace Geometry
} // namespace Mantid
