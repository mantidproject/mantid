// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif
#include <map>
#include <set>
#include <string>
#include <vector>

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"

namespace Mantid {
namespace Geometry {

/** A class containing the Point Groups for a crystal.
 *
 * @author Vickie Lynch
 * @date 2012-02-02
 */
class MANTID_GEOMETRY_DLL PointGroup : public Group {
public:
  enum class CrystalSystem { Triclinic, Monoclinic, Orthorhombic, Tetragonal, Hexagonal, Trigonal, Cubic };

  enum class LatticeSystem { Triclinic, Monoclinic, Orthorhombic, Tetragonal, Hexagonal, Rhombohedral, Cubic };

  PointGroup(const std::string &symbolHM, const Group &group, const std::string &description = "");
  /// Name of the point group
  const std::string &getName() const { return m_name; }
  /// Hermann-Mauguin symbol
  const std::string &getSymbol() const;

  CrystalSystem crystalSystem() const { return m_crystalSystem; }
  LatticeSystem latticeSystem() const { return m_latticeSystem; }

  std::string getLauePointGroupSymbol() const;

  /// Return true if the hkls are in same group
  bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;

  /// Returns a vector with all equivalent hkls
  std::vector<Kernel::V3D> getEquivalents(const Kernel::V3D &hkl) const;
  /// Returns the same hkl for all equivalent hkls
  Kernel::V3D getReflectionFamily(const Kernel::V3D &hkl) const;

protected:
  std::vector<Kernel::V3D> getAllEquivalents(const Kernel::V3D &hkl) const;

  CrystalSystem getCrystalSystemFromGroup() const;
  LatticeSystem getLatticeSystemFromCrystalSystemAndGroup(const CrystalSystem &crystalSystem) const;

  std::string m_symbolHM;
  std::string m_name;
  CrystalSystem m_crystalSystem;
  LatticeSystem m_latticeSystem;
};

/// Shared pointer to a PointGroup
using PointGroup_sptr = std::shared_ptr<PointGroup>;

MANTID_GEOMETRY_DLL std::vector<PointGroup_sptr> getAllPointGroups();

MANTID_GEOMETRY_DLL
std::string getCrystalSystemAsString(const PointGroup::CrystalSystem &crystalSystem);

MANTID_GEOMETRY_DLL
PointGroup::CrystalSystem getCrystalSystemFromString(const std::string &crystalSystem);

MANTID_GEOMETRY_DLL
std::string getLatticeSystemAsString(const PointGroup::LatticeSystem &latticeSystem);

MANTID_GEOMETRY_DLL
PointGroup::LatticeSystem getLatticeSystemFromString(const std::string &latticeSystem);

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &stream, const PointGroup &self);

/// This is necessary to make the map work with older compilers. Can be removed
/// when GCC 4.4 is not used anymore.
struct MANTID_GEOMETRY_DLL CrystalSystemComparator {
  bool operator()(const PointGroup::CrystalSystem &lhs, const PointGroup::CrystalSystem &rhs) const;
};

using PointGroupCrystalSystemMap = std::multimap<PointGroup::CrystalSystem, PointGroup_sptr, CrystalSystemComparator>;

MANTID_GEOMETRY_DLL PointGroupCrystalSystemMap getPointGroupsByCrystalSystem();

} // namespace Geometry
} // namespace Mantid
