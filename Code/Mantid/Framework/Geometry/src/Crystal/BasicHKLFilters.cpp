#include "MantidGeometry/Crystal/BasicHKLFilters.h"

namespace Mantid {
namespace Geometry {

/// Constructor, dMax is set to the largest lattice parameter.
HKLFilterDRange::HKLFilterDRange(const UnitCell &cell, double dMin)
    : m_cell(cell), m_dmin(dMin) {
  m_dmax = std::max(m_cell.a(), std::max(m_cell.b(), m_cell.c()));
}

/// Constructor with explicit dMax.
HKLFilterDRange::HKLFilterDRange(const UnitCell &cell, double dMin, double dMax)
    : m_cell(cell), m_dmin(dMin), m_dmax(dMax) {}

/// Returns a description containing the parameters of the filter.
std::string HKLFilterDRange::getDescription() const {
  std::ostringstream strm;
  strm << "(" << m_dmin << " <= d <= " << m_dmax << ")";

  return strm.str();
}

/// Returns true if the d-value of the HKL is within the specified range.
bool HKLFilterDRange::isAllowed(const Kernel::V3D &hkl) const {
  double d = m_cell.d(hkl);

  return d >= m_dmin && d <= m_dmax;
}

/// Constructor, throws exception if the supplied pointer is invalid.
HKLFilterSpaceGroup::HKLFilterSpaceGroup(
    const SpaceGroup_const_sptr &spaceGroup)
    : m_spaceGroup(spaceGroup) {
  if (!m_spaceGroup) {
    throw std::runtime_error(
        "Cannot construct HKLFilterSpaceGroup from null space group.");
  }
}

/// Returns a description of the filter that contains the space group symbol.
std::string HKLFilterSpaceGroup::getDescription() const {
  std::ostringstream strm;
  strm << "(Space group: " << m_spaceGroup->hmSymbol() << ")";

  return strm.str();
}

/// Returns true if the reflection is allowed by the space group reflection
/// conditions.
bool HKLFilterSpaceGroup::isAllowed(const Kernel::V3D &hkl) const {
  return m_spaceGroup->isAllowedReflection(hkl);
}

/// Constructor, throws exception if the calculator pointer is invalid.
HKLFilterStructureFactor::HKLFilterStructureFactor(
    const StructureFactorCalculator_sptr &calculator, double fSquaredMin)
    : m_calculator(calculator), m_fSquaredMin(fSquaredMin) {
  if (!m_calculator) {
    throw std::runtime_error(
        "Cannot construct HKLFilterStructureFactor from null calculator.");
  }
}

/// Returns a description for the filter that contains the minimum F^2.
std::string HKLFilterStructureFactor::getDescription() const {
  std::ostringstream strm;
  strm << "(F^2 > " << m_fSquaredMin << ")";

  return strm.str();
}

/// Returns true if F^2(hkl) is larger than the stored minimum.
bool HKLFilterStructureFactor::isAllowed(const Kernel::V3D &hkl) const {
  return m_calculator->getFSquared(hkl) > m_fSquaredMin;
}

/// Constructor, throws exception if pointer is null.
HKLFilterCentering::HKLFilterCentering(
    const ReflectionCondition_sptr &centering)
    : m_centering(centering) {
  if (!m_centering) {
    throw std::runtime_error(
        "Cannot construct HKLFilterCentering from null centering.");
  }
}

/// Returns a description with the centering symbol.
std::string HKLFilterCentering::getDescription() const {
  return "(Centering: " + m_centering->getSymbol() + ")";
}

/// Returns true if the HKL is allowed according to the lattice centering.
bool HKLFilterCentering::isAllowed(const Kernel::V3D &hkl) const {
  return m_centering->isAllowed(static_cast<int>(hkl.X()),
                                static_cast<int>(hkl.Y()),
                                static_cast<int>(hkl.Z()));
}

} // namespace Geometry
} // namespace Mantid
