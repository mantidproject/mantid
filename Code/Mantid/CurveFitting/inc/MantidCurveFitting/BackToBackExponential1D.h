#ifndef MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL1D_H_
#define MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"


namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Takes a histogram in a 2D workspace and fit it to a back-to-back exponential
    peak function, that is the function:

      I*(exp(a/2*(a*s^2+2*(x-x0)))*erfc((a*s^2+(x-x0))/sqrt(2*s^2))+exp(b/2*(b*s^2-2*(x-x0)))*erfc((b*s^2-(x-x0))/sqrt(2*s^2)))+bk.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> SpectrumNumber - The spectrum to fit, using the workspace numbering of the spectra (default 0)</LI>
    <LI> StartX - X value to start fitting from (default 0.0)</LI>
    <LI> EndX - last X value to include in fitting range (default 1.0)</LI>
    <LI> I - height of peak (default 0.0)</LI>
    <LI> a - exponential constant of rising part of neutron pulse (default 0.0)</LI>
    <LI> b - exponential constant of decaying part of neutron pulse (default 0.0)</LI>
    <LI> x0 - peak position (default 0.0)</LI>
    <LI> s - standard deviation of gaussian part of peakshape function (default 1.0)</LI>
    <LI> bk - constant background (default 0.0)</LI>
    <LI> MaxIterations - The spectrum to fit (default 500)</LI>
    <LI> Output Status - whether the fit was successful. Direction::Output</LI>
    <LI> Output Chi^2/DoF - returns how good the fit was (default 0.0). Direction::Output</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 21/5/2009

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport BackToBackExponential1D : public Fit1D
    {
    public:
      /// Destructor
      virtual ~BackToBackExponential1D() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "BackToBackExponential1D";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

    private:
      // Overridden Fit1D methods
      void declareParameters();
      void function(double* in, double* out, double* xValues, double* yValues, double* yErrors, int nData);
      void functionDeriv(double* in, double* out, double* xValues, double* yValues, double* yErrors, int nData);

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };


  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL1D_H_*/
