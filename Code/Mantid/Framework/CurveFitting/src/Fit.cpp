//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/GenericFit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/SimplexMinimizer.h"
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
    declareProperty("WorkspaceIndex",0, mustBePositive,
      "The Workspace to fit, uses the workspace numbering of the spectra (default 0)");
    declareProperty("StartX", EMPTY_DBL(),
      "A value of x in, or on the low x boundary of, the first bin to include in\n"
      "the fit (default lowest value of x)" );
    declareProperty("EndX", EMPTY_DBL(),
      "A value in, or on the high x boundary of, the last bin the fitting range\n"
      "(default the highest value of x)" );

    //declareProperty("Function","","Parameters defining the fitting function and its initial values",Direction::InOut );
    declareProperty("Function","",Direction::InOut );
    declareProperty("Ties","","Math expressions that tie parameters to other parameters or to constants" );
    declareProperty("Constraints","","List of constraints" );

    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("Output Status","", Direction::Output);
    declareProperty("Output Chi^2/DoF",0.0, Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    declareProperty("Output","","If not empty OutputParameters TableWorksace and OutputWorkspace will be created.");

    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Direction::InOut);

    std::vector<std::string> costFuncOptions = CostFunctionFactory::Instance().getKeys();
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

    // Process the Function property and create the function using FunctionFactory
    // fills in m_function_input
    processParameters();

    boost::shared_ptr<GenericFit> fit = boost::dynamic_pointer_cast<GenericFit>(createSubAlgorithm("GenericFit"));
    fit->setChild(false);
    fit->initialize();
    fit->setProperty("InputWorkspace",boost::dynamic_pointer_cast<API::Workspace>(ws));
    fit->setProperty("Input",input);
    fit->setProperty("Function",m_function_input);
    fit->setProperty("Output",getPropertyValue("Output"));
    fit->setPropertyValue("MaxIterations",getPropertyValue("MaxIterations"));
    fit->setPropertyValue("Minimizer",getPropertyValue("Minimizer"));
    fit->setPropertyValue("CostFunction",getPropertyValue("CostFunction"));
    fit->execute();

    m_function_input = fit->getPropertyValue("Function");
    setProperty("Function",m_function_input);

    // also output summary to properties
    setProperty("Output Status", fit->getPropertyValue("Output Status"));
    double finalCostFuncVal = fit->getProperty("Output Chi^2/DoF");
    setProperty("Output Chi^2/DoF", finalCostFuncVal);
    setProperty("Minimizer", fit->getPropertyValue("Minimizer"));

    std::string output = getProperty("Output");

    if (!output.empty())
    {
      // create output parameter table workspace to store final fit parameters 
      // including error estimates if derivative of fitting function defined

      API::IFunctionMW* funmw = dynamic_cast<API::IFunctionMW*>(fit->getFunction());
      if (funmw)
      {
        declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
          "Name of the output Workspace holding resulting simlated spectrum");

        setPropertyValue("OutputWorkspace",output+"_Workspace");

        // Save the fitted and simulated spectra in the output workspace
        int iSpec = getProperty("WorkspaceIndex");
        funmw->setWorkspace(ws,input);
        API::MatrixWorkspace_sptr outws = funmw->createCalculatedWorkspace(ws,iSpec);

        setProperty("OutputWorkspace",outws);
      }
    }

    if (fit->existsProperty("Parameters"))
    {
      // Add Parameters, Errors and ParameterNames properties to output so they can be queried on the algorithm.
      declareProperty(new ArrayProperty<double> ("Parameters",new NullValidator<std::vector<double> >,Direction::Output));
      declareProperty(new ArrayProperty<double> ("Errors",new NullValidator<std::vector<double> >,Direction::Output));
      declareProperty(new ArrayProperty<std::string> ("ParameterNames",new NullValidator<std::vector<std::string> >,Direction::Output));
      std::vector<double> params = fit->getProperty("Parameters");
      std::vector<double> errors = fit->getProperty("Errors");
      std::vector<std::string> parNames = fit->getProperty("ParameterNames");

      setProperty("Parameters",params);
      setProperty("Errors",errors);
      setProperty("ParameterNames",parNames);
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
    m_function_input = getPropertyValue("Function");
    if (m_function_input.empty()) return;

    std::string::size_type i = m_function_input.find_last_not_of(" \t\n\r");
    if (i == std::string::npos) return;
    if (m_function_input[i] == ';')
    {
      m_function_input.erase(i);
    }

    std::string inputConstraints = getProperty("Constraints");
    if (!inputConstraints.empty())
    {
      if (m_function_input.find(';') != std::string::npos)
      {
        m_function_input += ";";
      }
      else
      {
        m_function_input += ",";
      }
      std::string::size_type i = inputConstraints.find_last_not_of(" \t\n\r");
      if (inputConstraints[i] == ',')
      {
        inputConstraints.erase(i);
      }
      m_function_input += "constraints=("+inputConstraints+")";
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
      if (m_function_input.find(';') != std::string::npos)
      {
        m_function_input += ";";
      }
      else
      {
        m_function_input += ",";
      }
      std::string::size_type i = inputTies.find_last_not_of(" \t\n\r");
      if (inputTies[i] == ',')
      {
        inputTies.erase(i);
      }
      m_function_input += "ties=("+inputTies+")";
    }

  }

} // namespace Algorithm
} // namespace Mantid
