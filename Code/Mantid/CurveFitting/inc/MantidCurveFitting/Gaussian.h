#ifndef MANTID_CURVEFITTING_GAUSSIAN_H_
#define MANTID_CURVEFITTING_GAUSSIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <gsl/gsl_matrix.h>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Takes a histogram in a 2D workspace and fit it to a Gaussian atop a flat background.
    i.e. a function: bg0+height*exp(-0.5*((x-peakCentre)/sigma)^2),.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> SpectrumIndex - The spectrum to fit, using the workspace numbering of the spectra (default 0)</LI>

    <LI> peakCentre - centre of peak (default 0.0)</LI>
    <LI> sigma - standard deviation (default 1.0)</LI>
    <LI> height - height of peak (default 0.0)</LI>
    <LI> bg0 - constant background value (default 0.0)</LI>

    <LI> StartX - X value to start fitting from (default to -6*sigma away from the peakCentre)</LI>
    <LI> EndX - last X value to include in fitting range (default to +6*sigma away from the peakCentre)</LI>

    <LI> MaxIterations - Max iterations (default 500)</LI>

    <LI> Output Status - whether fit successful. Direction::Output</LI>
    <LI> Output Chi^2/DoF - measure of how good the fit is (default 0.0). Direction::Output</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 04/7/2008

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
    class DLLExport Gaussian : public API::Algorithm
    {
    public:
      /// Default constructor
      Gaussian() : API::Algorithm() {};
      /// Destructor
      virtual ~Gaussian() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Gaussian";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();

      /// The X bin to start the fitting from
      int m_minX;
      /// The X bin to finish the fitting at
      int m_maxX;

      // function which guesses initial parameter values
      // This method need further work done to it.
      //void guessInitialValues(const FitData& data, gsl_vector* param_init);

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

    /// Gaussian function and derivative function in GSL format
    // defined as friend do get access to function pointers
    //int gauss_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
    //int gauss_df (const gsl_vector * x, void *params, gsl_matrix * J);
    //int gauss_f (const gsl_vector * x, void *params,gsl_vector * f);

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN_H_*/
