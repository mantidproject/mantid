#include "MantidAPI/LatticeDomain.h"
#include "MantidKernel/Exception.h"
namespace Mantid {
namespace API {

LatticeDomain::LatticeDomain(const std::vector<Kernel::V3D> &hkls)
    : m_hkls(hkls) {}

size_t LatticeDomain::size() const { return m_hkls.size(); }

const Kernel::V3D &LatticeDomain::operator[](size_t i) const {
  if (i >= m_hkls.size()) {
    throw Kernel::Exception::IndexError(i, m_hkls.size() - 1,
                                        "Index exceeds size of LatticeDomain.");
  }
  return m_hkls[i];
}

} // namespace API
} // namespace Mantid
