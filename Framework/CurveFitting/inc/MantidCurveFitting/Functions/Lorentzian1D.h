#ifndef MANTID_CURVEFITTING_LORENTZIAN1D_H_
#define MANTID_CURVEFITTING_LORENTZIAN1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Algorithms/Fit1D.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Deprecation notice: instead of using this algorithm please use the Fit algorithm
where the Function parameter of this algorithm is used
to specified the fitting function.

Takes a histogram in a 2D workspace and fit it to a Lorentzian on top of
a linear background.
i.e. a function: Height*( HWHM^2/((x-PeakCentre)^2+HWHM^2) ) + BG0 + BG1*x

Properties specific to this derived class:
<UL>
<LI> BG0 - background intercept value (default 0.0)</LI>
<LI> BG1 - background slope value (default 0.0)</LI>
<LI> Height - height of peak (default 0.0)</LI>
<LI> PeakCentre - centre of peak (default 0.0)</LI>
<LI> HWHM - half-width half-maximum (default 1.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 24/5/2009

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport Lorentzian1D : public Algorithms::Fit1D {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Lorentzian1D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "== Deprecation notice == Instead of using this algorithm to fit a "
           "Lorentzian please use the Fit algorithm where the Function "
           "parameter of this algorithm is used to specified the fitting "
           "function, including selecting a Lorentzian.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Optimization\\FitAlgorithms";
  }

private:
  // Overridden Fit1D methods
  void declareParameters() override;
  void function(const double *in, double *out, const double *xValues,
                const size_t nData) override;
  void functionDeriv(const double *in, API::Jacobian *out,
                     const double *xValues, const size_t nData) override;
};

} // namespace Functions
} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LORENTZIAN1D_H_*/
