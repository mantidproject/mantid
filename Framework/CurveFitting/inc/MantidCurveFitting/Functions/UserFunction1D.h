#ifndef MANTID_CURVEFITTING_USERFUNCTION1D_H_
#define MANTID_CURVEFITTING_USERFUNCTION1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Algorithms/Fit1D.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/shared_array.hpp>

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

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class UserFunction1D : public Algorithms::Fit1D {
public:
  /// Constructor
  UserFunction1D()
      : m_x(0.0), m_x_set(false), m_parameters(new double[100]), m_nPars(0){};
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "UserFunction1D"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Optimization\\FitAlgorithms";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fits a histogram from a workspace to a user defined function.";
  }
  const std::vector<std::string> seeAlso() const override { return {"Fit"}; }

protected:
  /// overwrite base class methods
  // double function(const double* in, const double& x);
  void function(const double *in, double *out, const double *xValues,
                const size_t nData) override;
  void declareAdditionalProperties() override;
  void declareParameters() override{};
  void prepare() override;
  /// Derivatives of function with respect to parameters you are trying to fit
  void functionDeriv(const double *in, API::Jacobian *out,
                     const double *xValues, const size_t nData) override;

  static double *AddVariable(const char *varName, void *palg);

private:
  /// muParser instance
  mu::Parser m_parser;
  /// Used as 'x' variable in m_parser.
  double m_x;
  /// True indicates that input formula contains 'x' variable
  bool m_x_set;
  /// Pointer to muParser variables' buffer
  boost::shared_array<double> m_parameters;
  /// Number of actual parameters
  int m_nPars;
  /// Temporary data storage
  boost::shared_array<double> m_tmp;
  /// Temporary data storage
  boost::shared_array<double> m_tmp1;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN1D_H_*/
