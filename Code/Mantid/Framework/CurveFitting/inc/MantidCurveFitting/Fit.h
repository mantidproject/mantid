#ifndef MANTID_CURVEFITTING_FIT_H_
#define MANTID_CURVEFITTING_FIT_H_
/*WIKI* 


This algorithm fits a spectrum in a [[Workspace2D]] with a function. The function and the initial values for its parameters are set with the Function property. A function can be simple or composite. A [[:Category:Fit_functions|simple function]] has a name registered with Mantid framework. The Fit algorithm creates an instance of a function by this name. A composite function is an arithmetic sum of two or more simple functions. Each function has a number of named parameters, the names are case sensitive. All function parameters will be used in the fit unless some of them are tied. Parameters can be tied by setting the Ties property. A tie is a mathematical expression which is used to calculate the value of a (dependent) parameter. Only the parameter names of the same function can be used as variables in this expression.

Using the Minimizer property, Fit can be set to use different algorithms to perform the minimization. By default if the function's derivatives can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer. If the function's derivatives cannot be evaluated the GSL Simplex minimizer is used. Also, if one minimizer fails, for example the Levenberg-Marquardt minimizer, Fit may try its luck with a different minimizer. If this happens the user is notified about this and the Minimizer property is updated accordingly.

In Mantidplot this algorithm can be run from the [[MantidPlot:_Data Analysis and Curve Fitting#Simple Peak Fitting with the Fit Wizard|Fit Property Browser]] which allows all the settings to be specified via its graphical user interface.

===Setting a simple function===

To use a simple function for a fit set its name and initial parameter values using the Function property. This property is a comma separated list of name=value pairs. The name of the first name=value pairs must be "name" and it must be set equal to the name of one of a [[:Category:Fit_functions|simple function]]. This name=value pair is followed by name=value pairs specifying values for the parameters of this function. If a parameter is not set in Function it will be given its default value defined by the function. All names are case sensitive. For example for fitting a Gaussian the Function property might look like this:

 Function: "name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5"

Some functions have attributes. An attribute is a non-fitting parameter and can be of one of the following types: text string, integer, or double. Attributes are set just like the parameters using name=value pairs. For example:

 Function: "name=UserFunction, Formula=a+b*x, a=1, b=2"

In this example Formula is the name of a string attribute which defines an expression for the user UserFunction. The fitting parameters a and b are created when the Formula attribute is set. It is important that Formula is defined before initializing the parameters.

A list of the available simple functions can be found [[:Category:Fit_functions|here]].

===Setting a composite function===

A composite function is a sum of simple functions. It does not have a name. To define a composite function set a number of simple functions in the Function property. Each simple function definition must be separated by a semicolon ';'. For example fitting two Gaussians on a linear background might look like this:

 Function: "name=LinearBackground, A0=0.3; 
            name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5;
            name=Gaussian, PeakCentre=7.6, Height=8, Sigma=0.5"

===Setting ties===

Parameters can be tied to other parameters or to a constant. In this case they do not take part in the fitting but are evaluated using the tying expressions. Use Ties property to set any ties. In case of a simple function the parameter names are used as variables in the tying expressions. For example

 Ties: "a=2*b+1, c=2"

This ties parameter "a" to parameter "b" and fixes "c" to the constant 2.

In case of a composite function the variable name must refer to both the parameter name and the simple function it belongs to. It is done by writing the variable name in the following format: 
 f<index>.<name>
The format consists of two parts separated by a period '.'. The first part defines the function by its index in the composite function (starting at 0). The index corresponds to the order in which the functions are defined in the Function property. For example:

 Ties: "f1.Sigma=f0.Sigma,f2.Sigma=f0.Sigma"

This ties parameter "Sigma" of functions 1 and 2 to the "Sigma" of function 0. Of course all three functions must have a parameter called "Sigma" for this to work. The last example can also be written

 Ties: "f1.Sigma=f2.Sigma=f0.Sigma"

===Setting constraints===

Parameters can be constrained to be above a lower boundary and/or below an upper boundary. If a constraint is violated a penalty to the fit is applied which should result the parameters satisfying the constraint. The penalty applied is described in more detail [[FitConstraint|here]]. Use Constraints property to set any constraints. In case of a simple function the parameter names are used as variables in the constraint expressions. For example

 Constraints: "4.0 < c < 4.2"

Constraint the parameter "c" to be with the range 4.0 to 4.2, whereas 

 Constraints: "c > 4.0"

means "c" is constrained to be above the lower value 4.0 and 

 Constraints: "c < 4.2"

means "c" is constrained to be below the upper value 4.2.

In case of a composite function the same notation is used for constraints and for ties. For example

 Constraints: "f1.c < 4.2"

constrain the parameter "c" of function 1.

===Output===

Setting the Output property defines the names of the two output workspaces. One of them is a [[TableWorkspace]] with the fitted parameter values. The other is a [[Workspace2D]] which compares the fit with the original data. It has three spectra. The first (index 0) contains the original data, the second one the data simulated with the fitting function and the third spectrum is the difference between the first two. For example, if the Output was set to "MyResults" the parameter TableWorkspace will have name "MyResults_Parameters" and the Workspace2D will be named "MyResults_Workspace". If the function's derivatives can be evaluated an additional TableWorkspace is returned. When the Output is set to "MyResults" this TableWorkspace will have the name "MyResults_NormalisedCovarianceMatrix" and it returns a calculated correlation matrix. Denote this matrix C and its elements Cij then the diagonal elements are listed as 1.0 and the off diagnonal elements as percentages of correlation between parameter i and j equal to 100*Cij/sqrt(Cii*Cjj).

==Examples==

This example shows a simple fit to a Gaussian function. The algorithm properties are:

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=Gaussian, PeakCentre=4, Height=1.3, Sigma=0.5
 Output:          res

[[Image:GaussianFit.jpg]]

----

The next example shows a fit of the same data but with a tie.

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=Gaussian, PeakCentre=4, Height=1.3, Sigma=0.5
 Ties:            Sigma=Height/2
 Output:          res

[[Image:GaussianFit_Ties.jpg]]

----

This example shows a fit of two overlapping Gaussians on a linear background. Here we create a composite function with a LinearBackground and two Gaussians:

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=LinearBackground,A0=1;
                  name=Gaussian,PeakCentre=4,Height=1.5, Sigma=0.5;
                  name=Gaussian,PeakCentre=6,Height=4, Sigma=0.5 
 Output:          res

[[Image:Gaussian2Fit.jpg]]

----

This example repeats the previous one but with the Sigmas of the two Gaussians tied:

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=LinearBackground,A0=1;
                  name=Gaussian,PeakCentre=4,Height=1.5, Sigma=0.5;
                  name=Gaussian,PeakCentre=6,Height=4, Sigma=0.5 
 Ties:            f2.Sigma = f1.Sigma
 Output:          res

[[Image:Gaussian2Fit_Ties.jpg]]


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/CostFuncIgnorePosPeaks.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Abstract base class for fitting functions.

    Properties common for all fitting functions:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>

    <LI> SpectrumNumber - The spectrum to fit, using the workspace numbering of the spectra (default 0)</LI>
    <LI> StartX - Lowest value of x data array </LI>
    <LI> EndX - Highest value of x data array </LI>

    <LI> Function - 

    <LI> MaxIterations - The spectrum to fit (default 500)</LI>
    <LI> OutputStatus - whether the fit was successful. Direction::Output</LI>
    <LI> OutputChi2overDoF - returns how good the fit was (default 0.0). Direction::Output</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 15/5/2009

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
    class DLLExport Fit : public API::Algorithm
    {
    public:
      /// Default constructor
      Fit() : API::Algorithm() {};
      /// Destructor
      virtual ~Fit();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Fit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

    protected:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();

      // Process input parameters and create the fitting function.
      void processParameters();

      /// Function initialization string
      std::string m_function_input;

      friend struct FitData1;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FIT_H_*/
