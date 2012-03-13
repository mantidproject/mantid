/*WIKI* 


This algorithm fits data in a [[Workspace]] with a function. The function and the initial values for its parameters are set with the Function property. The function must be compatible with the workspace.

Using the Minimizer property, Fit can be set to use different algorithms to perform the minimization. By default if the function's derivatives can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer. If the function's derivatives cannot be evaluated the GSL Simplex minimizer is used. Also, if one minimizer fails, for example the Levenberg-Marquardt minimizer, Fit may try its luck with a different minimizer. If this happens the user is notified about this and the Minimizer property is updated accordingly.

===Output===

Setting the Output property defines the names of the output workspaces. One of them is a [[TableWorkspace]] with the fitted parameter values.  If the function's derivatives can be evaluated an additional TableWorkspace is returned containing correlation coefficients in %.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/NewFit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/TempFunction.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_errno.h>

#include <sstream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace CurveFitting
{

  // Register the class into the algorithm factory
  //DECLARE_ALGORITHM(NewFit)
  
  /// Sets documentation strings for this algorithm
  void NewFit::initDocs()
  {
    this->setWikiSummary("Fits a function to data in a Workspace ");
    this->setOptionalMessage("Fits a function to data in a Workspace");
  }
  

  using namespace Kernel;
  using API::WorkspaceProperty;
  using API::Workspace;
  using API::Axis;
  using API::MatrixWorkspace;
  using API::Algorithm;
  using API::Progress;
  using API::Jacobian;


  ///Destructor
  NewFit::~NewFit()
  {
  }

  /** Initialisation method
  */
  void NewFit::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

    declareProperty("Function","",Direction::InOut );

    auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
    mustBePositive->setLower(0);
    declareProperty("MaxIterations", 500, mustBePositive,
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("OutputStatus","", Direction::Output);
    declareProperty("OutputChi2overDoF",0.0, Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    declareProperty("Output","","If not empty OutputParameters TableWorksace and OutputWorkspace will be created.");

  }

  /**
   * Override to dynamically set new properties.
   */
  void NewFit::setPropertyValue(const std::string &name, const std::string &value)
  {
    std::string NAME(name);
    std::transform(name.begin(),name.end(),NAME.begin(),toupper);
    if( NAME == "INPUTWORKSPACE" )
    {
      if ( !value.empty() )
      {
        API::Algorithm::setPropertyValue(name, value);
        declareWorkspaceDomainProperties();
      }
      declareVectorDomainProperties();
    }
    else
    {
      API::Algorithm::setPropertyValue(name, value);
    }
  }

  /**
   * Declare new properties for setting fit data from a MatrixWorkspace.
   */
  void NewFit::declareWorkspaceDomainProperties()
  {
    m_domainType = MatrixWorkspace;
  }

  /**
   * Declare new properties for setting fit data from vectors.
   */
  void NewFit::declareVectorDomainProperties()
  {
    m_domainType = Vector;
    API::MatrixWorkspace_sptr dummy = API::WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    setProperty("InputWorkspace",dummy);
    declareProperty(new ArrayProperty<double> ("XValues"),"The X values.");
    declareProperty(new ArrayProperty<double> ("FitData"),"The data to fit to.");
    declareProperty(new ArrayProperty<double> ("Weights",std::vector<double>()),"The fitting data weights.");
  }

  /**
   * Create the function domain from the input workspace.
   * @return :: FunctionDomain with fit data and weights set.
   */
  API::FunctionDomain* NewFit::createDomain() const
  {
    if (m_domainType == Vector)
    {
      std::vector<double> xvalues = getProperty("XValues");
      std::vector<double> data = getProperty("FitData");
      std::vector<double> weights = getProperty("Weights");
      if (xvalues.size() != data.size())
      {
        throw std::invalid_argument("XValues and FitData have different sizes.");
      }
      if (weights.size() != data.size())
      {
        g_log.warning() << "All weights set to 1\n";
        weights.resize(data.size(),1.0);
      }
      API::FunctionDomain1D* domain = new API::FunctionDomain1D(xvalues);
      domain->setFitData(data);
      domain->setFitWeights(weights);
      return domain;
    }
    else if (m_domainType == MatrixWorkspace)
    {
    }
    throw std::runtime_error("Cannot create FunctionDomain.");
  }

  /**
   * Set the workspace to the function.
   */
  void NewFit::setWorkspace()
  {
  }

  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void NewFit::exec()
  {

    // Try to retrieve optional properties
    //const int maxInterations = getProperty("MaxIterations");

    //Progress prog(this,0.0,1.0,maxInterations?maxInterations:1);

    std::string funIni = getProperty("Function");
    API::IFunctionMW* tmp = dynamic_cast<API::IFunctionMW*>( API::FunctionFactory::Instance().createInitialized(funIni) );
    if ( !tmp )
    {
      throw std::invalid_argument("Only MW functions are currently supported.");
    }
    m_function.reset( new API::TempFunction(tmp) );

    return;

    // FIXME: The following code is never executed, so can it be removed?
    /*
    prog.report("Setting workspace");
    API::Workspace_sptr ws = getProperty("InputWorkspace");
    m_function->setWorkspace(ws);


    const size_t nParam = m_function->nActive();
    const size_t nData = 0;//m_function->dataSize();
    if (nParam == 0)
    {
      g_log.error("There are no active parameters.");
      //setProperty("OutputChi2overDoF", minimizer->costFunctionVal());
      throw std::runtime_error("There are no active parameters.");
    }
    if (nData == 0)
    {
      g_log.error("The data set is empty.");
      throw std::runtime_error("The data set is empty.");
    }
    if (nData < nParam)
    {
      g_log.error("Number of data points less than number of parameters to be fitted.");
      throw std::runtime_error("Number of data points less than number of parameters to be fitted.");
    }
    */
    // finally do the fitting

    // Comment out the following 4 lines as the variables are not used.
    //int iter = 0;
    //int status = 0;
    //double finalCostFuncVal = 0.0;
    //double dof = static_cast<double>(nData - nParam);  // dof stands for degrees of freedom

  }

  /**
   * If i-th parameter is transformed the derivative will be != 1.0.
   * The derivative is calculated numerically.
   * @param i :: The index of an active parameter
   * @return The transformation derivative
   */
  double NewFit::transformationDerivative(int i)
  {
    size_t j = m_function->indexOfActive(i);
    double p0 = m_function->getParameter(j);
    double ap0 = m_function->activeParameter(i);
    double dap = ap0 != 0.0? ap0 * 0.001 : 0.001;
    m_function->setActiveParameter(i,ap0 + dap);
    double deriv = ( m_function->getParameter(j) - p0 ) / dap;
    m_function->setParameter(j,p0,false);
    return deriv;
  }

} // namespace Algorithm
} // namespace Mantid
