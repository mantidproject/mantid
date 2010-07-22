#ifndef FITPEAKEXAMPLELORENTZIAN_H_
#define FITPEAKEXAMPLELORENTZIAN_H_

#include "MantidCurveFitting/Fit1D.h"

namespace Mantid
{
namespace CurveFitting
{
/** An example algorithm illustrating how to add a fitting function, here 
    the Lorentzian peakshape function:

      Height*( HWHM^2/((x-PeakCentre)^2+HWHM^2) ) + BG0 + BG1*x

    Where the parameters means the following:
    <UL>
    <LI> BG0 - background intercept value </LI>
    <LI> BG1 - background slope value )</LI>
    <LI> Height - height of peak </LI>
    <LI> PeakCentre - centre of peak </LI>
    <LI> HWHM - half-width half-maximum </LI>
    </UL>

    This implementation does not uses derivatives. For an example with does 
    not uses derivatives see FitPeakExampleLorentzianUseDerivatives.h/.cpp. 
    In general you may expect the derivative implementation of a fitting function 
    to be faster at locating the minimum. 

    @author Anders J Markvardsen, ISIS, RAL
    @date 02/10/2009

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class FitPeakExampleLorentzian : public Fit1D
{
public:
  /// Virtual destructor
  virtual ~FitPeakExampleLorentzian() {}
  /// Algorithm's name
  virtual const std::string name() const { return "FitPeakExampleLorentzian"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "CurveFitting"; }

private:
  // Overridden Fit1D methods
  void declareParameters();
  void function(const double* in, double* out, const double* xValues, const int& nData);

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*FITPEAKEXAMPLELORENTZIAN_H_*/
