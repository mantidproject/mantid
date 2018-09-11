#ifndef MANTID_INELASTICDIFFSPHERE_H_
#define MANTID_INELASTICDIFFSPHERE_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/DeltaFunction.h"
// Mantid headers from other projects
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"
// 3rd party library headers (N/A)
// standard library headers (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 25/06/2016

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

/// structure to hold info on Volino's coefficients
struct xnlc {
  double x;
  size_t n;
  size_t l;
};

/// simple structure to hold a linear interpolation of factor J around its
/// numerical divergence point
struct linearJ {
  double slope;
  double intercept;
};

/**
 * @brief Inelastic part of the DiffSphere function. Contains the 98
 * Lorentzians.
 */
class DLLExport InelasticDiffSphere : public API::ParamFunction,
                                      public API::IFunction1D {
public:
  InelasticDiffSphere();

  /// overwrite IFunction base class methods
  void init() override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "InelasticDiffSphere"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "QuasiElastic"; }

  /// Calculate the (2l+1)*A_{n,l} coefficients for each Lorentzian
  std::vector<double> LorentzianCoefficients(double a) const;

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

private:
  /// initialize the Xnl coefficients
  void initXnlCoeff();

  /// initialize the m_alpha coefficients
  void initAlphaCoeff();

  /// initialize the list of parameters for A_{n,l} linear interpolation around
  /// the indeterminacy point
  void initLinJlist();

  /// Cache Q values from the workspace
  void setWorkspace(boost::shared_ptr<const API::Workspace> ws) override;

  /// xnl coefficients
  std::vector<xnlc> m_xnl;

  /// certain coefficients invariant during fitting
  std::vector<double> m_alpha;

  /// maximum value of l in xnlist
  size_t m_lmax;

  /// linear interpolation zone around the numerical divergence of factor J
  double m_divZone;

  /// Plank's constant divided by \f$2\pi\f$, in units of meV*THz
  double m_hbar;

  /// list of linearized J values
  std::vector<linearJ> m_linearJlist;

  /// list of calculated Q values
  std::vector<double> m_qValueCache;

}; // end of class InelasticDiffSphere

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_INELASTICDIFFSPHERE_H_
