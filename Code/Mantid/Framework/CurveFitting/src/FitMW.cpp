/*WIKI* 


This algorithm fits data in a [[Workspace]] with a function. The function and the initial values for its parameters are set with the Function property. The function must be compatible with the workspace.

Using the Minimizer property, Fit can be set to use different algorithms to perform the minimization. By default if the function's derivatives can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer. If the function's derivatives cannot be evaluated the GSL Simplex minimizer is used. Also, if one minimizer fails, for example the Levenberg-Marquardt minimizer, Fit may try its luck with a different minimizer. If this happens the user is notified about this and the Minimizer property is updated accordingly.

===Output===

Setting the Output property defines the names of the output workspaces. One of them is a [[TableWorkspace]] with the fitted parameter values.  If the function's derivatives can be evaluated an additional TableWorkspace is returned containing correlation coefficients in %.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionMW.h"

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
  DECLARE_ALGORITHM(FitMW)
  
  /// Sets documentation strings for this algorithm
  void FitMW::initDocs()
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

  /// for use in an std algorithm
  bool isEmptyString(const std::string& str){return str.empty();}


  /** Initialisation method
  */
  void FitMW::init()
  {
    declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

    declareProperty(new API::FunctionProperty("Function"),"Parameters defining the fitting function and its initial values");

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

    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("OutputStatus","", Direction::Output);
    declareProperty("OutputChi2overDoF",0.0, Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    //declareProperty("Output","","If not empty OutputParameters TableWorksace and OutputWorkspace will be created.");

    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Direction::InOut);

    std::vector<std::string> costFuncOptions = API::CostFunctionFactory::Instance().getKeys();
    // select only CostFuncFitting variety
    for(std::vector<std::string>::iterator it = costFuncOptions.begin(); it != costFuncOptions.end(); ++it)
    {
      boost::shared_ptr<CostFuncFitting> costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(*it)
        );
      if (!costFunc)
      {
        *it = "";
      }
    }
    std::remove_if(costFuncOptions.begin(),costFuncOptions.end(),isEmptyString);
    declareProperty("CostFunction","Least squares",new ListValidator(costFuncOptions),
      "The cost function to be used for the fit, default is Least squares", Direction::InOut);
    declareProperty("CreateOutput", false,
      "Set to true to create output workspaces with the results if the fit"
      "(default is false)." );
  }

  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void FitMW::exec()
  {

    // Try to retrieve optional properties
    const int maxIterations = getProperty("MaxIterations");

    // get the function
    m_function = getProperty("Function");

    // get the workspace 
    API::MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
    //m_function->setWorkspace(ws);
    int index = getProperty("WorkspaceIndex");
    size_t wsIndex = static_cast<size_t>(index);
    const Mantid::MantidVec& X = ws->readX(wsIndex);
    double startX = getProperty("StartX");
    if (startX == EMPTY_DBL())
    {
      startX = X.front();
    }
    double endX = getProperty("EndX");
    if (endX == EMPTY_DBL())
    {
      endX = X.back();
    }
    Mantid::MantidVec::const_iterator from = std::lower_bound(X.begin(),X.end(),startX);
    Mantid::MantidVec::const_iterator to = std::upper_bound(from,X.end(),endX);

    API::IFunctionMW* funMW = dynamic_cast<API::IFunctionMW*>(m_function.get());
    if (funMW)
    {
      funMW->setMatrixWorkspace(ws,wsIndex,startX,endX);
    }

    API::FunctionDomain1D_sptr domain;
    if (ws->isHistogramData())
    {
      if ( X.end() == to ) to = X.end() - 1;
      std::vector<double> x( static_cast<size_t>(to - from) );
      Mantid::MantidVec::const_iterator it = from;
      for(size_t i = 0; it != to; ++it,++i)
      {
        x[i] = (*it + *(it+1)) / 2;
      }
      domain.reset(new API::FunctionDomain1D(x));
      x.clear();
    }
    else
    {
      domain.reset(new API::FunctionDomain1D(from,to));
    }

    // set the data to fit to
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    size_t ifrom = static_cast<size_t>( from - X.begin() );
    size_t n = values->size();
    const Mantid::MantidVec& Y = ws->readY( wsIndex );
    const Mantid::MantidVec& E = ws->readE( wsIndex );
    bool foundZeroOrNegativeError = false;
    for(size_t i = ifrom; i < n; ++i)
    {
      size_t j = i - ifrom;
      values->setFitData( j, Y[i] );
      double error = E[i];
      if (error <= 0)
      {
        error = 1.0;
        foundZeroOrNegativeError = true;
      }
      values->setFitWeight( j, 1.0 / error );
    }

    if (foundZeroOrNegativeError)
    {
      g_log.warning() << "Zero or negative errors are replaced with 1.0\n";
    }

    // get the minimizer
    std::string minimizerName = getPropertyValue("Minimizer");
    IFuncMinimizer_sptr minimizer = FuncMinimizerFactory::Instance().create(minimizerName);

    // get the cost function which must be a CostFuncFitting
    boost::shared_ptr<CostFuncFitting> costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
      API::CostFunctionFactory::Instance().create(getPropertyValue("CostFunction"))
      );

    costFunc->setFittingFunction(m_function,domain,values);
    minimizer->initialize(costFunc);

    Progress prog(this,0.0,1.0,maxIterations?maxIterations:1);

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
        errorString = "success";
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

    bool doCreateOutput = getProperty("CreateOutput");
    if (doCreateOutput)
    {
      std::string baseName = ws->name();
      if (baseName.empty())
      {
        baseName = "Output";
      }
      baseName += "_" + boost::lexical_cast<std::string>(wsIndex) + "_";

      // Create a workspace with calculated values
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
      setPropertyValue("OutputWorkspace",baseName+"Workspace");
      
      API::MatrixWorkspace_sptr outWS = createOutputWorkspace(ws,wsIndex,ifrom,domain,values);
      setProperty("OutputWorkspace",outWS);

      // Calculate the covariance matrix and the errors.
      Kernel::Matrix<double> covar;
      costFunc->calCovarianceMatrix(covar);
      costFunc->calFittingErrors(covar);

        declareProperty(
          new WorkspaceProperty<API::ITableWorkspace>("OutputNormalisedCovarianceMatrix","",Direction::Output),
          "The name of the TableWorkspace in which to store the final covariance matrix" );
        setPropertyValue("OutputNormalisedCovarianceMatrix",baseName+"NormalisedCovarianceMatrix");

        Mantid::API::ITableWorkspace_sptr covariance = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        covariance->addColumn("str","Name");
        // set plot type to Label = 6
        covariance->getColumn(covariance->columnCount()-1)->setPlotType(6);
        std::vector<std::string> paramThatAreFitted; // used for populating 1st "name" column
        for(size_t i=0; i < m_function->nParams(); i++)
        {
          if (m_function->isActive(i)) 
          {
            covariance->addColumn("double",m_function->parameterName(i));
            paramThatAreFitted.push_back(m_function->parameterName(i));
          }
        }

        size_t np = m_function->nParams();
        for(size_t i = 0; i < np; i++)
        {
          Mantid::API::TableRow row = covariance->appendRow();
          row << paramThatAreFitted[i];
          for(size_t j = 0; j < np; j++)
          {
            if (j == i)
              row << 100.0;
            else
            {
              row << 100.0*covar[i][j]/sqrt(covar[i][i]*covar[j][j]);
            }
          }
        }

        setProperty("OutputNormalisedCovarianceMatrix",covariance);

      // create output parameter table workspace to store final fit parameters 
      // including error estimates if derivative of fitting function defined

      declareProperty(
        new WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Direction::Output),
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

    }

  }

  /**
   * Create an output workspace with the calculated values.
   * @param inWS :: The input workspace to the algorithm
   * @param wi :: The input workspace index.
   * @param startIndex :: The starting index in the X array where the fit data start
   * @param domain :: The domain
   * @param values :: The values
   */
  API::MatrixWorkspace_sptr FitMW::createOutputWorkspace(
        boost::shared_ptr<const API::MatrixWorkspace> inWS,
        size_t wi,
        size_t startIndex,
        boost::shared_ptr<API::FunctionDomain1D> domain,
        boost::shared_ptr<API::FunctionValues> values
    )
  {
    // calculate the values
    m_function->function(*domain,*values);
    const MantidVec& inputX = inWS->readX(wi);
    const MantidVec& inputY = inWS->readY(wi);
    const MantidVec& inputE = inWS->readE(wi);
    size_t nData = values->size();

      size_t histN = inWS->isHistogramData() ? 1 : 0;
      API::MatrixWorkspace_sptr ws =
        Mantid::API::WorkspaceFactory::Instance().create(
            "Workspace2D",
            3,
            nData + histN,
            nData);
      ws->setTitle("");
      ws->setYUnitLabel(inWS->YUnitLabel());
      ws->setYUnit(inWS->YUnit());
      ws->getAxis(0)->unit() = inWS->getAxis(0)->unit();
      API::TextAxis* tAxis = new API::TextAxis(3);
      tAxis->setLabel(0,"Data");
      tAxis->setLabel(1,"Calc");
      tAxis->setLabel(2,"Diff");
      ws->replaceAxis(1,tAxis);

      for(size_t i=0;i<3;i++)
      {
        ws->dataX(i).assign( inputX.begin() + startIndex, inputX.begin() + nData + histN );
      }

      ws->dataY(0).assign( inputY.begin() + startIndex, inputY.begin() + nData);
      ws->dataE(0).assign( inputE.begin() + startIndex, inputE.begin() + nData );

      MantidVec& Ycal = ws->dataY(1);
      MantidVec& Ecal = ws->dataE(1);
      MantidVec& Diff = ws->dataY(2);

      for(size_t i = 0; i < nData; ++i)
      {
        Ycal[i] = values->getCalculated(i);
        Diff[i] = values->getFitData(i) - Ycal[i];
      }

      //if (sd.size() == static_cast<size_t>(this->nParams()))
      //{
      //  SimpleJacobian J(nData,this->nParams());
      //  try
      //  {
      //    this->functionDeriv(&J);
      //  }
      //  catch(...)
      //  {
      //    this->calNumericalDeriv(&J,&m_xValues[0],nData);
      //  }
      //  for(size_t i=0; i<nData; i++)
      //  {
      //    double err = 0.0;
      //    for(size_t j=0;j< static_cast<size_t>(nParams());++j)
      //    {
      //      double d = J.get(i,j) * sd[j];
      //      err += d*d;
      //    }
      //    Ecal[i] = sqrt(err);
      //  }
      //}
      return ws;
  }

  /**
   * If i-th parameter is transformed the derivative will be != 1.0.
   * The derivative is calculated numerically.
   * @param i :: The index of an active parameter
   * @return The transformation derivative
   */
  double FitMW::transformationDerivative(int i)
  {
    //size_t j = m_function->indexOfActive(i);
    //double p0 = m_function->getParameter(j);
    //double ap0 = m_function->activeParameter(i);
    //double dap = ap0 != 0.0? ap0 * 0.001 : 0.001;
    //m_function->setActiveParameter(i,ap0 + dap);
    //double deriv = ( m_function->getParameter(j) - p0 ) / dap;
    //m_function->setParameter(j,p0,false);
    //return deriv;
    return 0.0;
  }

} // namespace Algorithm
} // namespace Mantid
