// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_COMPOSITEDOMAINMD_H_
#define MANTID_API_COMPOSITEDOMAINMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/IMDWorkspace.h"

#include <vector>

namespace Mantid {
namespace API {
class FunctionDomainMD;

/**
    A composite domain for MD functions.


    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_API_DLL CompositeDomainMD : public CompositeDomain {
public:
  CompositeDomainMD(IMDWorkspace_const_sptr ws, size_t maxDomainSize);
  ~CompositeDomainMD() override;
  /// Return the total number of arguments in the domain
  size_t size() const override { return m_totalSize; }
  /// Return the number of parts in the domain
  size_t getNParts() const override { return m_domains.size(); }
  /// Return i-th domain
  const FunctionDomain &getDomain(size_t i) const override;

protected:
  mutable std::unique_ptr<IMDIterator> m_iterator; ///< IMDIterator
  size_t m_totalSize; ///< The total size of the domain
  mutable std::vector<std::unique_ptr<FunctionDomainMD>>
      m_domains; ///< smaller parts of the domain
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEDOMAINMD_H_*/
