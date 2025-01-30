// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"

#include "boost/format.hpp"
#include <stdexcept>

namespace Mantid::Poldi {

MillerIndices::MillerIndices(int h, int k, int l)
    : m_h(h), m_k(k), m_l(l), m_asVector(3),
      m_asV3D(static_cast<double>(h), static_cast<double>(k), static_cast<double>(l)) {
  populateVector();
}

MillerIndices::MillerIndices(std::vector<int> &hkl) {
  if (hkl.size() != 3) {
    throw std::runtime_error("MillerIndices object can only be created with 3 indices");
  }

  m_asVector = hkl;
  m_h = hkl[0];
  m_k = hkl[1];
  m_l = hkl[2];
  m_asV3D(static_cast<double>(m_h), static_cast<double>(m_k), static_cast<double>(m_l));
}

MillerIndices::MillerIndices(const Kernel::V3D &hkl)
    : m_h(static_cast<int>(hkl.X())), m_k(static_cast<int>(hkl.Y())), m_l(static_cast<int>(hkl.Z())), m_asVector(3),
      m_asV3D(hkl) {
  populateVector();
}

int MillerIndices::h() const { return m_h; }

int MillerIndices::k() const { return m_k; }

int MillerIndices::l() const { return m_l; }

int MillerIndices::operator[](int index) {
  if (index < 0 || index > 2) {
    throw std::range_error("Index for accessing hkl is out of range.");
  }

  return m_asVector[index];
}

bool MillerIndices::operator==(const MillerIndices &other) const {
  return m_h == other.m_h && m_k == other.m_k && m_l == other.m_l;
}

bool MillerIndices::operator!=(const MillerIndices &other) const { return !operator==(other); }

const std::vector<int> &MillerIndices::asVector() const { return m_asVector; }

const Kernel::V3D &MillerIndices::asV3D() const { return m_asV3D; }

void MillerIndices::populateVector() {
  m_asVector[0] = m_h;
  m_asVector[1] = m_k;
  m_asVector[2] = m_l;
}
} // namespace Mantid::Poldi
