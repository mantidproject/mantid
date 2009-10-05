#ifndef FITPEAKEXAMPLELORENTZIANUSEDERIVATIVES_H_
#define FITPEAKEXAMPLELORENTZIANUSEDERIVATIVES_H_

#include "MantidCurveFitting/Fit1D.h"

namespace Mantid
{
namespace CurveFitting
{
/** An example algorithm illustrating how to add a fitting function, in 
    particular here the LorentzianUseDerivatives peakshape function as described 
    http://www.mantidproject.org/LorentzianUseDerivatives1D. This implementation does
    not uses derivatives, hence uses the simplex minimisation method
    for miminisation. For an example with uses derivatives see 
    FitPeakExampleLorentzianUseDerivativesUseDerivatives.h/.cpp. In general you may
    expect the derivative implementation of a fitting function to more
    quickly locate a local minimum. 

    @author Anders J Markvardsen, ISIS, RAL
    @date 02/10/2009

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class FitPeakExampleLorentzianUseDerivatives : public Fit1D
{
public:
  /// Virtual destructor
  virtual ~FitPeakExampleLorentzianUseDerivatives() {}
  /// Algorithm's name
  virtual const std::string name() const { return "FitPeakExampleLorentzianUseDerivatives"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "CurveFitting"; }

private:
  // Overridden Fit1D methods
  void declareParameters();
  void function(const double* in, double* out, const double* xValues, const int& nData);
  void functionDeriv(const double* in, Jacobian* out, const double* xValues, const int& nData);

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*FITPEAKEXAMPLELORENTZIANUSEDERIVATIVES_H_*/
