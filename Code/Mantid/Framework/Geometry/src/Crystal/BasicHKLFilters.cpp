#include "MantidGeometry/Crystal/BasicHKLFilters.h"

namespace Mantid {
namespace Geometry {

HKLFilterDRange::HKLFilterDRange(const UnitCell &cell, double dMin)
    : m_cell(cell), m_dmin(dMin) {
  m_dmax = std::max(m_cell.a(), std::max(m_cell.b(), m_cell.c()));
}

HKLFilterDRange::HKLFilterDRange(const UnitCell &cell, double dMin, double dMax)
    : m_cell(cell), m_dmin(dMin), m_dmax(dMax) {}

std::string HKLFilterDRange::getDescription() const {
  std::ostringstream strm;
  strm << "(" << m_dmin << " <= d <= " << m_dmax << ")";

  return strm.str();
}

bool HKLFilterDRange::isAllowed(const Kernel::V3D &hkl) const {
  double d = m_cell.d(hkl);

  return d >= m_dmin && d <= m_dmax;
}

HKLFilterSpaceGroup::HKLFilterSpaceGroup(
    const SpaceGroup_const_sptr &spaceGroup)
    : m_spaceGroup(spaceGroup) {
  if (!m_spaceGroup) {
    throw std::runtime_error(
        "Cannot construct HKLFilterSpaceGroup from null space group.");
  }
}

std::string HKLFilterSpaceGroup::getDescription() const {
  std::ostringstream strm;
  strm << "(Space group: " << m_spaceGroup->hmSymbol() << ")";

  return strm.str();
}

bool HKLFilterSpaceGroup::isAllowed(const Kernel::V3D &hkl) const {
  return m_spaceGroup->isAllowedReflection(hkl);
}

HKLFilterStructureFactor::HKLFilterStructureFactor(
    const StructureFactorCalculator_sptr &calculator, double fSquaredMin)
    : m_calculator(calculator), m_fSquaredMin(fSquaredMin) {
  if (!m_calculator) {
    throw std::runtime_error(
        "Cannot construct HKLFilterStructureFactor from null calculator.");
  }
}

std::string HKLFilterStructureFactor::getDescription() const {
  std::ostringstream strm;
  strm << "(F^2 > " << m_fSquaredMin << ")";

  return strm.str();
}

bool HKLFilterStructureFactor::isAllowed(const Kernel::V3D &hkl) const {
  return m_calculator->getFSquared(hkl) > m_fSquaredMin;
}

HKLFilterCentering::HKLFilterCentering(
    const ReflectionCondition_sptr &centering)
    : m_centering(centering) {
  if (!m_centering) {
    throw std::runtime_error(
        "Cannot construct HKLFilterCentering from null centering.");
  }
}

std::string HKLFilterCentering::getDescription() const {
  return "(Centering: " + m_centering->getSymbol() + ")";
}

bool HKLFilterCentering::isAllowed(const Kernel::V3D &hkl) const {
  return m_centering->isAllowed(static_cast<int>(hkl.X()),
                                static_cast<int>(hkl.Y()),
                                static_cast<int>(hkl.Z()));
}

} // namespace Geometry
} // namespace Mantid
