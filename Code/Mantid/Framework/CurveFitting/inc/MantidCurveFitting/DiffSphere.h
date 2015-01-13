#ifndef MANTID_DIFFSPHERE_H_
#define MANTID_DIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DeltaFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "DeltaFunction.h"

namespace Mantid {
namespace CurveFitting {
/**
@author Jose Borreguero, NScD
@date 11/14/2011

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

class DLLExport ElasticDiffSphere : public DeltaFunction {
public:
  /// Constructor
  ElasticDiffSphere();

  /// Destructor
  virtual ~ElasticDiffSphere(){};

  /// overwrite IFunction base class methods
  virtual std::string name() const { return "ElasticDiffSphere"; }

  virtual const std::string category() const { return "QuasiElastic"; }

  /// A rescaling of the peak intensity
  double HeightPrefactor() const;

  /// overwrite IFunction base class method, which declare function parameters
  virtual void init();
};

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

/* Class representing the inelastic portion of the DiffSphere algorithm.
 * Contains the 98 Lorentzians.
 */
class DLLExport InelasticDiffSphere : public API::ParamFunction,
                                      public API::IFunction1D {
public:
  InelasticDiffSphere();

  virtual ~InelasticDiffSphere() {}

  /// overwrite IFunction base class methods
  virtual void init();

  /// overwrite IFunction base class methods
  virtual std::string name() const { return "InelasticDiffSphere"; }

  /// overwrite IFunction base class methods
  virtual const std::string category() const { return "QuasiElastic"; }

  /// Calculate the (2l+1)*A_{n,l} coefficients for each Lorentzian
  std::vector<double> LorentzianCoefficients(double a) const;

protected:
  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const;

private:
  /// initialize the Xnl coefficients
  void initXnlCoeff();

  /// initialize the m_alpha coefficients
  void initAlphaCoeff();

  /// initialize the list of parameters for A_{n,l} linear interpolation around
  /// the indeterminacy point
  void initLinJlist();

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

}; // end of class InelasticDiffSphere

class DLLExport DiffSphere : public API::ImmutableCompositeFunction {

public:
  /// Destructor
  ~DiffSphere(){};

  /// overwrite IFunction base class methods
  std::string name() const { return "DiffSphere"; }

  /// overwrite IFunction base class methods
  virtual const std::string category() const { return "QuasiElastic"; }

  /// overwrite IFunction base class methods
  virtual int version() const { return 1; }

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute(const std::string &name);

  /// Override parent definition
  virtual void declareAttribute(const std::string &name,
                                const API::IFunction::Attribute &defaultValue);

  /// Override parent definition
  virtual void setAttribute(const std::string &attName, const Attribute &att);

  /// overwrite IFunction base class method, which declare function parameters
  virtual void init();

private:
  boost::shared_ptr<ElasticDiffSphere>
      m_elastic; // elastic intensity of the DiffSphere structure factor
  boost::shared_ptr<InelasticDiffSphere>
      m_inelastic; // inelastic intensity of the DiffSphere structure factor
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFSPHERE_H_*/
