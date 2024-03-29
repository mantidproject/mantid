// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/LatticeDomain.h"
#include "MantidKernel/Exception.h"
namespace Mantid::API {

LatticeDomain::LatticeDomain(std::vector<Kernel::V3D> hkls) : m_hkls(std::move(hkls)) {}

size_t LatticeDomain::size() const { return m_hkls.size(); }

const Kernel::V3D &LatticeDomain::operator[](size_t i) const {
  if (i >= m_hkls.size()) {
    throw Kernel::Exception::IndexError(i, m_hkls.size() - 1, "Index exceeds size of LatticeDomain.");
  }
  return m_hkls[i];
}

} // namespace Mantid::API
