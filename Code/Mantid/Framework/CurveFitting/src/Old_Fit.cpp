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
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/GenericFit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"

#include <sstream>
#include <numeric>
#include <cmath>
#include <iomanip>

namespace Mantid
{
namespace CurveFitting
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(Fit)
  
  /// Sets documentation strings for this algorithm
  void Fit::initDocs()
  {
    this->setWikiSummary("Fits a function to a spectrum in a Workspace2D ");
    this->setOptionalMessage("Fits a function to a spectrum in a Workspace2D");
  }
  

  using namespace Kernel;
  using API::WorkspaceProperty;
  using API::Axis;
  using API::MatrixWorkspace;
  using API::Algorithm;
  using API::Progress;
  using API::Jacobian;


  ///Destructor
  Fit::~Fit()
  {
  }

  /** Initialisation method
  */
  void Fit::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty(new PropertyWithValue<int>("WorkspaceIndex",0, mustBePositive),
                    "The Workspace Index to fit in the input workspace");
    declareProperty("StartX", EMPTY_DBL(),
      "A value of x in, or on the low x boundary of, the first bin to include in\n"
      "the fit (default lowest value of x)" );
    declareProperty("EndX", EMPTY_DBL(),
      "A value in, or on the high x boundary of, the last bin the fitting range\n"
      "(default the highest value of x)" );

    //declareProperty("Function","","Parameters defining the fitting function and its initial values",Direction::InOut );
    declareProperty(new API::FunctionProperty("Function"),"Parameters defining the fitting function and its initial values");
    declareProperty("Ties","","Math expressions that tie parameters to other parameters or to constants" );
    declareProperty("Constraints","","List of constraints" );

    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("OutputStatus","", Direction::Output);
    declareProperty("OutputChi2overDoF",0.0, Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    declareProperty("Output","","If not empty OutputParameters TableWorksace and OutputWorkspace will be created.");

    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Direction::InOut);

