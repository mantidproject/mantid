#ifndef MANTID_ALGORITHM_GAUSSLEASTSQUARESFIT_H_
#define MANTID_ALGORITHM_GAUSSLEASTSQUARESFIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

#include <gsl/gsl_matrix.h>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
//namespace GSL {
	struct FitData; // redefined in .cpp file
//}
/// @endcond

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    Takes a histogram in a 2D workspace and fit it to a Gaussian, i.e. a 
    function: y0+A*sqrt(2/PI)/w*exp(-2*((x-xc)/w)^2).

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

    /// Structure to contain least squares data

    //struct FitData; // redefined in .cpp file
    /// Structure to contain least squares data
//namespace GSL {
  struct FitData {
    size_t n; // number of points to be fitted (size of X, Y and sigma arrays)
    size_t p; // number of fit parameters
    double * X; // the data to be fitted (abscissae) 
    double * Y; // the data to be fitted (ordinates)
    double * sigma; // the weighting data
  };
//}
    

    class DLLExport GaussLeastSquaresFit : public API::Algorithm
    {
    public:
      /// Default constructor
      GaussLeastSquaresFit() : API::Algorithm() {};
      /// Destructor
      virtual ~GaussLeastSquaresFit() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "GaussLeastSquaresFit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}
  
    private:
      // Overridden Algorithm methods
      void init();
      void exec();

      /// The spectrum to fit
      int m_spectrumNumber;
      /// The X bin to start the fitting from
      int m_minX;
      /// The X bin to finish the fitting at
      int m_maxX;

      /// function which guesses initial parameter values
      void guessInitialValues(const FitData& data, gsl_vector* param_init);

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

#endif /*MANTID_ALGORITHM_GAUSSLEASTSQUARESFIT_H_*/
