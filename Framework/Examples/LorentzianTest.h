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
This is a copy of the Lorentzian peak shape function in Mantid.

This copy is here used to demonstrate how to add a user defined Fit Function.

The instructions here complement the instruction at
<http://http://www.mantidproject.org/Writing_a_Fit_Function>

1) Rename LorentzianTest.h and LorentzianTest.cpp to the name of your new Fit
Function, say MyFit.h and MyFit.cpp
2) Open MyFit.h and substitute all instances of LORENTZIANTEST with MYFIT and
LorentzianTest with MyFit.
3) Open MyFit.cpp and substitute LorentzianTest with MyFit
4) Check that MyFit works, see
<http://http://www.mantidproject.org/Writing_a_Fit_Function> for instructions
5) Now the fun bit, modify the sections in MyFit.h and MyFit.cpp entitled **
MODIFY THIS **


Note for information about the Mantid implementation of the Lorentzian Fit
Function see
<http://http://www.mantidproject.org/Lorentzian>. It has three fitting
parameters and its formula is: Height*( HWHM^2/((x-PeakCentre)^2+HWHM^2) ).

In more detail the Lorentzian fitting parameters are:
<UL>
<LI> Height - height of peak (default 0.0)</LI>
<LI> PeakCentre - centre of peak (default 0.0)</LI>
<LI> HWHM - half-width half-maximum (default 0.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 21/11/2010
*/
class DLLExport LorentzianTest : public API::IPeakFunction {
public:
  // ** MODIFY THIS **
  // These methods are used by the MantidPlot GUI to allow users to
  // graphically adjust the e.g. the peak width and translate such an
  // adjustments to width fit parameter(s)
  //
  // This implementation of a Lorentzian has three fitting parameters
  // PeakCentre, Height and HWHM as described above and declared in
  // the init() method in the .cpp file
  //
  // From the fitting parameters of your function use these to give
  // a best estimate of how to calculate the centre (in the case of
  // an exponential function this might be set to the start value of
  // the function), the height and width. Note these should just be
  // best estimates and is only used Fit GUI for given a better user
  // feel when using your fit function
  //
  // Initially you may set these as follows:
  //
  //       virtual double centre()const {return 0.0;}
  //       virtual double height()const {return 1.0;}
  //       virtual double width()const {return 0.0;}
  //       virtual void setCentre(const double c) {}
  //       virtual void setHeight(const double h) {}
  //       virtual void setWidth(const double w) {}
  //
  // I.e. replace the below 6 lines of code with the above 6 lines
  double centre() const override { return getParameter("PeakCentre"); }
  double height() const override { return getParameter("Height"); }
  double fwhm() const override { return 2 * getParameter("HWHM"); }
  void setCentre(const double c) override { setParameter("PeakCentre", c); }
  void setHeight(const double h) override { setParameter("Height", h); }
  void setFwhm(const double w) override { setParameter("HWHM", w / 2.0); }

  // ** MODIFY THIS **
  /// The name of the fitting function
  std::string name() const override { return "LorentzianTest"; }

  // ** OPTIONALLY MODIFY THIS **
  /// The categories the Fit function belong to.
  /// Categories must be listed as a comma separated list.
  /// For example: "General, Muon\\Custom" which adds
  /// this function to the category "General" and the sub-category
  /// "Muon\\Custom"
  const std::string category() const override { return "C++ User Defined"; }

  virtual const std::string summary() const { return "C++ User defined algorithm."; }

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;
  void init() override;
};

} // namespace CurveFitting
} // namespace Mantid