    std::vector<std::string> costFuncOptions = API::CostFunctionFactory::Instance().getKeys();
    declareProperty("CostFunction","Least squares",new ListValidator(costFuncOptions),
      "The cost function to be used for the fit, default is Least squares", Direction::InOut);
  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void Fit::exec()
  {
    API::MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
    std::string input = "WorkspaceIndex=" + getPropertyValue("WorkspaceIndex");
    double startX = getProperty("StartX");
    if (startX != EMPTY_DBL())
    {
      input += ",StartX=" + getPropertyValue("StartX");
    }
    double endX = getProperty("EndX");
    if (endX != EMPTY_DBL())
    {
      input += ",EndX=" + getPropertyValue("EndX");
    }

    // Process the Function property
    processParameters();

    boost::shared_ptr<GenericFit> fit = boost::dynamic_pointer_cast<GenericFit>(createSubAlgorithm("GenericFit"));
    fit->setChild(false);
    fit->setLogging(false); // No logging of time to run GenericFit
    fit->initialize();
    fit->setProperty("InputWorkspace",boost::dynamic_pointer_cast<API::Workspace>(ws));
    fit->setProperty("Input",input);
    fit->setProperty("Function",m_function);
    fit->setProperty("Output",getPropertyValue("Output"));
    fit->setPropertyValue("MaxIterations",getPropertyValue("MaxIterations"));
    fit->setPropertyValue("Minimizer",getPropertyValue("Minimizer"));
    fit->setPropertyValue("CostFunction",getPropertyValue("CostFunction"));
    fit->execute();

    m_function = fit->getProperty("Function");
    setProperty("Function",m_function);

    // also output summary to properties
    setProperty("OutputStatus", fit->getPropertyValue("OutputStatus"));
    double finalCostFuncVal = fit->getProperty("OutputChi2overDoF");
    setProperty("OutputChi2overDoF", finalCostFuncVal);
    setProperty("Minimizer", fit->getPropertyValue("Minimizer"));

    std::vector<double> errors;
    if (fit->existsProperty("Parameters"))
    {
      // Add Parameters, Errors and ParameterNames properties to output so they can be queried on the algorithm.
      declareProperty(new ArrayProperty<double> ("Parameters",new NullValidator<std::vector<double> >,Direction::Output));
      declareProperty(new ArrayProperty<double> ("Errors",new NullValidator<std::vector<double> >,Direction::Output));
      declareProperty(new ArrayProperty<std::string> ("ParameterNames",new NullValidator<std::vector<std::string> >,Direction::Output));
      std::vector<double> params = fit->getProperty("Parameters");
      errors = fit->getProperty("Errors");
      std::vector<std::string> parNames = fit->getProperty("ParameterNames");

      setProperty("Parameters",params);
      setProperty("Errors",errors);
      setProperty("ParameterNames",parNames);
    }
    
    std::string output = getProperty("Output");
    if (!output.empty())
    {
      // create output parameter table workspace to store final fit parameters 
      // including error estimates if derivative of fitting function defined

      boost::shared_ptr<API::IFunctionMW> funmw = boost::dynamic_pointer_cast<API::IFunctionMW>(fit->getFunction());
      if (funmw)
      {
        declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
          "Name of the output Workspace holding resulting simlated spectrum");

        setPropertyValue("OutputWorkspace",output+"_Workspace");

        // Save the fitted and simulated spectra in the output workspace
        int workspaceIndex  = getProperty("WorkspaceIndex");
        if (workspaceIndex < 0) throw std::invalid_argument("WorkspaceIndex must be >= 0");
        funmw->setWorkspace(ws,input,true);
        API::MatrixWorkspace_sptr outws = funmw->createCalculatedWorkspace(ws,workspaceIndex,errors);

        setProperty("OutputWorkspace",outws);
      }
    }

  }

  /**
   * Process input parameters and create the fitting function.
   */
  void Fit::processParameters()
  {

    // Parameters of different functions are separated by ';'. Parameters of the same function
    // are separated by ','. parameterName=value pairs are used to set a parameter value. For each function
    // "name" parameter must be set to a function name. E.g.
    // Function = "name=LinearBackground,A0=0,A1=1; name = Gaussian, PeakCentre=10.,Sigma=1"
    m_function = getProperty("Function");
    if (!m_function) return;

    std::string function_input;
    std::string inputConstraints = getProperty("Constraints");
    // if Constraints property isn't empty change the function by adding the constraints to it
    if (!inputConstraints.empty())
    {
      function_input = m_function->asString();
      std::string::size_type i = function_input.find_last_not_of(" \t\n\r");
      if (i == std::string::npos) return;
      if (function_input[i] == ';')
      {
        function_input.erase(i);
      }

      if (!inputConstraints.empty())
      {
        if (function_input.find(';') != std::string::npos)
        {
          function_input += ";";
        }
        else
        {
          function_input += ",";
        }
        std::string::size_type i = inputConstraints.find_last_not_of(" \t\n\r");
        if (inputConstraints[i] == ',')
        {
          inputConstraints.erase(i);
        }
        function_input += "constraints=("+inputConstraints+")";
      }
    }

    // Ties property is a comma separated list of formulas of the form:
    // tiedParamName = MathExpression, parameter names defined in the fitted function can be used
    // as variables in MathExpression. If the fitted function is a CompositeFunction parameter names
    // have form: f<index>.<name>, i.e. start with symbol 'f' (for function) followed by function's index 
    // in the CompositeFunction and a period '.' which is followed by the parameter name. e.g.
    // "f2.A = 2*f1.B + f5.C, f1.A=10"
    std::string inputTies = getProperty("Ties");
    if (!inputTies.empty())
    {
      if (function_input.empty())
      {
        function_input = m_function->asString();
      }
      if (function_input.find(';') != std::string::npos)
      {
        function_input += ";";
      }
      else
      {
        function_input += ",";
      }
      std::string::size_type i = inputTies.find_last_not_of(" \t\n\r");
      if (inputTies[i] == ',')
      {
        inputTies.erase(i);
      }
      function_input += "ties=("+inputTies+")";
    }
    
    // if function_input isn't empty then new function must be created
    if (!function_input.empty())
    {
      // this creates a new function from definition in function_input
      setPropertyValue("Function",function_input);
      m_function = getProperty("Function");
    }

  }

} // namespace Algorithm
} // namespace Mantid
