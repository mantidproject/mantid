// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IBACKGROUNDFUNCTION_H_
#define MANTID_API_IBACKGROUNDFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionWithLocation.h"

namespace Mantid {
namespace API {
/** An interface to a background function. This interface is just
    a copy of the IFunctionWithLocation interface for now.

    @author Anders Markvardsen, ISIS, RAL
    @date 2/11/2009
*/
class DLLExport IBackgroundFunction : public IFunctionWithLocation {
public:
  /// Fits the function
  /// @param X :: a vector of x values
  /// @param Y :: a matching vector of Y values
  virtual void fit(const std::vector<double> &X,
                   const std::vector<double> &Y) = 0;
};

using IBackgroundFunction_sptr = boost::shared_ptr<IBackgroundFunction>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IBACKGROUNDFUNCTION_H_*/
