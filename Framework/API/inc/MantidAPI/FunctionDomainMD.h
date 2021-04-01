// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid {
namespace API {
/** Implements a domain for MD functions (IFunctionMD).

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_API_DLL FunctionDomainMD : public FunctionDomain {
public:
  /// Constructor.
  FunctionDomainMD(const IMDWorkspace_const_sptr &ws, size_t start = 0, size_t length = 0);
  /// Destructor.
  ~FunctionDomainMD() override;
  /// Return the number of arguments in the domain
  size_t size() const override { return m_size; }
  /// Reset the iterator to point to the start of the domain.
  void reset() const override;
  /// Next iterator.
  const IMDIterator *getNextIterator() const;
  /// Returns the pointer to the original workspace
  IMDWorkspace_const_sptr getWorkspace() const;

protected:
  /// IMDIterator
  mutable std::unique_ptr<IMDIterator> m_iterator;
  /// start of the domain, 0 <= m_startIndex < m_iterator->getDataSize()
  const size_t m_startIndex;
  /// track the iterator's index, 0 <= m_currentIndex < m_size.
  mutable size_t m_currentIndex;
  /// The size of the domain
  size_t m_size;
  /// Just reset flag
  mutable bool m_justReset;

private:
  /// A pointer to the workspace
  IMDWorkspace_const_sptr m_workspace;
};

} // namespace API
} // namespace Mantid
