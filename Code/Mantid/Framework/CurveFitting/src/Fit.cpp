/*WIKI* 


This algorithm fits data in a [[Workspace]] with a function. The function and the initial values for its parameters are set with the Function property. The function must be compatible with the workspace.

Using the Minimizer property, Fit can be set to use different algorithms to perform the minimization. By default if the function's derivatives can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer. If the function's derivatives cannot be evaluated the GSL Simplex minimizer is used. Also, if one minimizer fails, for example the Levenberg-Marquardt minimizer, Fit may try its luck with a different minimizer. If this happens the user is notified about this and the Minimizer property is updated accordingly.

===Output===

Setting the Output property defines the names of the output workspaces. One of them is a [[TableWorkspace]] with the fitted parameter values.  If the function's derivatives can be evaluated an additional TableWorkspace is returned containing correlation coefficients in %.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/FitMD.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunctionMD.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Matrix.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_errno.h>
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(Fit)
  
  /// Sets documentation strings for this algorithm
  void Fit::initDocs()
  {
    this->setWikiSummary("Fits a function to data in a Workspace ");
    this->setOptionalMessage("Fits a function to data in a Workspace");
  }
  
  /**
   * Examine "Function" and "InputWorkspace" properties to decide which domain creator to use.
   * @param propName :: A property name.
   */
  void Fit::afterPropertySet(const std::string& propName)
  {
    if (propName == "Function")
    {
      m_isFunctionSet = true;
    }
    if (propName == "InputWorkspace")
    {
      m_isWorkspaceSet = true;
    }

    // Create the creator
    if (!m_domainCreator && m_isFunctionSet && m_isWorkspaceSet)
    {
      // get the function
      m_function = getProperty("Function");

      // get the workspace 
      API::Workspace_const_sptr ws = getProperty("InputWorkspace");
      //m_function->setWorkspace(ws);

      if ( boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws) &&
          !boost::dynamic_pointer_cast<API::IFunctionMD>(m_function) )
      {
        m_domainCreator.reset(new FitMW(this));
      }
      else if (boost::dynamic_pointer_cast<const API::IMDWorkspace>(ws))
      {
        m_domainCreator.reset(new FitMD(this));
      }
      else
      {// don't know what to do with this workspace
        throw std::invalid_argument("Unsupported workspace type" + ws->id());
      }
      m_domainCreator->declareDatasetProperties();
    }

  }

  /** Initialisation method
  */
  void Fit::init()
  {
    declareProperty(new API::WorkspaceProperty<API::Workspace>("InputWorkspace","",Kernel::Direction::Input), "Name of the input Workspace");

    //declareDatasetProperties();

    declareProperty(new API::FunctionProperty("Function"),"Parameters defining the fitting function and its initial values");

    auto mustBePositive = new Kernel::BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("OutputStatus","", Kernel::Direction::Output);
    declareProperty("OutputChi2overDoF",0.0, Kernel::Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",new Kernel::ListValidator(minimizerOptions),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Kernel::Direction::InOut);

    std::vector<std::string> costFuncOptions = API::CostFunctionFactory::Instance().getKeys();
    // select only CostFuncFitting variety
    for(auto it = costFuncOptions.begin(); it != costFuncOptions.end(); ++it)
    {
      auto costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(*it)
        );
      if (!costFunc)
      {
        *it = "";
      }
    }
    std::remove_if(costFuncOptions.begin(),costFuncOptions.end(),[](std::string& str)->bool{return str.empty();});
    declareProperty("CostFunction","Least squares",new Kernel::ListValidator(costFuncOptions),
      "The cost function to be used for the fit, default is Least squares", Kernel::Direction::InOut);
    declareProperty("CreateOutput", false,
      "Set to true to create output workspaces with the results of the fit"
      "(default is false)." );
    declareProperty("Output", "",
      "A base name for the output workspaces (if not given default names will be created)." );
  }

  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void Fit::exec()
  {

    // Try to retrieve optional properties
    const int maxIterations = getProperty("MaxIterations");

    // do something with the function which may depend on workspace
    m_domainCreator->initFunction();

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;
    m_domainCreator->createDomain(domain,values);

    // get the minimizer
    std::string minimizerName = getPropertyValue("Minimizer");
    IFuncMinimizer_sptr minimizer = FuncMinimizerFactory::Instance().create(minimizerName);

    // get the cost function which must be a CostFuncFitting
    boost::shared_ptr<CostFuncFitting> costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
      API::CostFunctionFactory::Instance().create(getPropertyValue("CostFunction"))
      );

    costFunc->setFittingFunction(m_function,domain,values);
    minimizer->initialize(costFunc);

    API::Progress prog(this,0.0,1.0,maxIterations?maxIterations:1);

    // do the fitting until success or iteration limit is reached
    size_t iter = 0;
    bool success = false;
    std::string errorString;
    double costFuncVal = 0;
    do
    {
      iter++;
      if ( !minimizer->iterate() )
      {
        errorString = minimizer->getError();
        success = errorString.empty() || errorString == "success";
        if (success)
        {
          errorString = "success";
        }
        break;
      }
      prog.report();
    }
    while (iter < maxIterations);

    if (iter >= maxIterations)
    {
      if ( !errorString.empty() )
      {
        errorString += '\n';
      }
      errorString += "Failed to converge after " + boost::lexical_cast<std::string>(maxIterations) + " iterations.";
    }

    // return the status flag
    setPropertyValue("OutputStatus",errorString);

    // degrees of freedom
    size_t dof = values->size() - costFunc->nParams();
    if (dof == 0) dof = 1;
    double finalCostFuncVal = minimizer->costFunctionVal() / dof;

    setProperty("OutputChi2overDoF",finalCostFuncVal);

    // fit ended, creating output

    // get the workspace 
    API::Workspace_const_sptr ws = getProperty("InputWorkspace");

    bool doCreateOutput = getProperty("CreateOutput");
    std::string baseName = getPropertyValue("Output");
    if ( !baseName.empty() )
    {
      doCreateOutput = true;
    }

    if (doCreateOutput)
    {
      if (baseName.empty())
      {
        baseName = ws->name();
        if (baseName.empty())
        {
          baseName = "Output";
        }
      }
      baseName += "_";

      // Calculate the covariance matrix and the errors.
      GSLMatrix covar;
      costFunc->calCovarianceMatrix(covar);
      costFunc->calFittingErrors(covar);

        declareProperty(
          new API::WorkspaceProperty<API::ITableWorkspace>("OutputNormalisedCovarianceMatrix","",Kernel::Direction::Output),
          "The name of the TableWorkspace in which to store the final covariance matrix" );
        setPropertyValue("OutputNormalisedCovarianceMatrix",baseName+"NormalisedCovarianceMatrix");

        Mantid::API::ITableWorkspace_sptr covariance = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        covariance->addColumn("str","Name");
        // set plot type to Label = 6
        covariance->getColumn(covariance->columnCount()-1)->setPlotType(6);
        //std::vector<std::string> paramThatAreFitted; // used for populating 1st "name" column
        for(size_t i=0; i < m_function->nParams(); i++)
        {
          if (m_function->isActive(i)) 
          {
            covariance->addColumn("double",m_function->parameterName(i));
            //paramThatAreFitted.push_back(m_function->parameterName(i));
          }
        }

        size_t np = m_function->nParams();
        size_t ia = 0;
        for(size_t i = 0; i < np; i++)
        {
          if (m_function->isFixed(i)) continue;
          Mantid::API::TableRow row = covariance->appendRow();
          row << m_function->parameterName(i);
          size_t ja = 0;
          for(size_t j = 0; j < np; j++)
          {
            if (m_function->isFixed(j)) continue;
            if (j == i)
              row << 100.0;
            else
            {
              row << 100.0*covar.get(ia,ja)/sqrt(covar.get(ia,ia)*covar.get(ja,ja));
            }
            ++ja;
          }
          ++ia;
        }

        setProperty("OutputNormalisedCovarianceMatrix",covariance);

      // create output parameter table workspace to store final fit parameters 
      // including error estimates if derivative of fitting function defined

      declareProperty(
        new API::WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Kernel::Direction::Output),
        "The name of the TableWorkspace in which to store the final fit parameters" );

      setPropertyValue("OutputParameters",baseName+"Parameters");

      Mantid::API::ITableWorkspace_sptr result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      result->addColumn("str","Name");
      // set plot type to Label = 6
      result->getColumn(result->columnCount()-1)->setPlotType(6);
      result->addColumn("double","Value");
      result->addColumn("double","Error");
      // yErr = 5
      result->getColumn(result->columnCount()-1)->setPlotType(5);

      for(size_t i=0;i<m_function->nParams();i++)
      {
        Mantid::API::TableRow row = result->appendRow();
        row << m_function->parameterName(i) 
            << m_function->getParameter(i)
            << m_function->getError(i);
      }
      // Add chi-squared value at the end of parameter table
      Mantid::API::TableRow row = result->appendRow();
      row << "Cost function value" << finalCostFuncVal;      
      setProperty("OutputParameters",result);

      m_domainCreator->createOutputWorkspace(baseName,domain,values);

    }

  }


} // namespace Algorithm
} // namespace Mantid
