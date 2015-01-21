#include "MantidGeometry/Crystal/CrystalStructure.h"
#include <boost/bind.hpp>
#include <stdexcept>

#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"

namespace Mantid {
namespace Geometry {

using namespace Mantid::Kernel;

/// PointGroup/Centering based constructor
CrystalStructure::CrystalStructure(const UnitCell &unitCell,
                                   const PointGroup_sptr &pointGroup,
                                   const ReflectionCondition_sptr &centering) {
  initializeScatterers();

  setCell(unitCell);
  setPointGroup(pointGroup);
  setCentering(centering);
}

/// SpaceGroup/Scatterers constructor
CrystalStructure::CrystalStructure(
    const UnitCell &unitCell, const SpaceGroup_const_sptr &spaceGroup,
    const CompositeBraggScatterer_sptr &scatterers) {
  initializeScatterers();

  addScatterers(scatterers);
  setCell(unitCell);
  setSpaceGroup(spaceGroup);
}

/// Returns the unit cell of the structure
UnitCell CrystalStructure::cell() const { return m_cell; }

/// Assigns a new unit cell
void CrystalStructure::setCell(const UnitCell &cell) {
  m_cell = cell;

  assignUnitCellToScatterers(m_cell);
}

/// Returns the space group of the crystal structure
SpaceGroup_const_sptr CrystalStructure::spaceGroup() const {
  return m_spaceGroup;
}

/**
 * Assigns a new space group to the crystal structure
 *
 * Setting a new space group on a crystal structure causes several effects:
 *  - The space group is propagated to the scatterers
 *  - A new point group is assigned
 *  - A change in point group can also mean change in crystal system
 *  - A new reflection condition object is assigned
 * After this operation, the crystal structure object may be in a state
 * where it does not have a valid point group or centering.
 *
 * @param spaceGroup :: New space group of the crystal structure
 */
void CrystalStructure::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup) {
  m_spaceGroup = spaceGroup;

  setPointGroupFromSpaceGroup(m_spaceGroup);
  setReflectionConditionFromSpaceGroup(m_spaceGroup);
  assignSpaceGroupToScatterers(m_spaceGroup);
}

/// Assigns the point group or throws std::runtime_error if the space group has
/// been set before.
void CrystalStructure::setPointGroup(const PointGroup_sptr &pointGroup) {
  if (m_spaceGroup) {
    throw std::runtime_error(
        "Cannot set point group if a space group has been set.");
  }

  m_pointGroup = pointGroup;
}

/// Returns the structure's point group
PointGroup_sptr CrystalStructure::pointGroup() const { return m_pointGroup; }

/// Convenience method to get the PointGroup's crystal system
PointGroup::CrystalSystem CrystalStructure::crystalSystem() const {
  if (!m_pointGroup) {
    throw std::invalid_argument(
        "Cannot determine crystal system from null-PointGroup.");
  }

  return m_pointGroup->crystalSystem();
}

/// Sets the centering or throws std::runtime_error if a space group has been
/// assigned to the crystal structure
void CrystalStructure::setCentering(const ReflectionCondition_sptr &centering) {
  if (m_spaceGroup) {
    throw std::runtime_error(
        "Cannot set centering if a space group has been set.");
  }

  m_centering = centering;
}

/// Returns the centering of the structure
ReflectionCondition_sptr CrystalStructure::centering() const {
  return m_centering;
}

/// Return a clone of the internal CompositeBraggScatterer instance.
CompositeBraggScatterer_sptr CrystalStructure::getScatterers() const {
  BraggScatterer_sptr clone = m_scatterers->clone();

  return boost::dynamic_pointer_cast<CompositeBraggScatterer>(clone);
}

/// Remove all scatterers and set the supplied ones as new scatterers.
void CrystalStructure::setScatterers(
    const CompositeBraggScatterer_sptr &scatterers) {
  m_scatterers->removeAllScatterers();

  addScatterers(scatterers);
}

/// Adds all scatterers in the supplied collection into the internal one
/// (scatterers are copied).
void CrystalStructure::addScatterers(
    const CompositeBraggScatterer_sptr &scatterers) {
  size_t count = scatterers->nScatterers();

  for (size_t i = 0; i < count; ++i) {
    m_scatterers->addScatterer(scatterers->getScatterer(i));
  }

  assignUnitCellToScatterers(m_cell);
  assignSpaceGroupToScatterers(m_spaceGroup);
}

/// Returns a vector with all allowed HKLs in the given d-range
std::vector<Kernel::V3D>
CrystalStructure::getHKLs(double dMin, double dMax,
                          ReflectionConditionMethod method) const {
  if (!isStateSufficientForHKLGeneration(method)) {
    throw std::invalid_argument("Insufficient data for creation of a "
                                "reflection list, need at least centering and "
                                "unit cell.");
  }

  throwIfRangeUnacceptable(dMin, dMax);

  std::vector<V3D> hkls;

  /* There's an estimation for the number of reflections from the unit cell
   * volume
   * and the minimum d-value. This should speed up insertion into the vector.
   */
  size_t estimatedReflectionCount = static_cast<size_t>(
      ceil(32.0 * M_PI * m_cell.volume()) / (3.0 * pow(2.0 * dMin, 3.0)));
  hkls.reserve(estimatedReflectionCount);

  int hMax = static_cast<int>(m_cell.a() / dMin);
  int kMax = static_cast<int>(m_cell.b() / dMin);
  int lMax = static_cast<int>(m_cell.c() / dMin);

  for (int h = -hMax; h <= hMax; ++h) {
    for (int k = -kMax; k <= kMax; ++k) {
      for (int l = -lMax; l <= lMax; ++l) {
        V3D hkl(h, k, l);
        double d = getDValue(hkl);

        if (d <= dMax && d >= dMin && isAllowed(hkl, method)) {
          hkls.push_back(hkl);
        }
      }
    }
  }

  return hkls;
}

/// Returns a vector with all allowed symmetry independent HKLs (depends on
/// point group) in the given d-range
std::vector<Kernel::V3D>
CrystalStructure::getUniqueHKLs(double dMin, double dMax,
                                ReflectionConditionMethod method) const {
  if (!isStateSufficientForUniqueHKLGeneration(method)) {
    throw std::invalid_argument("Insufficient data for creation of a "
                                "reflection list, need at least centering, "
                                "unit cell and point group.");
  }

  throwIfRangeUnacceptable(dMin, dMax);

  std::set<V3D> uniqueHKLs;

  int hMax = static_cast<int>(m_cell.a() / dMin);
  int kMax = static_cast<int>(m_cell.b() / dMin);
  int lMax = static_cast<int>(m_cell.c() / dMin);

  for (int h = -hMax; h <= hMax; ++h) {
    for (int k = -kMax; k <= kMax; ++k) {
      for (int l = -lMax; l <= lMax; ++l) {
        V3D hkl(h, k, l);
        double d = getDValue(hkl);

        if (d <= dMax && d >= dMin && isAllowed(hkl, method)) {
          uniqueHKLs.insert(m_pointGroup->getReflectionFamily(hkl));
        }
      }
    }
  }

  return std::vector<V3D>(uniqueHKLs.begin(), uniqueHKLs.end());
}

/// Maps a vector of hkls to d-values using the internal cell
std::vector<double>
CrystalStructure::getDValues(const std::vector<V3D> &hkls) const {

  std::vector<double> dValues(hkls.size());
  std::transform(hkls.begin(), hkls.end(), dValues.begin(),
                 boost::bind<double>(&CrystalStructure::getDValue, this, _1));

  return dValues;
}

/// Returns |F(hkl)|^2 for all supplied hkls.
std::vector<double>
CrystalStructure::getFSquared(const std::vector<V3D> &hkls) const {
  std::vector<double> fSquared;
  fSquared.reserve(hkls.size());

  for (auto hkl = hkls.begin(); hkl != hkls.end(); ++hkl) {
    fSquared.push_back(getFSquared(*hkl));
  }

  return fSquared;
}

/// Tries to set the point group from the space group symbol or removes the
/// current point group if creation fails.
void CrystalStructure::setPointGroupFromSpaceGroup(
    const SpaceGroup_const_sptr &spaceGroup) {
  m_pointGroup.reset();

  if (spaceGroup) {
    try {
      m_pointGroup =
          PointGroupFactory::Instance().createPointGroupFromSpaceGroupSymbol(
              spaceGroup->hmSymbol());
    } catch (...) {
      // do nothing - point group will be null
    }
  }
}

/// Tries to set the centering from the space group symbol or removes the
/// current centering if creation fails.
void CrystalStructure::setReflectionConditionFromSpaceGroup(
    const SpaceGroup_const_sptr &spaceGroup) {
  m_centering.reset();

  // First letter is centering
  std::string centering = spaceGroup->hmSymbol().substr(0, 1);

  std::vector<ReflectionCondition_sptr> reflectionConditions =
      getAllReflectionConditions();
  for (auto it = reflectionConditions.begin(); it != reflectionConditions.end();
       ++it) {
    if ((*it)->getSymbol() == centering) {
      m_centering = *it;
      break;
    }
  }
}

/// Assigns the space group to all scatterers
void CrystalStructure::assignSpaceGroupToScatterers(
    const SpaceGroup_const_sptr &spaceGroup) {
  if (!m_scatterers) {
    throw std::runtime_error(
        "Scatterer collection is a null pointer. Aborting.");
  }

  if (m_spaceGroup && m_scatterers->existsProperty("SpaceGroup")) {
    m_scatterers->setProperty("SpaceGroup", spaceGroup->hmSymbol());
  }
}

/// Assigns the cell to all scatterers
void CrystalStructure::assignUnitCellToScatterers(const UnitCell &unitCell) {
  if (!m_scatterers) {
    throw std::runtime_error(
        "Scatterer collection is a null pointer. Aborting.");
  }

  if (m_scatterers->existsProperty("UnitCell")) {
    m_scatterers->setProperty("UnitCell", unitCellToStr(unitCell));
  }
}

/// Initializes the internal storage for scatterers
void CrystalStructure::initializeScatterers() {
  if (!m_scatterers) {
    m_scatterers = CompositeBraggScatterer::create();
  }
}

/// Check that the internal state is sufficient for generating a list of HKLs
bool CrystalStructure::isStateSufficientForHKLGeneration(
    CrystalStructure::ReflectionConditionMethod method) const {
  switch (method) {
  case UseStructureFactor:
    return m_scatterers->nScatterers() > 0;
  default:
    return static_cast<bool>(m_centering);
  }
}

/// Check that the internal state is sufficient for generating a list of
/// symmetry independent HKLs
bool CrystalStructure::isStateSufficientForUniqueHKLGeneration(
    CrystalStructure::ReflectionConditionMethod method) const {
  return isStateSufficientForHKLGeneration(method) && m_pointGroup;
}

/// Throws std::invalid_argument if dMin <= 0 or dMax >= dMin
void CrystalStructure::throwIfRangeUnacceptable(double dMin,
                                                double dMax) const {
  if (dMin <= 0.0) {
    throw std::invalid_argument("dMin is <= 0.0, not a valid spacing.");
  }

  if (dMax <= dMin) {
    throw std::invalid_argument("dMax must be larger than dMin.");
  }
}

/// Checks whether a reflection is allowed, using the specified method
bool CrystalStructure::isAllowed(
    const V3D &hkl, CrystalStructure::ReflectionConditionMethod method) const {
  switch (method) {
  case UseStructureFactor:
    return getFSquared(hkl) > 1e-9;
  default:
    return m_centering->isAllowed(static_cast<int>(hkl.X()),
                                  static_cast<int>(hkl.Y()),
                                  static_cast<int>(hkl.Z()));
  }
}

/// Returns the lattice plane spacing for the given HKL.
double CrystalStructure::getDValue(const V3D &hkl) const {
  return m_cell.d(hkl);
}

/// Returns |F|^2 for the given HKL.
double CrystalStructure::getFSquared(const V3D &hkl) const {
  return m_scatterers->calculateFSquared(hkl);
}

} // namespace Geometry
} // namespace Mantid
