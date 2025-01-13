// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <set>

namespace Mantid::Geometry {
using Kernel::IntMatrix;
using Kernel::V3D;
namespace {
/// static logger object
Kernel::Logger g_log("PointGroup");
} // namespace

/**
 * Returns all equivalent reflections for the supplied hkl.
 *
 * This method returns a vector containing all equivalent hkls for the supplied
 * one. It depends on the internal state of the pointgroup object (e.g. which
 * symmetry operations and therefore, which transformation matrices are
 * present). This internal state is unique for each concrete point group and
 * is set in the constructor.
 *
 * The returned vector always contains a set of unique hkls, so for special hkls
 * like (100), it has fewer entries than for a general hkl. See also
 * PointGroup::getEquivalentSet.
 *
 * @param hkl :: Arbitrary hkl
 * @return :: std::vector containing all equivalent hkls.
 */
std::vector<V3D> PointGroup::getEquivalents(const V3D &hkl) const {
  auto equivalents = getAllEquivalents(hkl);

  std::sort(equivalents.begin(), equivalents.end(), std::greater<V3D>());

  equivalents.erase(std::unique(equivalents.begin(), equivalents.end()), equivalents.end());

  return equivalents;
}

/**
 * Returns the same V3D for all equivalent hkls.
 *
 * This method is closely related to PointGroup::getEquivalents. It returns the
 * same V3D for all hkls of one "family". For example in a cubic point group
 * it will return (100) for (001), (010), (0-10), etc.
 *
 * It can be used to generate a set of symmetry independent hkls, useful for
 * example in powder diffraction.
 *
 * @param hkl :: Arbitrary hkl
 * @return :: hkl specific to a family of index-triplets
 */
V3D PointGroup::getReflectionFamily(const Kernel::V3D &hkl) const {
  auto equivalents = getAllEquivalents(hkl);

  return *std::max_element(equivalents.begin(), equivalents.end());
}

/// Protected constructor - can not be used directly.
PointGroup::PointGroup(const std::string &symbolHM, const Group &group, const std::string &description)
    : Group(group), m_symbolHM(symbolHM), m_name(symbolHM + " (" + description + ")") {
  m_crystalSystem = getCrystalSystemFromGroup();
  m_latticeSystem = getLatticeSystemFromCrystalSystemAndGroup(m_crystalSystem);
}

/// Hermann-Mauguin symbol
std::string PointGroup::getSymbol() const { return m_symbolHM; }

bool PointGroup::isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const {
  auto hklEquivalents = getAllEquivalents(hkl);

  return (std::find(hklEquivalents.cbegin(), hklEquivalents.cend(), hkl2) != hklEquivalents.end());
}

std::string PointGroup::getLauePointGroupSymbol() const {
  switch (m_crystalSystem) {
  case CrystalSystem::Triclinic:
    return "-1";
  case CrystalSystem::Monoclinic:
    if (m_symbolHM.substr(0, 2) == "11") {
      return "112/m"; // unique axis c
    } else {
      return "2/m"; // unique axis b
    }
  case CrystalSystem::Orthorhombic:
    return "mmm";
  case CrystalSystem::Tetragonal:
    if (m_symbolHM == "4" || m_symbolHM == "-4" || m_symbolHM == "4/m") {
      return "4/m";
    } else {
      return "4/mmm";
    }
  case CrystalSystem::Hexagonal:
    if (m_symbolHM == "6" || m_symbolHM == "-6" || m_symbolHM == "6/m") {
      return "6/m";
    } else {
      return "6/mmm";
    }
  case CrystalSystem::Trigonal:
    if (m_symbolHM.substr(m_symbolHM.size() - 1) == "r") {
      // Rhombohedral
      if (m_symbolHM == "3" || m_symbolHM == "-3") {
        return "-3 r";
      } else {
        return "-3m r";
      }
    } else {
      // Hexagonal
      if (m_symbolHM == "3" || m_symbolHM == "-3") {
        return "-3";
      } else if (m_symbolHM.substr(1) == "1") {
        return "-31m";
      } else {
        return "-3m1";
      }
    }
  case CrystalSystem::Cubic:
    if (m_symbolHM == "23" || m_symbolHM == "m-3") {
      return "m-3";
    } else {
      return "m-3m";
    }
  default:
    g_log.warning() << "Invalid crystal system - returning group with lowest symmetry (inversion only).\n";
    return "-1"; // never used but required for gcc warning
  }
}

/**
 * Generates a set of hkls
 *
 * This method applies all transformation matrices to the supplied hkl and puts
 * them into a vector, which is returned in the end. For special reflections
 * such as 100 or 110 or 111, the vector may contain duplicates that need to
 * be filtered out.
 *
 * The symmetry operations need to be set prior to calling this method by a call
 * to PointGroup::setTransformationMatrices.
 *
 * @param hkl :: Arbitrary hkl
 * @return :: vector of hkls.
 */
std::vector<V3D> PointGroup::getAllEquivalents(const Kernel::V3D &hkl) const {
  std::vector<V3D> equivalents;
  equivalents.reserve(m_allOperations.size());
  std::transform(m_allOperations.cbegin(), m_allOperations.cend(), std::back_inserter(equivalents),
                 [&hkl](const auto &operation) { return operation.transformHKL(hkl); });
  return equivalents;
}

/**
 * Returns the CrystalSystem determined from symmetry elements
 *
 * This method determines the crystal system of the point group. It makes
 * use of the fact that each crystal system has a characteristic set of
 * symmetry elements. The requirement for the cubic system is for example
 * that four 3-fold axes are present, whereas one 3-fold axis indicates
 * that the group belongs to the trigonal system.
 *
 * @return Crystal system that the point group belongs to.
 */
PointGroup::CrystalSystem PointGroup::getCrystalSystemFromGroup() const {
  std::map<std::string, std::set<V3D>> symbolMap;

  for (const auto &operation : m_allOperations) {
    SymmetryElementWithAxis_sptr element = std::dynamic_pointer_cast<SymmetryElementWithAxis>(
        SymmetryElementFactory::Instance().createSymElement(operation));

    if (element) {
      std::string symbol = element->hmSymbol();
      V3D axis = element->getAxis();

      symbolMap[symbol].insert(axis);
    }
  }

  if (symbolMap["3"].size() == 4) {
    return CrystalSystem::Cubic;
  }

  if (symbolMap["6"].size() == 1 || symbolMap["-6"].size() == 1) {
    return CrystalSystem::Hexagonal;
  }

  if (symbolMap["3"].size() == 1) {
    return CrystalSystem::Trigonal;
  }

  if (symbolMap["4"].size() == 1 || symbolMap["-4"].size() == 1) {
    return CrystalSystem::Tetragonal;
  }

  if (symbolMap["2"].size() == 3 || (symbolMap["2"].size() == 1 && symbolMap["m"].size() == 2)) {
    return CrystalSystem::Orthorhombic;
  }

  if (symbolMap["2"].size() == 1 || symbolMap["m"].size() == 1) {
    return CrystalSystem::Monoclinic;
  }

  return CrystalSystem::Triclinic;
}

/**
 * Returns the LatticeSystem of the point group, using the crystal system
 *
 * This function uses the crystal system argument and the coordinate system
 * stored in Group to determine the lattice system. For all crystal systems
 * except trigonal there is a 1:1 correspondence, but for trigonal groups
 * the lattice system can be either rhombohedral or hexagonal.
 *
 * @param crystalSystem :: CrystalSystem of the point group.
 * @return LatticeSystem the point group belongs to.
 */
PointGroup::LatticeSystem
PointGroup::getLatticeSystemFromCrystalSystemAndGroup(const CrystalSystem &crystalSystem) const {
  switch (crystalSystem) {
  case CrystalSystem::Cubic:
    return LatticeSystem::Cubic;
  case CrystalSystem::Hexagonal:
    return LatticeSystem::Hexagonal;
  case CrystalSystem::Tetragonal:
    return LatticeSystem::Tetragonal;
  case CrystalSystem::Orthorhombic:
    return LatticeSystem::Orthorhombic;
  case CrystalSystem::Monoclinic:
    return LatticeSystem::Monoclinic;
  case CrystalSystem::Triclinic:
    return LatticeSystem::Triclinic;
  default: {
    if (getCoordinateSystem() == Group::Hexagonal) {
      return LatticeSystem::Hexagonal;
    }

    return LatticeSystem::Rhombohedral;
  }
  }
}

/** @return a vector with all possible PointGroup objects */
std::vector<PointGroup_sptr> getAllPointGroups() {
  auto &pointGroupFactory = PointGroupFactory::Instance();
  std::vector<std::string> allSymbols = pointGroupFactory.getAllPointGroupSymbols();
  std::vector<PointGroup_sptr> out;
  out.reserve(allSymbols.size());
  std::transform(allSymbols.cbegin(), allSymbols.cend(), std::back_inserter(out),
                 [&pointGroupFactory](const auto &symbol) { return pointGroupFactory.createPointGroup(symbol); });
  return out;
}

/// Returns a multimap with crystal system as key and point groups as values.
PointGroupCrystalSystemMap getPointGroupsByCrystalSystem() {
  PointGroupCrystalSystemMap map;

  std::vector<PointGroup_sptr> pointGroups = getAllPointGroups();

  for (auto &pointGroup : pointGroups) {
    map.emplace(pointGroup->crystalSystem(), pointGroup);
  }

  return map;
}

/// Return a human-readable string for the given crystal system
std::string getCrystalSystemAsString(const PointGroup::CrystalSystem &crystalSystem) {
  switch (crystalSystem) {
  case PointGroup::CrystalSystem::Cubic:
    return "Cubic";
  case PointGroup::CrystalSystem::Tetragonal:
    return "Tetragonal";
  case PointGroup::CrystalSystem::Hexagonal:
    return "Hexagonal";
  case PointGroup::CrystalSystem::Trigonal:
    return "Trigonal";
  case PointGroup::CrystalSystem::Orthorhombic:
    return "Orthorhombic";
  case PointGroup::CrystalSystem::Monoclinic:
    return "Monoclinic";
  default:
    return "Triclinic";
  }
}

/// Returns the crystal system enum that corresponds to the supplied string or
/// throws an invalid_argument exception.
PointGroup::CrystalSystem getCrystalSystemFromString(const std::string &crystalSystem) {
  std::string crystalSystemLC = boost::algorithm::to_lower_copy(crystalSystem);

  if (crystalSystemLC == "cubic") {
    return PointGroup::CrystalSystem::Cubic;
  } else if (crystalSystemLC == "tetragonal") {
    return PointGroup::CrystalSystem::Tetragonal;
  } else if (crystalSystemLC == "hexagonal") {
    return PointGroup::CrystalSystem::Hexagonal;
  } else if (crystalSystemLC == "trigonal") {
    return PointGroup::CrystalSystem::Trigonal;
  } else if (crystalSystemLC == "orthorhombic") {
    return PointGroup::CrystalSystem::Orthorhombic;
  } else if (crystalSystemLC == "monoclinic") {
    return PointGroup::CrystalSystem::Monoclinic;
  } else if (crystalSystemLC == "triclinic") {
    return PointGroup::CrystalSystem::Triclinic;
  } else {
    throw std::invalid_argument("Not a valid crystal system: '" + crystalSystem + "'.");
  }
}

/// Returns the supplied LatticeSystem as a string.
std::string getLatticeSystemAsString(const PointGroup::LatticeSystem &latticeSystem) {
  switch (latticeSystem) {
  case PointGroup::LatticeSystem::Cubic:
    return "Cubic";
  case PointGroup::LatticeSystem::Tetragonal:
    return "Tetragonal";
  case PointGroup::LatticeSystem::Hexagonal:
    return "Hexagonal";
  case PointGroup::LatticeSystem::Rhombohedral:
    return "Rhombohedral";
  case PointGroup::LatticeSystem::Orthorhombic:
    return "Orthorhombic";
  case PointGroup::LatticeSystem::Monoclinic:
    return "Monoclinic";
  default:
    return "Triclinic";
  }
}

/// Returns the lattice system enum that corresponds to the supplied string or
/// throws an invalid_argument exception.PointGroup::LatticeSystem
PointGroup::LatticeSystem getLatticeSystemFromString(const std::string &latticeSystem) {
  std::string latticeSystemLC = boost::algorithm::to_lower_copy(latticeSystem);

  if (latticeSystemLC == "cubic") {
    return PointGroup::LatticeSystem::Cubic;
  } else if (latticeSystemLC == "tetragonal") {
    return PointGroup::LatticeSystem::Tetragonal;
  } else if (latticeSystemLC == "hexagonal") {
    return PointGroup::LatticeSystem::Hexagonal;
  } else if (latticeSystemLC == "rhombohedral") {
    return PointGroup::LatticeSystem::Rhombohedral;
  } else if (latticeSystemLC == "orthorhombic") {
    return PointGroup::LatticeSystem::Orthorhombic;
  } else if (latticeSystemLC == "monoclinic") {
    return PointGroup::LatticeSystem::Monoclinic;
  } else if (latticeSystemLC == "triclinic") {
    return PointGroup::LatticeSystem::Triclinic;
  } else {
    throw std::invalid_argument("Not a valid lattice system: '" + latticeSystem + "'.");
  }
}

bool CrystalSystemComparator::operator()(const PointGroup::CrystalSystem &lhs,
                                         const PointGroup::CrystalSystem &rhs) const {
  return static_cast<int>(lhs) < static_cast<int>(rhs);
}

/// Returns a streamed representation of the PointGroup object
std::ostream &operator<<(std::ostream &stream, const PointGroup &self) {
  stream << "Point group with:\n"
         << "Lattice system: " << getLatticeSystemAsString(self.latticeSystem()) << "\n"
         << "Crystal system: " << getCrystalSystemAsString(self.crystalSystem()) << "\n"
         << "Symbol: " << self.getSymbol();
  return stream;
}

} // namespace Mantid::Geometry
