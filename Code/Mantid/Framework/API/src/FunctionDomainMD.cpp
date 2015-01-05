//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/IMDIterator.h"

#include <stdexcept>

namespace Mantid {
namespace API {

/**
  * Create a domain form a IMDWorkspace.
  * @param ws :: Pointer to a workspace.
  * @param start :: Index of the first iterator in this domain.
  * @param length :: Size of this domain. If 0 use all workspace.
  */
FunctionDomainMD::FunctionDomainMD(IMDWorkspace_const_sptr ws, size_t start,
                                   size_t length)
    : m_iterator(ws->createIterator()), m_startIndex(start), m_currentIndex(0),
      m_justReset(true), m_workspace(ws) {
  size_t dataSize = m_iterator->getDataSize();
  m_size = length == 0 ? dataSize : length;
  if (start >= dataSize) {
    throw std::out_of_range("Start point out of range");
  }
  if (start + length > dataSize) {
    throw std::out_of_range("End point out of range");
  }
  if (start > 0) {
    m_iterator->jumpTo(start);
  }
}

/** Destructor.
 */
FunctionDomainMD::~FunctionDomainMD() { delete m_iterator; }

/// Reset the iterator to point to the start of the domain.
void FunctionDomainMD::reset() const {
  m_iterator->jumpTo(m_startIndex);
  m_currentIndex = 0;
  m_justReset = true;
}

/**
 * First call after creation returns the first iterator.
 * Successive calls return advanced iterators until the end of the domain
 * reached
 * in which case a NULL pointer is returned.
 * @return :: Pointer to an iterator or NULL.
 */
const IMDIterator *FunctionDomainMD::getNextIterator() const {
  if (m_justReset) {
    m_justReset = false;
    return m_iterator;
  }
  ++m_currentIndex;
  if (!m_iterator->next() || m_currentIndex >= m_size) {
    m_currentIndex = m_size;
    return NULL;
  }
  return m_iterator;
}

/// Returns the pointer to the original workspace
IMDWorkspace_const_sptr FunctionDomainMD::getWorkspace() const {
  return m_workspace;
}

} // namespace API
} // namespace Mantid
