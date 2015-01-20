//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeDomainMD.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/IMDIterator.h"

#include <stdexcept>

namespace Mantid {
namespace API {

/**
  * Create a composite domain from a IMDWorkspace.
  * @param ws :: Pointer to a workspace.
  * @param maxDomainSize :: The maximum size each domain can have.
  */
CompositeDomainMD::CompositeDomainMD(IMDWorkspace_const_sptr ws,
                                     size_t maxDomainSize)
    : m_iterator(ws->createIterator()) {
  m_totalSize = m_iterator->getDataSize();
  size_t nParts = m_totalSize / maxDomainSize + 1;
  m_domains.resize(nParts);
  for (size_t i = 0; i < nParts - 1; ++i) {
    size_t start = i * maxDomainSize;
    m_domains[i] = new FunctionDomainMD(ws, start, maxDomainSize);
  }
  size_t start = (nParts - 1) * maxDomainSize;
  m_domains.back() = new FunctionDomainMD(ws, start, m_totalSize - start);
}

/**
 * Destructor.
 */
CompositeDomainMD::~CompositeDomainMD() {
  std::vector<FunctionDomainMD *>::iterator it = m_domains.begin();
  for (; it != m_domains.end(); ++it) {
    delete *it;
  }
}

/// Return i-th domain reset to its start.
const FunctionDomain &CompositeDomainMD::getDomain(size_t i) const {
  if (i >= m_domains.size()) {
    throw std::out_of_range("Domain index out of range");
  }
  FunctionDomainMD *domain = m_domains[i];
  domain->reset();
  return *domain;
}

} // namespace API
} // namespace Mantid
