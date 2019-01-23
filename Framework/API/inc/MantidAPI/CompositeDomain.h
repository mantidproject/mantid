// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_COMPOSITEDOMAIN_H_
#define MANTID_API_COMPOSITEDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"

#include <stdexcept>

namespace Mantid {
namespace API {
/** Base class for a composite domain. A composite domain consists of a set of
   domains.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_API_DLL CompositeDomain : public FunctionDomain {
public:
  /// Return the number of parts in the domain
  virtual size_t getNParts() const = 0;
  /// Return i-th domain
  virtual const FunctionDomain &getDomain(size_t i) const = 0;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEDOMAIN_H_*/
