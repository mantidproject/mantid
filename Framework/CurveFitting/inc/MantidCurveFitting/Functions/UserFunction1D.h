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
#include "MantidCurveFitting/Algorithms/Fit1D.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Deprecation notice: instead of using this algorithm please use the Fit algorithm
where the Function parameter of this algorithm is used
to specified the fitting function.

Fits a histogram in a 2D workspace to a user defined function.

Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace to take as input </LI>

<LI> SpectrumIndex - The spectrum to fit, using the workspace numbering of the
spectra (default 0)</LI>
<LI> StartX - X value to start fitting from (default start of the spectrum)</LI>
<LI> EndX - last X value to include in fitting range (default end of the
spectrum)</LI>
<LI> MaxIterations - Max iterations (default 500)</LI>
<LI> OutputStatus - whether the fit was successful. Direction::Output</LI>
<LI> OutputChi2overDoF - returns how good the fit was (default 0.0).
Direction::Output</LI>

<LI> Function - The user defined function. It must have x as its argument.</LI>
<LI> InitialParameters - A list of initial values for the parameters in the
function. It is a comma separated
        list of name=value items where name is the name of a parameter and value
is its initial vlaue.
        The name=value pairs can appear in any order in the list. The parameters
that are not set in this property
        will be given the default initial value of 0.0</LI>
<LI> Parameters - The output table workspace with the final values of the fit
parameters</LI>
<LI> OutputWorkspace - A matrix workspace to hold the resulting model spectrum,
the initial histogram and the difference
        between them</LI>
</UL>

@author Roman Tolchenov, Tessella plc
@date 17/6/2009
*/
class UserFunction1D : public Algorithms::Fit1D {
public:
  /// Constructor
  UserFunction1D() : m_x(0.0), m_x_set(false), m_parameters(100), m_nPars(0) {};
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "UserFunction1D"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Optimization\\FitAlgorithms"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Fits a histogram from a workspace to a user defined function."; }
  const std::vector<std::string> seeAlso() const override { return {"Fit"}; }

protected:
  /// overwrite base class methods
  // double function(const double* in, const double& x);
  void function(const double *in, double *out, const double *xValues, const size_t nData) override;
  void declareAdditionalProperties() override;
  void declareParameters() override {};
  void prepare() override;
  /// Derivatives of function with respect to parameters you are trying to fit
  void functionDeriv(const double *in, API::Jacobian *out, const double *xValues, const size_t nData) override;

  static double *AddVariable(const char *varName, void *palg);

private:
  /// muParser instance
  mu::Parser m_parser;
  /// Used as 'x' variable in m_parser.
  double m_x;
  /// True indicates that input formula contains 'x' variable
  bool m_x_set;
  /// Pointer to muParser variables' buffer
  std::vector<double> m_parameters;
  /// Number of actual parameters
  int m_nPars;
  /// Temporary data storage
  std::vector<double> m_tmp;
  /// Temporary data storage
  std::vector<double> m_tmp1;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
