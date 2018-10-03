// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"

#include "MantidGeometry/Crystal/ProductOfCyclicGroups.h"
#include "MantidKernel/LibraryManager.h"
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace Geometry {

/// Creates a PointGroup object from its Hermann-Mauguin symbol.
PointGroup_sptr
PointGroupFactoryImpl::createPointGroup(const std::string &hmSymbol) {
  if (!isSubscribed(hmSymbol)) {
    throw std::invalid_argument("Point group with symbol '" + hmSymbol +
                                "' is not registered.");
  }

  return constructFromPrototype(getPrototype(hmSymbol));
}

PointGroup_sptr PointGroupFactoryImpl::createPointGroupFromSpaceGroup(
    const SpaceGroup_const_sptr &spaceGroup) {
  return createPointGroupFromSpaceGroup(*spaceGroup);
}

PointGroup_sptr PointGroupFactoryImpl::createPointGroupFromSpaceGroup(
    const SpaceGroup &spaceGroup) {
  std::string pointGroupSymbol =
      pointGroupSymbolFromSpaceGroupSymbol(spaceGroup.hmSymbol());

  try {
    PointGroup_sptr pointGroup = createPointGroup(pointGroupSymbol);

    // If the crystal system is trigonal, we need to do more.
    if (pointGroup->crystalSystem() == PointGroup::CrystalSystem::Trigonal) {
      throw std::invalid_argument(
          "Trigonal space groups need to be processed differently.");
    }

    return pointGroup;
  } catch (const std::invalid_argument &) {
    if (spaceGroup.getCoordinateSystem() !=
        Group::CoordinateSystem::Hexagonal) {
      pointGroupSymbol.append(" r");
    }

    return createPointGroup(pointGroupSymbol);
  }
}

bool PointGroupFactoryImpl::isSubscribed(const std::string &hmSymbol) const {
  return m_generatorMap.find(hmSymbol) != m_generatorMap.end();
}

/// Returns the Hermann-Mauguin symbols of all registered point groups.
std::vector<std::string>
PointGroupFactoryImpl::getAllPointGroupSymbols() const {
  std::vector<std::string> pointGroups;
  pointGroups.reserve(m_generatorMap.size());
  for (const auto &generator : m_generatorMap) {
    pointGroups.push_back(generator.first);
  }

  return pointGroups;
}

/// Returns the Hermann-Mauguin symbols of all point groups that belong to a
/// certain crystal system.
std::vector<std::string> PointGroupFactoryImpl::getPointGroupSymbols(
    const PointGroup::CrystalSystem &crystalSystem) {
  std::vector<std::string> pointGroups;

  for (auto &generator : m_generatorMap) {
    PointGroup_sptr pointGroup = getPrototype(generator.first);

    if (pointGroup->crystalSystem() == crystalSystem) {
      pointGroups.push_back(generator.first);
    }
  }

  return pointGroups;
}

void PointGroupFactoryImpl::subscribePointGroup(
    const std::string &hmSymbol, const std::string &generatorString,
    const std::string &description) {
  if (isSubscribed(hmSymbol)) {
    throw std::invalid_argument(
        "Point group with this symbol is already registered.");
  }

  PointGroupGenerator_sptr generator = boost::make_shared<PointGroupGenerator>(
      hmSymbol, generatorString, description);

  subscribe(generator);
}

/**
 * Returns the point group symbol from a given space group symbol
 *
 * This method exploits the direct relationship between point and
 * space groups. Point groups don't have translational symmetry, which
 * is reflected in the symbol as well. To get the symbol of the point group
 * a certain space group belongs to, some simple string replacements are enough:
 *  1. Replace screw axes ( (2|3|4|6)[1|2|3|4|5] ) with rotations (first
 *number).
 *  2. Replace glide planes (a|b|c|d|e|g|n) with mirror planes (m)
 *  3. Remove centering symbol.
 *  4. Remove origin choice ( :(1|2) )
 * The two different possibilities in monoclinic are handled explicitly.
 *
 * @param spaceGroupSymbol :: Space group symbol
 * @return
 */
std::string PointGroupFactoryImpl::pointGroupSymbolFromSpaceGroupSymbol(
    const std::string &spaceGroupSymbol) const {
  std::string noOriginChoice =
      boost::regex_replace(spaceGroupSymbol, m_originChoiceRegex, "");
  std::string noScrews =
      boost::regex_replace(noOriginChoice, m_screwAxisRegex, "\\1");
  std::string noGlides = boost::regex_replace(noScrews, m_glidePlaneRegex, "m");
  std::string noCentering =
      boost::regex_replace(noGlides, m_centeringRegex, "");

  std::string noSpaces = boost::algorithm::erase_all_copy(noCentering, " ");

  if (noSpaces.substr(0, 1) == "1" && noSpaces.size() > 2 &&
      noSpaces.substr(noSpaces.size() - 1, 1) == "1") {
    noSpaces = noSpaces.substr(1, noSpaces.size() - 2);
  }

  return noSpaces;
}

PointGroup_sptr
PointGroupFactoryImpl::getPrototype(const std::string &hmSymbol) {
  PointGroupGenerator_sptr generator = m_generatorMap.find(hmSymbol)->second;

  if (!generator) {
    throw std::runtime_error("No generator for symbol '" + hmSymbol + "'");
  }

  return generator->getPrototype();
}

void PointGroupFactoryImpl::subscribe(
    const PointGroupGenerator_sptr &generator) {
  if (!generator) {
    throw std::runtime_error("Cannot register null-generator.");
  }

  m_generatorMap.emplace(generator->getHMSymbol(), generator);
}

PointGroup_sptr PointGroupFactoryImpl::constructFromPrototype(
    const PointGroup_sptr &prototype) const {
  return boost::make_shared<PointGroup>(*prototype);
}

/// Private default constructor.
PointGroupFactoryImpl::PointGroupFactoryImpl()
    : m_generatorMap(), m_crystalSystemMap(),
      m_screwAxisRegex("(2|3|4|6)[1|2|3|4|5]"),
      m_glidePlaneRegex("a|b|c|d|e|g|n"), m_centeringRegex("[A-Z]"),
      m_originChoiceRegex(":(1|2|r)") {
  Kernel::LibraryManager::Instance();
}

PointGroupGenerator::PointGroupGenerator(
    const std::string &hmSymbol, const std::string &generatorInformation,
    const std::string &description)
    : m_hmSymbol(hmSymbol), m_generatorString(generatorInformation),
      m_description(description) {}

PointGroup_sptr PointGroupGenerator::getPrototype() {
  if (!hasValidPrototype()) {
    m_prototype = generatePrototype();
  }

  return m_prototype;
}

PointGroup_sptr PointGroupGenerator::generatePrototype() {
  Group_const_sptr generatingGroup =
      GroupFactory::create<ProductOfCyclicGroups>(m_generatorString);

  if (!generatingGroup) {
    throw std::runtime_error(
        "Could not create group from supplied symmetry operations.");
  }

  return boost::make_shared<PointGroup>(m_hmSymbol, *generatingGroup,
                                        m_description);
}

DECLARE_POINTGROUP("1", "x,y,z", "Triclinic")
DECLARE_POINTGROUP("-1", "-x,-y,-z", "Triclinic")
DECLARE_POINTGROUP("2", "-x,y,-z", "Monoclinic, unique axis b")
DECLARE_POINTGROUP("112", "-x,-y,z", "Monoclinic, unique axis c")
DECLARE_POINTGROUP("m", "x,-y,z", "Monoclinic, unique axis b")
DECLARE_POINTGROUP("11m", "x,y,-z", "Monoclinic, unique axis c")
DECLARE_POINTGROUP("2/m", "-x,y,-z; -x,-y,-z", "Monoclinic, unique axis b")
DECLARE_POINTGROUP("112/m", "-x,-y,z; x,y,-z", "Monoclinic, unique axis c")
DECLARE_POINTGROUP("222", "-x,-y,z; x,-y,-z", "Orthorhombic")
DECLARE_POINTGROUP("mm2", "-x,-y,z; -x,y,z", "Orthorhombic")
DECLARE_POINTGROUP("2mm", "x,-y,-z; x,-y,z", "Orthorhombic")
DECLARE_POINTGROUP("m2m", "-x,y,-z; x,y,-z", "Orthorhombic")
DECLARE_POINTGROUP("mmm", "-x,-y,-z; -x,-y,z; x,-y,-z", "Orthorhombic")
DECLARE_POINTGROUP("4", "-y,x,z", "Tetragonal")
DECLARE_POINTGROUP("-4", "y,-x,-z", "Tetragonal")
DECLARE_POINTGROUP("4/m", "-y,x,z; -x,-y,-z", "Tetragonal")
DECLARE_POINTGROUP("422", "-y,x,z; x,-y,-z", "Tetragonal")
DECLARE_POINTGROUP("4mm", "-y,x,z; -x,y,z", "Tetragonal")
DECLARE_POINTGROUP("-42m", "y,-x,-z; x,-y,-z", "Tetragonal")
DECLARE_POINTGROUP("-4m2", "y,-x,-z; y,x,-z", "Tetragonal")
DECLARE_POINTGROUP("4/mmm", "-y,x,z; x,y,-z; x,-y,-z", "Tetragonal")

DECLARE_POINTGROUP("3", "-y,x-y,z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("-3", "y,y-x,-z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("321", "-y,x-y,z; x-y,-y,-z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("32", "-y,x-y,z; x-y,-y,-z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("312", "-y,x-y,z; x,x-y,-z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("3m1", "-y,x-y,z; y-x,y,z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("3m", "-y,x-y,z; y-x,y,z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("31m", "-y,x-y,z; -x,y-x,z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("-3m1", "y,y-x,-z; x-y,-y,-z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("-3m", "y,y-x,-z; x-y,-y,-z", "Trigonal - Hexagonal")
DECLARE_POINTGROUP("-31m", "y,y-x,-z; x,x-y,-z", "Trigonal - Hexagonal")

DECLARE_POINTGROUP("3 r", "z,x,y", "Trigonal - Rhombohedral")
DECLARE_POINTGROUP("-3 r", "-z,-x,-y", "Trigonal - Rhombohedral")
DECLARE_POINTGROUP("32 r", "z,x,y; -y,-x,-z", "Trigonal - Rhombohedral")
DECLARE_POINTGROUP("3m r", "z,x,y; y,x,z", "Trigonal - Rhombohedral")
DECLARE_POINTGROUP("-3m r", "-z,-x,-y; y,x,z", "Trigonal - Rhombohedral")

DECLARE_POINTGROUP("6", "x-y,x,z", "Hexagonal")
DECLARE_POINTGROUP("-6", "y-x,-x,-z", "Hexagonal")
DECLARE_POINTGROUP("6/m", "x-y,x,z; -x,-y,-z", "Hexagonal")
DECLARE_POINTGROUP("622", "x-y,x,z; x-y,-y,-z", "Hexagonal")
DECLARE_POINTGROUP("6mm", "x-y,x,z; y-x,y,z", "Hexagonal")
DECLARE_POINTGROUP("-62m", "y-x,-x,-z; x-y,-y,-z", "Hexagonal")
DECLARE_POINTGROUP("-6m2", "y-x,-x,-z; y-x,y,z", "Hexagonal")
DECLARE_POINTGROUP("6/mmm", "x-y,x,z; x-y,-y,-z; -x,-y,-z", "Hexagonal")

DECLARE_POINTGROUP("23", "z,x,y; -x,-y,z; x,-y,-z", "Cubic")
DECLARE_POINTGROUP("m-3", "-z,-x,-y; -x,-y,z; x,-y,-z", "Cubic")
DECLARE_POINTGROUP("432", "z,x,y; -y,x,z; x,-y,-z", "Cubic")
DECLARE_POINTGROUP("-43m", "z,x,y; y,-x,-z; -y,-x,z", "Cubic")
DECLARE_POINTGROUP("m-3m", "-z,-x,-y; -y,x,z; y,x,-z", "Cubic")

} // namespace Geometry
} // namespace Mantid
