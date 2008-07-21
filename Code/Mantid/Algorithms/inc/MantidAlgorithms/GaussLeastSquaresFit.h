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

      /// Gaussian function and derivative function in GSL format
      // defined as friend do get access to function pointers
      friend int gauss_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
      friend int gauss_df (const gsl_vector * x, void *params, gsl_matrix * J);
      friend int gauss_f (const gsl_vector * x, void *params,gsl_vector * f);

      /// function which guesses initial parameter values
      void guessInitialValues(const FitData& data, gsl_vector* param_init);

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };


    
    /** Gaussian function in GSL format
* @param x Input function arguments  
* @param params Input data
* @param f Output function value
* @return A GSL status information
*/
    /*
int gauss_f (const gsl_vector * x, void *params, gsl_vector * f) {
    size_t n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double Y0 = gsl_vector_get (x, 0);
    double A = gsl_vector_get (x, 1);
    double C = gsl_vector_get (x, 2);
    double w = gsl_vector_get (x, 3);
    size_t i;
    for (i = 0; i < n; i++) {
        double diff=X[i]-C;
        double Yi = A*exp(-0.5*diff*diff/(w*w))+Y0;
        gsl_vector_set (f, i, (Yi - Y[i])/sigma[i]);
    }
    return GSL_SUCCESS;
}*/

/** Calculates Gaussian derivatives in GSL format
* @param x Input function arguments  
* @param params Input data
* @param J Output derivatives
* @return A GSL status information
*/
/*
int gauss_df (const gsl_vector * x, void *params,
              gsl_matrix * J) 
{
    size_t n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double A = gsl_vector_get (x, 1);
    double C = gsl_vector_get (x, 2);
    double w = gsl_vector_get (x, 3);
    size_t i;
    for (i = 0; i < n; i++) {
        // Jacobian matrix J(i,j) = dfi / dxj,	 
        // where fi = Yi - yi,					
        // Yi = y=A*exp[-(Xi-xc)^2/(2*w*w)]+B		
        // and the xj are the parameters (B,A,C,w) 
        double s = sigma[i];
        double diff = X[i]-C;
        double e = exp(-0.5*diff*diff/(w*w))/s;
        gsl_matrix_set (J, i, 0, 1/s);
        gsl_matrix_set (J, i, 1, e);
        gsl_matrix_set (J, i, 2, diff*A*e/(w*w));
        gsl_matrix_set (J, i, 3, diff*diff*A*e/(w*w*w));
    }
    return GSL_SUCCESS;
} */

/** Calculates Gaussian derivatives and function value in GSL format
* @param x Input function arguments  
* @param params Input data
* @param f Output function value
* @param J Output derivatives
* @return A GSL status information
*/

/*int gauss_fdf (const gsl_vector * x, void *params,
               gsl_vector * f, gsl_matrix * J) {
    gauss_f (x, params, f);
    gauss_df (x, params, J);
    return GSL_SUCCESS;
} */



  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GAUSSLEASTSQUARESFIT_H_*/
