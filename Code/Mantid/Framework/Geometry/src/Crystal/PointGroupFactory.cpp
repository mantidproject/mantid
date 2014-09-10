#include "MantidGeometry/Crystal/PointGroupFactory.h"

#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Geometry
{

PointGroup_sptr PointGroupFactoryImpl::createPointgroup(const std::string &hmSymbol) const
{
    return create(hmSymbol);
}

std::vector<std::string> PointGroupFactoryImpl::getAllPointGroupSymbols() const
{
    std::vector<std::string> pointGroups;

    for(auto it = m_crystalSystemMap.begin(); it != m_crystalSystemMap.end(); ++it) {
        pointGroups.push_back(it->first);
    }

    return pointGroups;
}

std::vector<std::string> PointGroupFactoryImpl::getAllPointGroupSymbols(const PointGroup::CrystalSystem &crystalSystem) const
{
    std::vector<std::string> pointGroups;

    for(auto it = m_crystalSystemMap.begin(); it != m_crystalSystemMap.end(); ++it) {
        if(it->second == crystalSystem) {
            pointGroups.push_back(it->first);
        }
    }

    return pointGroups;
}

PointGroupFactoryImpl::PointGroupFactoryImpl() : Kernel::DynamicFactory<PointGroup>()
{
    Kernel::LibraryManager::Instance();
}

void PointGroupFactoryImpl::addToCrystalSystemMap(const PointGroup::CrystalSystem &crystalSystem, const std::string &hmSymbol)
{
    m_crystalSystemMap.insert(std::make_pair(hmSymbol, crystalSystem));
}

void PointGroupFactoryImpl::removeFromCrystalSystemMap(const std::string &hmSymbol)
{
    auto it = m_crystalSystemMap.find(hmSymbol);
    m_crystalSystemMap.erase(it);
}



} // namespace Geometry
} // namespace Mantid
