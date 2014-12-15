#include "MantidGeometry/Crystal/PointGroupFactory.h"

#include "MantidKernel/LibraryManager.h"
#include <boost/algorithm/string.hpp>


namespace Mantid
{
namespace Geometry
{

/// Creates a PointGroup object from its Hermann-Mauguin symbol.
PointGroup_sptr PointGroupFactoryImpl::createPointGroup(const std::string &hmSymbol) const
{
    PointGroup_sptr pointGroup = create(hmSymbol);
    pointGroup->init();

    return pointGroup;
}

PointGroup_sptr PointGroupFactoryImpl::createPointGroupFromSpaceGroupSymbol(const std::string &spaceGroupSymbol) const
{
    return createPointGroup(pointGroupSymbolFromSpaceGroupSymbol(spaceGroupSymbol));
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

/**
 * Returns the point group symbol from a given space group symbol
 *
 * This method exploits the direct relationship between point and
 * space groups. Point groups don't have translational symmetry, which
 * is reflected in the symbol as well. To get the symbol of the point group
 * a certain space group belongs to, some simple string replacements are enough:
 *  1. Replace screw axes ( (2|3|4|6)[1|2|3|5] ) with rotations (first number).
 *  2. Replace glide planes (a|b|c|d|e|g|n) with mirror planes (m)
 *  3. Remove centering symbol.
 *  4. Remove origin choice ( :(1|2) )
 * The two different possibilities in monoclinic are handled explicitly.
 *
 * @param spaceGroupSymbol :: Space group symbol
 * @return
 */
std::string PointGroupFactoryImpl::pointGroupSymbolFromSpaceGroupSymbol(const std::string &spaceGroupSymbol) const
{
    std::string noOriginChoice = boost::regex_replace(spaceGroupSymbol, m_originChoiceRegex, "");
    std::string noScrews = boost::regex_replace(noOriginChoice, m_screwAxisRegex, "\\1");
    std::string noGlides = boost::regex_replace(noScrews, m_glidePlaneRegex, "m");
    std::string noCentering = boost::regex_replace(noGlides, m_centeringRegex, "");

    std::string noSpaces = boost::algorithm::erase_all_copy(noCentering, " ");

    if(noSpaces.substr(0, 1) == "1" && noSpaces.substr(noSpaces.size() - 1, 1) == "1") {
        noSpaces = noSpaces.substr(1, noSpaces.size() - 2);
    }

    return noSpaces;
}

/// Private default constructor.
PointGroupFactoryImpl::PointGroupFactoryImpl() : Kernel::DynamicFactory<PointGroup>(),
    m_crystalSystemMap(),
    m_screwAxisRegex("(2|3|4|6)[1|2|3|5]"),
    m_glidePlaneRegex("a|b|c|d|e|g|n"),
    m_centeringRegex("[A-Z]"),
    m_originChoiceRegex(":(1|2)")
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
