#include "MantidGeometry/Crystal/HKLGenerator.h"

namespace Mantid {
namespace Geometry {

using namespace Kernel;

HKLGenerator::HKLGenerator(const Kernel::V3D &hklMin, const Kernel::V3D &hklMax)
    : m_hklMin(hklMin), m_hklMax(hklMax), m_size(getSize(m_hklMin, m_hklMax)) {}

HKLGenerator::HKLGenerator(const Kernel::V3D &hklMinMax)
    : m_hklMin(hklMinMax * -1), m_hklMax(hklMinMax),
      m_size(getSize(m_hklMin, m_hklMax)) {}

HKLGenerator::HKLGenerator(int hMinMax, int kMinMax, int lMinMax) {
  m_hklMax = V3D(hMinMax, kMinMax, lMinMax);
  m_hklMin = m_hklMax * -1;
  m_size = getSize(m_hklMin, m_hklMax);
}

HKLGenerator::HKLGenerator(const UnitCell &unitCell, double dMin) {
  m_hklMax = V3D(unitCell.a() / dMin, unitCell.b() / dMin, unitCell.c() / dMin);
  m_hklMin = m_hklMax * -1;
  m_size = getSize(m_hklMin, m_hklMax);
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
    : m_h(static_cast<int>(current.X())), m_k(static_cast<int>(current.Y())),
      m_l(static_cast<int>(current.Z())), m_hMin(m_h), m_hMax(m_h), m_kMin(m_k),
      m_kMax(m_k), m_lMin(m_l), m_lMax(m_l) {}

HKLGenerator::const_iterator::const_iterator(const V3D &hklMin,
                                             const V3D &hklMax)
    : m_h(static_cast<int>(hklMin.X())), m_k(static_cast<int>(hklMin.Y())),
      m_l(static_cast<int>(hklMin.Z())), m_hMin(m_h),
      m_hMax(static_cast<int>(hklMax.X())), m_kMin(m_k),
      m_kMax(static_cast<int>(hklMax.Y())), m_lMin(m_l),
      m_lMax(static_cast<int>(hklMax.Z())) {}

HKLGenerator::const_iterator::const_iterator(const V3D &hklMin,
                                             const V3D &hklMax,
                                             const V3D &current)
    : m_h(static_cast<int>(current.X())), m_k(static_cast<int>(current.Y())),
      m_l(static_cast<int>(current.Z())), m_hMin(static_cast<int>(hklMin.X())),
      m_hMax(static_cast<int>(hklMax.X())),
      m_kMin(static_cast<int>(hklMin.Y())),
      m_kMax(static_cast<int>(hklMax.Y())),
      m_lMin(static_cast<int>(hklMin.Z())),
      m_lMax(static_cast<int>(hklMax.Z())) {}

void HKLGenerator::const_iterator::increment() {
  if (++m_l > m_lMax) {
    m_l = m_lMin;

    if (++m_k > m_kMax) {
      m_k = m_kMin;
      ++m_h;
    }
  }

  m_hkl.setX(m_h);
  m_hkl.setY(m_k);
  m_hkl.setZ(m_l);
}

} // namespace Geometry
} // namespace Mantid
