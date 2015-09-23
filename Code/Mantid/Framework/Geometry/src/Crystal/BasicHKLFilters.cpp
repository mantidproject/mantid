#include "MantidGeometry/Crystal/BasicHKLFilters.h"

namespace Mantid {
namespace Geometry {

HKLFilterStructureFactor::HKLFilterStructureFactor(
    const StructureFactorCalculator_sptr &calculator)
    : m_calculator(calculator) {}

bool HKLFilterStructureFactor::isAllowed(const Kernel::V3D &hkl) const {
  return m_calculator->getFSquared(hkl) > 1e-6;
}

} // namespace Geometry
} // namespace Mantid
