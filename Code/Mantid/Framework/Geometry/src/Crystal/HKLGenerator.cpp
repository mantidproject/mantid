#include "MantidGeometry/Crystal/HKLGenerator.h"

namespace Mantid {
namespace Geometry {

using namespace Kernel;

HKLGenerator::HKLGenerator(const Kernel::V3D &hklMin, const Kernel::V3D &hklMax)
    : m_hklMin(hklMin), m_hklMax(hklMax), m_size(getSize(m_hklMin, m_hklMax)) {}

HKLGenerator::HKLGenerator(const Kernel::V3D &hklMinMax)
    : m_hklMin(hklMinMax * -1), m_hklMax(hklMinMax),
      m_size(getSize(m_hklMin, m_hklMax)) {}

HKLGenerator::HKLGenerator(int hMinMax, int kMinMax, int lMinMax)
    : m_hklMin(-hMinMax, -kMinMax, -lMinMax),
      m_hklMax(hMinMax, kMinMax, lMinMax), m_size(getSize(m_hklMin, m_hklMax)) {
}

HKLGenerator::const_iterator HKLGenerator::end() const {
  return HKLGenerator::const_iterator(getEndHKL());
}

V3D HKLGenerator::getEndHKL() const {
  return V3D(m_hklMax.X() + 1, m_hklMin.Y(), m_hklMin.Z());
}

size_t HKLGenerator::getSize(const V3D &min, const V3D &max) const {
  V3D diff = (max - min) + V3D(1, 1, 1);
  return static_cast<size_t>(diff.X() * diff.Y() * diff.Z());
}

HKLGenerator::const_iterator HKLGenerator::begin() const {
  return HKLGenerator::const_iterator(m_hklMin, m_hklMax);
}

HKLGenerator::const_iterator::const_iterator(const V3D &current)
    : m_h(current.X()), m_k(current.Y()), m_l(current.Z()), m_hMin(m_h),
      m_hMax(m_h), m_kMin(m_k), m_kMax(m_k), m_lMin(m_l), m_lMax(m_l) {}

HKLGenerator::const_iterator::const_iterator(const V3D &hklMin,
                                             const V3D &hklMax)
    : m_h(hklMin.X()), m_k(hklMin.Y()), m_l(hklMin.Z()), m_hMin(m_h),
      m_hMax(hklMax.X()), m_kMin(m_k), m_kMax(hklMax.Y()), m_lMin(m_l),
      m_lMax(hklMax.Z()) {}

HKLGenerator::const_iterator::const_iterator(const V3D &hklMin,
                                             const V3D &hklMax,
                                             const V3D &current)
    : m_h(current.X()), m_k(current.Y()), m_l(current.Z()), m_hMin(hklMin.X()),
      m_hMax(hklMax.X()), m_kMin(m_k), m_kMax(hklMax.Y()), m_lMin(m_l),
      m_lMax(hklMax.Z()) {}

void HKLGenerator::const_iterator::increment() {
  if (++m_l > m_lMax) {
    m_l = m_lMin;

    if (++m_k > m_kMax) {
      m_k = m_kMin;
      ++m_h;
    }
  }
}

} // namespace Geometry
} // namespace Mantid
