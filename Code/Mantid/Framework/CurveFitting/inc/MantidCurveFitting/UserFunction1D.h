#ifndef MANTID_CURVEFITTING_USERFUNCTION1D_H_
#define MANTID_CURVEFITTING_USERFUNCTION1D_H_
/*WIKI* 

This algorithm fits a spectrum to a user defined function. The function is supplied to the algorithm as a text string. The function here is a mathematical expression using numbers, variable names and internal function names. Symbols '+', '-', '*', '/', and '^' can be used for arithmetic operations. Names can contain only letters, digits, and the underscore symbol '_'. The internal functions are: 
{|
!Name 
!Argc. 
!Explanation 
|-
|sin || 1 || sine function 
|-
|cos || 1 || cosine function 
|-
|tan || 1 || tangens function 
|-
|asin || 1 || arcus sine function 
|-
|acos || 1 || arcus cosine function 
|-
|atan || 1 || arcus tangens function 
|-
|sinh || 1 || hyperbolic sine function 
|-
|cosh || 1 || hyperbolic cosine 
|-
|tanh || 1 || hyperbolic tangens function 
|-
|asinh || 1 || hyperbolic arcus sine function 
|-
|acosh || 1 || hyperbolic arcus tangens function 
|-
|atanh || 1 || hyperbolic arcur tangens function 
|-
|log2 || 1 || logarithm to the base 2 
|-
|log10 || 1 || logarithm to the base 10 
|-
|log || 1 || logarithm to the base 10 
|-
|ln || 1 || logarithm to base e (2.71828...) 
|-
|exp || 1 || e raised to the power of x 
|-
|sqrt || 1 || square root of a value 
|-
|sign || 1 || sign function -1 if x<0; 1 if x>0 
|-
|rint || 1 || round to nearest integer 
|-
|abs || 1 || absolute value 
|-
|if || 3 || if ... then ... else ... 
|-
|min || var. || min of all arguments 
|-
|max || var. || max of all arguments 
|-
|sum || var. || sum of all arguments 
|-
|avg || var. || mean value of all arguments 
|}

An example of ''Function'' property is "a + b*x + c*x^2". Valiable ''x'' is used to represent the values of the X-vector of the input spectrum. All other variable names are treated as fitting parameters. A parameter can be given an initial value in the ''InitialParameters'' property. For example, "b=1, c=0.2". The order in which the variables are listed is not important. If a variable is not given a value, it is initialized with 0.0. If some of the parameters should be fixed in the fit list them in the ''Fix'' property in any order, e.g. "a,c".

The resulting parameters are returned in a [[TableWorkspace]] set in ''OutputParameters'' property. Also for displaying purposes ''OutputWorkspace'' is returned. It contains the initial spectrum, the fitted spectrum and their difference.

== Example ==
[[Image:UserFunction1D.gif]]

In this example the fitting function is a*exp(-(x-c)^2*s). The parameter ''s'' is fixed.

*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/shared_array.hpp>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Deprecation notice: instead of using this algorithm please use the Fit algorithm 
    where the Function parameter of this algorithm is used 
    to specified the fitting function. 

    Fits a histogram in a 2D workspace to a user defined function.

    Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>

    <LI> SpectrumIndex - The spectrum to fit, using the workspace numbering of the spectra (default 0)</LI>
    <LI> StartX - X value to start fitting from (default start of the spectrum)</LI>
    <LI> EndX - last X value to include in fitting range (default end of the spectrum)</LI>
    <LI> MaxIterations - Max iterations (default 500)</LI>
    <LI> OutputStatus - whether the fit was successful. Direction::Output</LI>
    <LI> OutputChi2overDoF - returns how good the fit was (default 0.0). Direction::Output</LI>

    <LI> Function - The user defined function. It must have x as its argument.</LI>
    <LI> InitialParameters - A list of initial values for the parameters in the function. It is a comma separated
            list of name=value items where name is the name of a parameter and value is its initial vlaue.
            The name=value pairs can appear in any order in the list. The parameters that are not set in this property
            will be given the default initial value of 0.0</LI>
    <LI> Parameters - The output table workspace with the final values of the fit parameters</LI>
    <LI> OutputWorkspace - A matrix workspace to hold the resulting model spectrum, the initial histogram and the difference
            between them</LI>
    </UL>

    @author Roman Tolchenov, Tessella plc
    @date 17/6/2009

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
      class UserFunction1D : public Fit1D
      {
      public:
          /// Constructor
          UserFunction1D():m_x_set(false),m_parameters(new double[100]),m_buffSize(100),m_nPars(0) {};
          /// Destructor
          virtual ~UserFunction1D() {};
          /// Algorithm's name for identification overriding a virtual method
          virtual const std::string name() const { return "UserFunction1D";}
          /// Algorithm's version for identification overriding a virtual method
          virtual int version() const { return (1);}
          /// Algorithm's category for identification overriding a virtual method
          virtual const std::string category() const { return "CurveFitting";}


      protected:
          /// overwrite base class methods
          //double function(const double* in, const double& x);
          virtual void function(const double* in, double* out, const double* xValues, const size_t nData);
          virtual void declareAdditionalProperties();
          virtual void declareParameters(){};
          virtual void prepare();
          /// Derivatives of function with respect to parameters you are trying to fit
          virtual void functionDeriv(const double* in, Jacobian* out, const double* xValues, const size_t nData);

          static double* AddVariable(const char *varName, void *palg);

      private:
        /// Sets documentation strings for this algorithm
        virtual void initDocs();
          /// muParser instance
          mu::Parser m_parser;
          /// Used as 'x' variable in m_parser.
          double m_x;
          /// True indicates that input formula contains 'x' variable
          bool m_x_set;
          /// Pointer to muParser variables' buffer
          boost::shared_array<double> m_parameters;
          /// Size of the variables' buffer
          const int m_buffSize;
          /// Number of actual parameters
          int m_nPars;
          /// Temporary data storage
          boost::shared_array<double> m_tmp;
          /// Temporary data storage
          boost::shared_array<double> m_tmp1;
          
      };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN1D_H_*/
