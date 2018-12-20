#include "MantidGeometry/Crystal/HKLGenerator.h"

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Constructs a generator that creates all indices from hklMin to hklMax.
HKLGenerator::HKLGenerator(const Kernel::V3D &hklMin, const Kernel::V3D &hklMax)
    : m_hklMin(hklMin), m_hklMax(hklMax), m_size(getSize(m_hklMin, m_hklMax)),
      m_begin(getBeginIterator()), m_end(getEndIterator()) {}

/// Constructs a generator that creates all indices from -hklMinMax to
/// hklMinMax.
HKLGenerator::HKLGenerator(const Kernel::V3D &hklMinMax)
    : m_hklMin(hklMinMax * -1), m_hklMax(hklMinMax),
      m_size(getSize(m_hklMin, m_hklMax)), m_begin(getBeginIterator()),
      m_end(getEndIterator()) {}

/// Constructs a generator that creates all indices from -h,-k,-l to h,k,l.
HKLGenerator::HKLGenerator(int hMinMax, int kMinMax, int lMinMax) {
  m_hklMax = V3D(hMinMax, kMinMax, lMinMax);
  m_hklMin = m_hklMax * -1;
  m_size = getSize(m_hklMin, m_hklMax);

  m_begin = getBeginIterator();
  m_end = getEndIterator();
}

/// Constructs a generator that creates all indices for the given cell up to
/// dMin.
HKLGenerator::HKLGenerator(const UnitCell &unitCell, double dMin) {
  m_hklMax = V3D(floor(unitCell.a() / dMin), floor(unitCell.b() / dMin),
                 floor(unitCell.c() / dMin));
  m_hklMin = m_hklMax * -1;
  m_size = getSize(m_hklMin, m_hklMax);

  m_begin = getBeginIterator();
  m_end = getEndIterator();
}

/// Returns the number of indices between min and max.
size_t HKLGenerator::getSize(const V3D &min, const V3D &max) const {
  V3D diff = (max - min) + V3D(1, 1, 1);
  return static_cast<size_t>(diff.X() * diff.Y() * diff.Z());
}

/// Constructs an iterator that points to the beginning of the sequence.
HKLGenerator::const_iterator HKLGenerator::getBeginIterator() const {
  return HKLGenerator::const_iterator(m_hklMin, m_hklMax);
}

/// Constructs an iterator that points to an HKL one past the maximum.
HKLGenerator::const_iterator HKLGenerator::getEndIterator() const {
  return HKLGenerator::const_iterator(getEndHKL());
}

/// Returns the HKL "one past the maximum".
V3D HKLGenerator::getEndHKL() const {
  return V3D(m_hklMax.X() + 1, m_hklMin.Y(), m_hklMin.Z());
}

/// Default constructor, requirement from boost::iterator_facade
HKLGenerator::const_iterator::const_iterator()
    : m_h(0), m_k(0), m_l(0), m_hkl(V3D(0, 0, 0)), m_hMax(0), m_kMin(0),
      m_kMax(0), m_lMin(0), m_lMax(0) {}

/// Return an iterator with min = max = current.
HKLGenerator::const_iterator::const_iterator(const V3D &current)
    : m_h(static_cast<int>(current.X())), m_k(static_cast<int>(current.Y())),
      m_l(static_cast<int>(current.Z())), m_hkl(current), m_hMax(m_h),
      m_kMin(m_k), m_kMax(m_k), m_lMin(m_l), m_lMax(m_l) {}

/// Return an iterator that can move from min to max, with current = min
HKLGenerator::const_iterator::const_iterator(const V3D &hklMin,
                                             const V3D &hklMax)
    : m_h(static_cast<int>(hklMin.X())), m_k(static_cast<int>(hklMin.Y())),
      m_l(static_cast<int>(hklMin.Z())), m_hkl(hklMin),
      m_hMax(static_cast<int>(hklMax.X())), m_kMin(m_k),
      m_kMax(static_cast<int>(hklMax.Y())), m_lMin(m_l),
      m_lMax(static_cast<int>(hklMax.Z())) {}

/// Increments HKL, l moves fastest, h moves slowest, wrapping around at the max
/// for each index.
void HKLGenerator::const_iterator::increment() {
  ++m_l;

  if (m_l > m_lMax) {
    m_l = m_lMin;

    ++m_k;
    if (m_k > m_kMax) {
      m_k = m_kMin;
      ++m_h;
    }
  }

  m_hkl = V3D(m_h, m_k, m_l);
}

} // namespace Geometry
} // namespace Mantid
