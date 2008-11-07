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
    Takes a histogram in a 2D workspace and fit it to a Gaussian, i.e. a
    function: y0+A*sqrt(2/PI)/w*exp(-0.5*((x-xc)/w)^2).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> StartX - X bin number to integrate from (default 0)</LI>
    <LI> EndX - X bin number to integrate to (default max)</LI>
    <LI> SpectrumNumber - The spectrum to fit (default first spectrum in dataset)</LI>
    <LI> MaxIterations - The spectrum to fit (default 500)</LI>
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
    int gauss_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
    int gauss_df (const gsl_vector * x, void *params, gsl_matrix * J);
    int gauss_f (const gsl_vector * x, void *params,gsl_vector * f);


  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN_H_*/
