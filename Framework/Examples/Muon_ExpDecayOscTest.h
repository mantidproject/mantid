// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
namespace Mantid {
namespace CurveFitting {
/**
This is an example of a peak shape function which is a combination
of an exponential decay and cos function.

This function was originally provided by the ISIS Muon group.

For a general description of how to create a test
fitting function see LorentzianTest.h and LorentzianTest.cpp
*/
class DLLExport Muon_ExpDecayOscTest : public API::IPeakFunction {
public:
  /// overwrite IPeakFunction base class methods

  double centre() const override { return getParameter("lambda"); }
  double height() const override { return getParameter("A"); }
  double fwhm() const override { return 1 / getParameter("frequency"); }
  void setCentre(const double c) override { setParameter("lambda", c); }
  void setHeight(const double h) override { setParameter("A", h); }
  void setFwhm(const double w) override { setParameter("frequency", 1 / w); }

  /// Here specify name of function as it will appear
  std::string name() const override { return "Muon_ExpDecayOscTest"; }

  // ** OPTIONALLY MODIFY THIS **
  /// The categories the Fit function belong to.
  /// Categories must be listed as a comma separated list.
  /// For example: "General, Muon\\Custom" which adds
  /// this function to the category "General" and the sub-category
  /// "Muon\\Custom"
  const std::string category() const override { return "C++ User Defined"; }
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &out) override;

  // ** OPTIONALLY MODIFY THIS **
  virtual const std::string summary() const {
    return "An example of a peak shape function which is a combination of an "
           "exponential decay and cos function.";
  }

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *, const double *, const size_t) override {}
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace CurveFitting
} // namespace Mantid
