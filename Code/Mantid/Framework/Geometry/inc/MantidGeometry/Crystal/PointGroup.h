#ifndef MANTID_GEOMETRY_POINTGROUP_H_
#define MANTID_GEOMETRY_POINTGROUP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <vector>
#include <string>
#include <set>

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/Group.h"

namespace Mantid {
namespace Geometry {

/** A class containing the Point Groups for a crystal.
 *
 * @author Vickie Lynch
 * @date 2012-02-02
 */
class MANTID_GEOMETRY_DLL PointGroup : public Group {
public:
  enum CrystalSystem {
    Triclinic,
    Monoclinic,
    Orthorhombic,
    Tetragonal,
    Hexagonal,
    Trigonal,
    Cubic
  };

  PointGroup(const std::string &symbolHM, const Group &group,
             const std::string &description = "");

  PointGroup(const PointGroup &other);
  PointGroup &operator=(const PointGroup &other);

  virtual ~PointGroup() {}
  /// Name of the point group
  std::string getName() const { return m_name; }
  /// Hermann-Mauguin symbol
  std::string getSymbol() const;

  CrystalSystem crystalSystem() const { return Cubic; }

  /// Return true if the hkls are in same group
  bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;

  /// Returns a vector with all equivalent hkls
  std::vector<Kernel::V3D> getEquivalents(const Kernel::V3D &hkl) const;
  /// Returns the same hkl for all equivalent hkls
  Kernel::V3D getReflectionFamily(const Kernel::V3D &hkl) const;


protected:
  bool groupHasNoTranslations(const Group &group) const;

  std::vector<Kernel::V3D> getEquivalentSet(const Kernel::V3D &hkl) const;

  std::string m_symbolHM;
  std::string m_name;
};

/// Shared pointer to a PointGroup
typedef boost::shared_ptr<PointGroup> PointGroup_sptr;

MANTID_GEOMETRY_DLL std::vector<PointGroup_sptr> getAllPointGroups();

typedef std::multimap<PointGroup::CrystalSystem, PointGroup_sptr>
PointGroupCrystalSystemMap;
MANTID_GEOMETRY_DLL PointGroupCrystalSystemMap getPointGroupsByCrystalSystem();

} // namespace Mantid
} // namespace Geometry

#endif /* MANTID_GEOMETRY_POINTGROUP_H_ */
