// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_JOINTDOMAIN_H_
#define MANTID_API_JOINTDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/DllConfig.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace API {
/** An implementation of CompositeDomain.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_API_DLL JointDomain : public CompositeDomain {
public:
  /// Return the number of points in the domain
  size_t size() const override;
  /// Return the number of parts in the domain
  size_t getNParts() const override;
  /// Return i-th domain
  const FunctionDomain &getDomain(size_t i) const override;
  void addDomain(FunctionDomain_sptr domain);

protected:
  /// Vector with member domains.
  std::vector<FunctionDomain_sptr> m_domains;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_JOINTDOMAIN_H_*/
