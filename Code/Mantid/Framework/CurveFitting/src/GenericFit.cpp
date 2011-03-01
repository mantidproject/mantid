//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
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
  DECLARE_ALGORITHM(GenericFit)

  using namespace Kernel;
  using API::WorkspaceProperty;
  using API::Workspace;
  using API::Axis;
  using API::MatrixWorkspace;
  using API::Algorithm;
  using API::Progress;
  using API::Jacobian;


  ///Destructor
  GenericFit::~GenericFit()
  {
    if (m_function) delete m_function;
  }

  /** Initialisation method
  */
  void GenericFit::init()
  {
    //this->setWikiSummary("Fits a function to data in a Workspace");
    //this->setOptionalMessage("Fits a function to data in a Workspace");

    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input), "Name of the input Workspace");
    declareProperty("Input","","Workspace slicing parameters. Must be consistent with the Function type (see FitFunction::setWorkspace).");

    declareProperty("Function","",Direction::InOut );

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("MaxIterations", 500, mustBePositive,
      "Stop after this number of iterations if a good GenericFit is not found" );
    declareProperty("Output Status","", Direction::Output);
    declareProperty("Output Chi^2/DoF",0.0, Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    declareProperty("Output","","If not empty OutputParameters TableWorksace and OutputWorkspace will be created.");

    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
      "The minimizer method applied to do the GenericFit, default is Levenberg-Marquardt", Direction::InOut);

    std::vector<std::string> costFuncOptions = CostFunctionFactory::Instance().getKeys();
    declareProperty("CostFunction","Least squares",new ListValidator(costFuncOptions),
      "The cost function to be used for the GenericFit, default is Least squares", Direction::InOut);
  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void GenericFit::exec()
  {
    // Try to retrieve optional properties
    const int maxInterations = getProperty("MaxIterations");

    std::string funIni = getProperty("Function");
    m_function = API::FunctionFactory::Instance().createInitialized(funIni);

    if (m_function == NULL)
    {
      throw std::runtime_error("Function was not set.");
    }

    API::Workspace_sptr ws = getProperty("InputWorkspace");
    std::string input = getProperty("Input");
    m_function->setWorkspace(ws,input);

    // force initial parameters to satisfy constraints of function
    m_function->setParametersToSatisfyConstraints();

    // check if derivative defined in derived class
    bool isDerivDefined = true;
    try
    {
      m_function->functionDeriv(NULL);
    }
    catch (Exception::NotImplementedError&)
    {
      isDerivDefined = false;
    }

    // What minimizer to use
    std::string methodUsed = getProperty("Minimizer");
    if ( !isDerivDefined && methodUsed.compare("Simplex") != 0 )
    {
      methodUsed = "Simplex";
      g_log.information() << "No derivatives available for this GenericFitting function"
                          << " therefore Simplex method used for GenericFitting\n";
    }

    // create and populate data containers. Warn user if nData < nParam 
    // since as a rule of thumb this is required as a minimum to obtained 'accurate'
    // GenericFitting parameter values.

    const int nParam = m_function->nActive();
    const int nData = m_function->dataSize();
    if (nParam == 0)
    {
      g_log.error("There are no active parameters.");
      throw std::runtime_error("There are no active parameters.");
    }
    if (nData == 0)
    {
      g_log.error("The data set is empty.");
      throw std::runtime_error("The data set is empty.");
    }
    if (nData < nParam)
    {
      g_log.error("Number of data points less than number of parameters to be GenericFitted.");
      throw std::runtime_error("Number of data points less than number of parameters to be GenericFitted.");
    }

    // set-up minimizer

    std::string costFunction = getProperty("CostFunction");
    IFuncMinimizer* minimizer = FuncMinimizerFactory::Instance().createUnwrapped(methodUsed);
    minimizer->initialize(m_function, costFunction);
    

    // finally do the GenericFitting

    int iter = 0;
    int status = 0;
    double finalCostFuncVal = 0.0;
    double dof = nData - nParam;  // dof stands for degrees of freedom

    // Standard least-squares used if derivative function defined otherwise simplex
    Progress prog(this,0.0,1.0,maxInterations);
    if ( methodUsed.compare("Simplex") != 0 )
    {
      status = GSL_CONTINUE;
      while (status == GSL_CONTINUE && iter < maxInterations)
      {
        iter++;
        status = minimizer->iterate();
        if (status != GSL_SUCCESS)  
        { 
          // From experience it is found that gsl_multiGenericFit_fdfsolver_iterate occasionally get
          // stock - even after having achieved a sensible GenericFit. This seem in particular to be a
          // problem on Linux. For now only fall back to Simplex if iter = 1 or 2, i.e.   
          // gsl_multiGenericFit_fdfsolver_iterate has failed on the first or second hurdle
          if (iter < 3)
          {
            g_log.warning() << "GenericFit algorithm using " << methodUsed << " failed "
              << "reporting the following: " << gsl_strerror(status) << "\n"
              << "Try using Simplex method instead\n";
            methodUsed = "Simplex";
            delete minimizer;
            minimizer = FuncMinimizerFactory::Instance().createUnwrapped(methodUsed);
            minimizer->initialize(m_function, costFunction);
            iter = 0;
          }
          break;
        }

        status = minimizer->hasConverged();
        prog.report();
      }

      finalCostFuncVal = minimizer->costFunctionVal() / dof;
    }

    if ( methodUsed.compare("Simplex") == 0 )
    {
      status = GSL_CONTINUE;
      while (status == GSL_CONTINUE && iter < maxInterations)
      {
        iter++;
        status = minimizer->iterate();

        if (status)  // break if error
        {
          // if failed at first iteration try reducing the initial step size
          if (iter == 1)
          { 
            g_log.information() << "Simplex step size reduced to 0.1\n";
            delete minimizer;
            SimplexMinimizer* sm = new SimplexMinimizer;
            sm->initialize(m_function, costFunction);
            sm->resetSize(0.1, m_function, costFunction);
            minimizer = sm;
            status = GSL_CONTINUE;
            continue;
          }
          break;
        }

        status = minimizer->hasConverged();
        prog.report();
      }

      finalCostFuncVal = minimizer->costFunctionVal() / dof;
    }

    // Output summary to log file

    std::string reportOfGenericFit = gsl_strerror(status);

    g_log.information() << "Method used = " << methodUsed << "\n" <<
      "Iteration = " << iter << "\n";
    if ( reportOfGenericFit == "success" )
      g_log.information() << reportOfGenericFit << "  Chi^2/DoF = " << finalCostFuncVal << "\n";
    else
      g_log.warning() << reportOfGenericFit << "  Chi^2/DoF = " << finalCostFuncVal << "\n";
    for (int i = 0; i < m_function->nParams(); i++)
    {
      g_log.debug() << m_function->parameterName(i) << " = " << m_function->getParameter(i) << "  \n";
    }


    // also output summary to properties

    setProperty("Output Status", reportOfGenericFit);
    setProperty("Output Chi^2/DoF", finalCostFuncVal);
    setProperty("Minimizer", methodUsed);
    setPropertyValue("Function",*m_function);
    

    // if Output property is specified output additional workspaces

    std::vector<double> standardDeviations;
    std::string output = getProperty("Output");
    gsl_matrix *covar(NULL);

    // only if derivative is defined for GenericFitting function create covariance matrix output workspace
    if ( methodUsed.compare("Simplex") != 0 )    
    {
      // calculate covariance matrix
      covar = gsl_matrix_alloc (nParam, nParam);
      minimizer->calCovarianceMatrix( 0.0, covar);

      // take standard deviations to be the square root of the diagonal elements of
      // the covariance matrix
      int iPNotFixed = 0;
      for(int i=0; i < m_function->nParams(); i++)
      {
        standardDeviations.push_back(1.0);
        if (m_function->isActive(i))
        {
          standardDeviations[i] = sqrt(gsl_matrix_get(covar,iPNotFixed,iPNotFixed));
          if (m_function->activeParameter(iPNotFixed) != m_function->getParameter(m_function->indexOfActive(iPNotFixed)))
          {// it means the active param is not the same as declared but transformed
            standardDeviations[i] *= fabs(transformationDerivative(iPNotFixed));
          }
          iPNotFixed++;
        }
      }
    }

    if (!output.empty())
    {
      // only if derivative is defined for GenericFitting function create covariance matrix output workspace
      if ( methodUsed.compare("Simplex") != 0 )    
      {
        // Create covariance matrix output workspace
        declareProperty(
          new WorkspaceProperty<API::ITableWorkspace>("OutputNormalisedCovarianceMatrix","",Direction::Output),
          "The name of the TableWorkspace in which to store the final covariance matrix" );
        setPropertyValue("OutputNormalisedCovarianceMatrix",output+"_NormalisedCovarianceMatrix");

        Mantid::API::ITableWorkspace_sptr m_covariance = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        m_covariance->addColumn("str","Name");
        std::vector<std::string> paramThatAreGenericFitted; // used for populating 1st "name" column
        for(int i=0; i < m_function->nParams(); i++) 
        {
          if (m_function->isActive(i)) 
          {
            m_covariance->addColumn("double",m_function->parameterName(i));
            paramThatAreGenericFitted.push_back(m_function->parameterName(i));
          }
        }

        for(int i=0; i<nParam; i++)
        {
          Mantid::API::TableRow row = m_covariance->appendRow();
          row << paramThatAreGenericFitted[i];
          for(int j=0; j<nParam; j++)
          {
            if (j == i)
              row << 100.0;
            else
            {
              row << 100.0*gsl_matrix_get(covar,i,j)/sqrt(gsl_matrix_get(covar,i,i)*gsl_matrix_get(covar,j,j));
            }
          }
        }

        setProperty("OutputNormalisedCovarianceMatrix",m_covariance);
      }

      // create output parameter table workspace to store final GenericFit parameters 
      // including error estimates if derivative of GenericFitting function defined

      declareProperty(
        new WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Direction::Output),
        "The name of the TableWorkspace in which to store the final GenericFit parameters" );

      setPropertyValue("OutputParameters",output+"_Parameters");

      Mantid::API::ITableWorkspace_sptr m_result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      m_result->addColumn("str","Name");
      m_result->addColumn("double","Value");
      if ( methodUsed.compare("Simplex") != 0 ) 
        m_result->addColumn("double","Error");

      for(int i=0;i<m_function->nParams();i++)
      {
        Mantid::API::TableRow row = m_result->appendRow();
        row << m_function->parameterName(i) << m_function->getParameter(i);
        if ( methodUsed.compare("Simplex") != 0 && m_function->isActive(i)) 
        {
          row << standardDeviations[i];
        }
      }
      // Add chi-squared value at the end of parameter table
      Mantid::API::TableRow row = m_result->appendRow();
      row << "Cost function value" << finalCostFuncVal;      
      setProperty("OutputParameters",m_result);


      if ( methodUsed.compare("Simplex") != 0 ) 
        gsl_matrix_free(covar);
    }

    // Add Parameters, Errors and ParameterNames properties to output so they can be queried on the algorithm.
    declareProperty(new ArrayProperty<double> ("Parameters",new NullValidator<std::vector<double> >,Direction::Output));
    declareProperty(new ArrayProperty<double> ("Errors",new NullValidator<std::vector<double> >,Direction::Output));
    declareProperty(new ArrayProperty<std::string> ("ParameterNames",new NullValidator<std::vector<std::string> >,Direction::Output));
    std::vector<double> params,errors;
    std::vector<std::string> parNames;

    for(int i=0;i<m_function->nParams();i++)
    {
      parNames.push_back(m_function->parameterName(i));
      params.push_back(m_function->getParameter(i));
      if (!standardDeviations.empty())
      {
        errors.push_back(standardDeviations[i]);
      }
      else
      {
        errors.push_back(0.);
      }
    }
    setProperty("Parameters",params);
    setProperty("Errors",errors);
    setProperty("ParameterNames",parNames);
    
    // minimizer may have dynamically allocated memory hence make sure this memory is freed up
    delete minimizer;

    return;
  }

  /**
   * If i-th parameter is transformed the derivative will be != 1.0.
   * The derivative is calculated numerically.
   * @param i :: The index of an active parameter
   * @return The transformation derivative
   */
  double GenericFit::transformationDerivative(int i)
  {
    int j = m_function->indexOfActive(i);
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
