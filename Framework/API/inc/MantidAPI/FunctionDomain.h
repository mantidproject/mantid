// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_FUNCTIONDOMAIN_H_
#define MANTID_API_FUNCTIONDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
//#include "MantidKernel/PropertyManager.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <stdexcept>

namespace Mantid {
namespace API {
/** Base class that represents the domain of a function.
    It is a generalisation of function arguments.
    A domain consists at least of a list of function arguments for which a
   function (IFunction) should
    be evaluated.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_API_DLL FunctionDomain //: public Kernel::PropertyManager
{
public:
  /// Virtual destructor
  virtual ~FunctionDomain() = default;
  /// Return the number of points in the domain
  virtual size_t size() const = 0;
  /// Reset the the domain so it can be reused. Implement this method for
  /// domains with a state.
  virtual void reset() const {}
};

/// typedef for a shared pointer
using FunctionDomain_sptr = boost::shared_ptr<FunctionDomain>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONDOMAIN_H_*/
