#ifndef MANTID_INELASTICDIFFSPHERE_H_
#define MANTID_INELASTICDIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "DeltaFunction.h"

namespace Mantid
{
namespace CurveFitting
{
  /**
  @author Jose Borreguero, NScD
  @date 11/14/2011

  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
struct xnlc{
  double x;
  unsigned int l;
  unsigned int n;
};

/// simple structure to hold a linear interpolation of factor J around its numerical divergence point
struct linearJ{
  double slope;
  double intercept;
};

/* Class representing the inelastic portion of the DiffSphere algorithm.
 * Contains the 98 Lorentzians.
 */
class DLLExport InelasticDiffSphere : public API::ParamFunction, public API::IFunction1D
{
public:

  InelasticDiffSphere();
  virtual ~InelasticDiffSphere() {}

  virtual std::string name()const{return "InelasticDiffSphere";}
  virtual const std::string category() const { return "QuasiElastic";}

  void calNumericalDeriv2(const API::FunctionDomain& domain, API::Jacobian& out);

protected:
  virtual void function1D(double* out, const double* xValues, const size_t nData)const;
  virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
  virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);
  std::vector<double> LorentzianCoefficients(double a) const;

private:

  /// initialize the Xnl coefficients
  void initXnlCoeff();

  /// initialize the alpha coefficients
  void initAlphaCoeff();

  /// initialize the list of Linearized J values
  void initLinJlist();

  /// xnl coefficients
  std::vector<xnlc> xnl;

  /// certain coefficients invariant during fitting
  std::vector<double> alpha;

  /// maximum value of l in xnlist
  unsigned int lmax;

  /// number of coefficients
  unsigned int ncoeff;

  /// linear interpolation zone around the numerical divergence of factor J
  double m_divZone;

  /// list of linearized J values
  std::vector<linearJ> linearJlist;

}; // end of class InelasticDiffSphere

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_INELASTICDIFFSPHERE_H_*/
