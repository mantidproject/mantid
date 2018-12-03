// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_DELTAFUNCTION_H_
#define MANTID_CURVEFITTING_DELTAFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/System.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**

Delta function. Makes sence in Convolution only.

@author Roman Tolchenov, Tessella plc
@date 02/09/2010
*/
class DLLExport DeltaFunction : public API::IPeakFunction {
public:
  /// Constructor
  DeltaFunction();

  /// overwrite IPeakFunction base class methods
  double centre() const override { return 0; }
  double height() const override { return getParameter("Height"); }
  double fwhm() const override { return 0; }
  void setCentre(const double c) override { UNUSED_ARG(c); }
  void setHeight(const double h) override { setParameter("Height", h); }
  void setFwhm(const double w) override { UNUSED_ARG(w); }

  /// Return the peak FWHM uncertainty
  double fwhmUncertainty() const override;
  /// Return the peak intensity uncertainty
  virtual double intensityUncertainty() const override;
  /// Return the peak height uncertainty
  virtual double heightUncertainty() const override;

  virtual double HeightPrefactor() const {
    return 1.0;
  } // modulates the Height of the Delta function
  /// overwrite IFunction base class methods
  std::string name() const override { return "DeltaFunction"; }
  const std::string category() const override { return "Peak"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  void functionDerivLocal(API::Jacobian *out, const double *xValues,
                          const size_t nData) override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_DELTAFUNCTION_H_*/
