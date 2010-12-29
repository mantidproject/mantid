#ifndef MANTID_CURVEFITTING_LINEAR_H_
#define MANTID_CURVEFITTING_LINEAR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace CurveFitting
{
/** Performs a least-squares fit of a spectrum to the function Y(c,x) = c0 + c1*X.
    Uses the gsl linear regression algorithms, which are documented at: 
    <http://www.gnu.org/software/gsl/manual/html_node/Linear-regression.html>

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the Workspace containing the spectrum to fit </LI>
    <LI> OutputWorkspace - A 1D workspace containing the fit result & errors for the X of the input spectrum </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> SpectrumIndex - The spectrum to fit, using the workspace numbering of the spectra (default 0) </LI>
    <LI> StartX        - X value to start fitting from (default min) </LI>
    <LI> EndX          - last X value to include in fitting range (default max) </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> FitStatus    - Whether the fit succeeded (empty string if so, error message otherwise)
    <LI> FitIntercept - The c0 in the equation above </LI>
    <LI> FitSlope     - The slope of the fit; c1 in the equation above </LI>
    <LI> Chi2         - The chi^2 of the fit </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 21/01/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport Linear : public API::Algorithm
{
public:
  /// Constructor
  Linear();
  /// Virtual destructor
  virtual ~Linear();
  /// Algorithm's name
  virtual const std::string name() const { return "Linear"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "CurveFitting"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  void setRange(const MantidVec& X, const MantidVec& Y); 
  
  int m_minX;                ///< The X bin to start the fitting from
  int m_maxX;                ///< The X bin to finish the fitting at
  API::Progress *m_progress; ///< Progress reporting object

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LINEAR_H_*/
