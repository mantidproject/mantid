//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit.h"
#include <sstream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include "MantidKernel/Exception.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidCurveFitting/ICostFunction.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/CostFuncIgnorePosPeaks.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"
#include "MantidCurveFitting/PRConjugateGradientMinimizer.h"
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidCurveFitting/GSLFunctions.h"

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_blas.h>

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
  using DataObjects::Workspace2D_const_sptr;


  ///Destructor
  Fit::~Fit()
  {
    if (m_function) delete m_function;
  }

  /** Initialisation method
  */
  void Fit::init()
  {
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

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

    std::vector<std::string> minimizerOptions;
    minimizerOptions.push_back("Levenberg-Marquardt");
    minimizerOptions.push_back("Simplex");
    minimizerOptions.push_back("Conjugate gradient (Fletcher-Reeves imp.)");
    minimizerOptions.push_back("Conjugate gradient (Polak-Ribiere imp.)");
    minimizerOptions.push_back("BFGS");
    declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Direction::InOut);

    std::vector<std::string> costFuncOptions;
    minimizerOptions.push_back("Least squares");
    minimizerOptions.push_back("Ignore positive peaks");
    declareProperty("CostFunction","Least squares",new ListValidator(minimizerOptions),
      "The cost function to be used for the fit, default is Least squares", Direction::InOut);
  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void Fit::exec()
  {
    // Try to retrieve optional properties
    int histNumber = getProperty("WorkspaceIndex");
    const int maxInterations = getProperty("MaxIterations");

    // Get the input workspace
    DataObjects::Workspace2D_const_sptr localworkspace = getProperty("InputWorkspace");

    // number of histogram is equal to the number of spectra
    const int numberOfSpectra = localworkspace->getNumberHistograms();
    // Check that the index given is valid
    if ( histNumber >= numberOfSpectra )
    {
      g_log.warning("Invalid Workspace index given, using first Workspace");
      histNumber = 0;
    }

    // Retrieve the spectrum into a vector
    const MantidVec& XValues = localworkspace->readX(histNumber);
    const MantidVec& YValues = localworkspace->readY(histNumber);
    const MantidVec& YErrors = localworkspace->readE(histNumber);

    //Read in the fitting range data that we were sent
    double startX = getProperty("StartX");
    double endX = getProperty("EndX");
    //check if the values had been set, otherwise use defaults
    if ( isEmpty( startX ) )
    {
      startX = XValues.front();
      modifyStartOfRange(startX); // does nothing by default but derived class may provide a more intelligent value
    }
    if ( isEmpty( endX ) )
    {
      endX = XValues.back();
      modifyEndOfRange(endX); // does nothing by default but derived class may previde a more intelligent value
    }

    int m_minX;
    int m_maxX;

    // Check the validity of startX
    if ( startX < XValues.front() )
    {
      g_log.warning("StartX out of range! Set to start of frame.");
      startX = XValues.front();
    }
    // Get the corresponding bin boundary that comes before (or coincides with) this value
    for (m_minX = 0; XValues[m_minX+1] < startX; ++m_minX) {}

    // Check the validity of endX and get the bin boundary that come after (or coincides with) it
    if ( endX >= XValues.back() || endX < startX )
    {
      g_log.warning("EndX out of range! Set to end of frame");
      endX = XValues.back();
      m_maxX = YValues.size();
    }
    else
    {
      for (m_maxX = m_minX; XValues[m_maxX] < endX; ++m_maxX) {}
    }

    afterDataRangedDetermined(m_minX, m_maxX);

    // Process the Function property and create the function using FunctionFactory
    processParameters();

    if (m_function == NULL)
      throw std::runtime_error("Function was not set.");

    m_function->setWorkspace(localworkspace,histNumber,m_minX, m_maxX);

    // force initial parameters to satisfy constraints of function
    m_function->setParametersToSatisfyConstraints();

    // check if derivative defined in derived class
    bool isDerivDefined = true;
      const std::vector<double> inTest(nActive(),1.0);
      std::vector<double> outTest(nActive());
      const double xValuesTest = 0;
      JacobianImpl1 J;
      gsl_matrix* M( gsl_matrix_alloc(1,nActive()) );
      J.setJ(M);
      try
      {
      // note nData set to zero (last argument) hence this should avoid further memory problems
      functionDeriv(NULL, &J, &xValuesTest, 0);
    }
    catch (Exception::NotImplementedError&)
    {
      isDerivDefined = false;
    }
    gsl_matrix_free(M);

    // What minimizer to use
    std::string methodUsed = getProperty("Minimizer");
    if ( !isDerivDefined && methodUsed.compare("Simplex") != 0 )
    {
      methodUsed = "Simplex";
      g_log.information() << "No derivatives available for this fitting function"
                          << " therefore Simplex method used for fitting\n";
    }

    // create and populate GSL data container warn user if l_data.n < l_data.p 
    // since as a rule of thumb this is required as a minimum to obtained 'accurate'
    // fitting parameter values.

    FitData1 l_data(this);

    l_data.p = m_function->nActive();
    l_data.n = m_maxX - m_minX; // m_minX and m_maxX are array index markers. I.e. e.g. 0 & 19.
    if (l_data.p == 0)
    {
      g_log.error("There are no active parameters.");
      throw std::runtime_error("There are no active parameters.");
    }
    if (l_data.n == 0)
    {
      g_log.error("The data set is empty.");
      throw std::runtime_error("The data set is empty.");
    }
    if (l_data.n < l_data.p)
    {
      g_log.error("Number of data points less than number of parameters to be fitted.");
      throw std::runtime_error("Number of data points less than number of parameters to be fitted.");
    }
    l_data.X = new double[l_data.n];
    l_data.sqrtWeightData = new double[l_data.n];
    l_data.holdCalculatedData = new double[l_data.n];
    l_data.holdCalculatedJacobian =  gsl_matrix_alloc (l_data.n, l_data.p);

    // check if histogram data in which case use mid points of histogram bins

    const bool isHistogram = localworkspace->isHistogramData();
    for (unsigned int i = 0; i < l_data.n; ++i)
    {
      if (isHistogram)
        l_data.X[i] = 0.5*(XValues[m_minX+i]+XValues[m_minX+i+1]); // take mid-point if histogram bin
      else
        l_data.X[i] = XValues[m_minX+i];
    }

    l_data.Y = &YValues[m_minX];


    // check that no error is negative or zero

    for (unsigned int i = 0; i < l_data.n; ++i)
    {
      if (YErrors[m_minX+i] <= 0.0)
        l_data.sqrtWeightData[i] = 1.0;
      else
        l_data.sqrtWeightData[i] = 1./YErrors[m_minX+i];
    }

    if (localworkspace->hasMaskedBins(histNumber))
    {
      const MatrixWorkspace::MaskList& mlist = localworkspace->maskedBins(histNumber);
      MatrixWorkspace::MaskList::const_iterator it = mlist.begin();
      for(;it!=mlist.end();it++)
      {
        l_data.sqrtWeightData[it->first-m_minX] = 0.;
      }
    }


    // set-up initial guess for fit parameters

    gsl_vector *initFuncArg;
    initFuncArg = gsl_vector_alloc(l_data.p);

    for (size_t i = 0; i < nActive(); i++)
    {
        gsl_vector_set(initFuncArg, i, m_function->activeParameter(i));
    }

    // set-up GSL container to be used with GSL simplex algorithm

    gsl_multimin_function gslSimplexContainer;
    gslSimplexContainer.n = l_data.p;  // n here refers to number of parameters
    gslSimplexContainer.f = &gsl_costFunction;
    gslSimplexContainer.params = &l_data;


    // set-up GSL container to be used with none-least squares GSL routines using derivatives

    gsl_multimin_function_fdf gslMultiminContainer;
    gslMultiminContainer.n = l_data.p;  // n here refers to number of parameters
    gslMultiminContainer.f = &gsl_costFunction;
    gslMultiminContainer.df = &gsl_costFunction_df;
    gslMultiminContainer.fdf = &gsl_costFunction_fdf;
    gslMultiminContainer.params = &l_data;


    // set-up GSL least squares container

    gsl_multifit_function_fdf gslLeastSquaresContainer;
    gslLeastSquaresContainer.f = &gsl_f;
    gslLeastSquaresContainer.df = &gsl_df;
    gslLeastSquaresContainer.fdf = &gsl_fdf;
    gslLeastSquaresContainer.n = l_data.n;
    gslLeastSquaresContainer.p = l_data.p;
    gslLeastSquaresContainer.params = &l_data;


    // set-up which cost function to use

    std::string costFunction = getProperty("CostFunction");
    if ( methodUsed.compare("Levenberg-Marquardt") == 0 )
    {

      if ( costFunction.compare("Least squares") != 0 )
      {
        g_log.information() << "Levenberg-Marquardt only works with Least squares"
                          << " revert cost function to least squares\n";
      }
      costFunction = "Least squares";
    }

    if ( costFunction.compare("Least squares") == 0 )
      l_data.costFunc = new CostFuncLeastSquares();
    else if ( costFunction.compare("Ignore positive peaks") == 0 )
      l_data.costFunc = new CostFuncIgnorePosPeaks();
    else
    {
      g_log.error("Unrecognised cost function in Fit. Default to Least squares\n");
      costFunction = "Least squares";
      l_data.costFunc = new CostFuncLeastSquares();
    }


    // set-up minimizer

    IFuncMinimizer* minimizer = NULL;

    if ( methodUsed.compare("Simplex") == 0 )
    {
      minimizer = new SimplexMinimizer(gslSimplexContainer, initFuncArg, 1.0);
    }
    else
    {
      if ( methodUsed.compare("Levenberg-Marquardt") == 0 )
        minimizer = new LevenbergMarquardtMinimizer(gslLeastSquaresContainer, initFuncArg, m_function); 
      else if ( methodUsed.compare("Conjugate gradient (Fletcher-Reeves imp.)") == 0 )
        minimizer = new FRConjugateGradientMinimizer(gslMultiminContainer, initFuncArg, gslLeastSquaresContainer);
      else if ( methodUsed.compare("Conjugate gradient (Polak-Ribiere imp.)") == 0 )
        minimizer = new PRConjugateGradientMinimizer(gslMultiminContainer, initFuncArg, gslLeastSquaresContainer);
      else if ( methodUsed.compare("BFGS") == 0 )
        minimizer = new BFGS_Minimizer(gslMultiminContainer, initFuncArg, gslLeastSquaresContainer);
      else
      {
        g_log.error("Unrecognised minimizer in Fit. Default to Levenberg-Marquardt\n");
        methodUsed = "Levenberg-Marquardt";
        minimizer = new LevenbergMarquardtMinimizer(gslLeastSquaresContainer, initFuncArg, m_function); 
      }
    }
    

    // finally do the fitting

    int iter = 0;
    int status = 0;
    //bool simplexFallBack = false; // set to true if levenberg-marquardt fails
    double finalCostFuncVal = 0.0;
    double dof = l_data.n - l_data.p;  // dof stands for degrees of freedom

    // Standard least-squares used if derivative function defined otherwise simplex
    Progress prog(this,0.0,1.0,maxInterations);
    if ( methodUsed.compare("Simplex") != 0 )
    {
      status = GSL_CONTINUE;
      while (status == GSL_CONTINUE && iter < maxInterations)
      {
        iter++;
        status = minimizer->iterate();

        // break if status is not success
        if (status)  
        { 
          // From experience it is found that gsl_multifit_fdfsolver_iterate occasionally get
          // stock - even after having achieved a sensible fit. This seem in particular to be a
          // problem on Linux. For now only fall back to Simplex if iter = 1 or 2, i.e.   
          // gsl_multifit_fdfsolver_iterate has failed on the first or second hurdle
          if (iter < 3)
          {
            methodUsed = "Simplex";
            //simplexFallBack = true;
            delete minimizer;
            minimizer = new SimplexMinimizer(gslSimplexContainer, initFuncArg, 1.0);
            iter = 0;
            g_log.warning() << "Fit algorithm using Levenberg-Marquardt failed "
              << "reporting the following: " << gsl_strerror(status) << "\n"
              << "Try using Simplex method instead\n";
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
            minimizer = new SimplexMinimizer(gslSimplexContainer, initFuncArg, 0.1);
            //iter = 0;
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

    std::string reportOfFit = gsl_strerror(status);

    g_log.information() << "Method used = " << methodUsed << "\n" <<
      "Iteration = " << iter << "\n" <<
      "Status = " << reportOfFit << "\n" <<
      "Chi^2/DoF = " << finalCostFuncVal << "\n";
    for (int i = 0; i < m_function->nParams(); i++)
    {
      g_log.information() << m_function->parameterName(i) << " = " << m_function->getParameter(i) << "  \n";
    }


    // also output summary to properties

    setProperty("Output Status", reportOfFit);
    setProperty("Output Chi^2/DoF", finalCostFuncVal);
    setProperty("Minimizer", methodUsed);
    setPropertyValue("Function",*m_function);
    

    // if Output property is specified output additional workspaces

    std::string output = getProperty("Output");
    if (!output.empty())
    {
      gsl_matrix *covar(NULL);
      std::vector<double> standardDeviations;

      // only if derivative is defined for fitting function create covariance matrix output workspace
      if ( methodUsed.compare("Simplex") != 0 )    
      {
        // calculate covariance matrix
        covar = gsl_matrix_alloc (l_data.p, l_data.p);
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

        // Create covariance matrix output workspace
        declareProperty(
          new WorkspaceProperty<API::ITableWorkspace>("OutputNormalisedCovarianceMatrix","",Direction::Output),
          "The name of the TableWorkspace in which to store the final covariance matrix" );
        setPropertyValue("OutputNormalisedCovarianceMatrix",output+"_NormalisedCovarianceMatrix");

        Mantid::API::ITableWorkspace_sptr m_covariance = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        m_covariance->addColumn("str","Name");
        std::vector<std::string> paramThatAreFitted; // used for populating 1st "name" column
        for(int i=0; i < m_function->nParams(); i++) 
        {
          if (m_function->isActive(i)) 
          {
            m_covariance->addColumn("double",m_function->parameterName(i));
            paramThatAreFitted.push_back(m_function->parameterName(i));
          }
        }

        for(size_t i=0;i<l_data.p;i++) 
        {
          Mantid::API::TableRow row = m_covariance->appendRow();
          row << paramThatAreFitted[i];
          for(size_t j=0;j<l_data.p;j++)
          {
            if (j == i)
              row << 1.0;
            else
            {
              row << 100.0*gsl_matrix_get(covar,i,j)/sqrt(gsl_matrix_get(covar,i,i)*gsl_matrix_get(covar,j,j));
            }
          }
        }

        setProperty("OutputNormalisedCovarianceMatrix",m_covariance);
      }

      // create output parameter table workspace to store final fit parameters 
      // including error estimates if derivative of fitting function defined

      declareProperty(
        new WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Direction::Output),
        "The name of the TableWorkspace in which to store the final fit parameters" );
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output), 
        "Name of the output Workspace holding resulting simlated spectrum");

      setPropertyValue("OutputParameters",output+"_Parameters");
      setPropertyValue("OutputWorkspace",output+"_Workspace");

      Mantid::API::ITableWorkspace_sptr m_result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      m_result->addColumn("str","Name");
      m_result->addColumn("double","Value");
      if ( methodUsed.compare("Simplex") != 0 ) 
        m_result->addColumn("double","Error");
      //Mantid::API::TableRow row = m_result->appendRow();
      //row << "Chi^2/DoF" << finalCostFuncVal;

      for(int i=0;i<m_function->nParams();i++)
      {
        Mantid::API::TableRow row = m_result->appendRow();
        row << m_function->parameterName(i) << m_function->getParameter(i);
        if ( methodUsed.compare("Simplex") != 0 && m_function->isActive(i)) 
        {
          row << standardDeviations[i];
        }
      }
      setProperty("OutputParameters",m_result);

      // Save the fitted and simulated spectra in the output workspace
      Workspace2D_const_sptr inputWorkspace = getProperty("InputWorkspace");
      int iSpec = getProperty("WorkspaceIndex");
      const MantidVec& inputX = inputWorkspace->readX(iSpec);
      const MantidVec& inputY = inputWorkspace->readY(iSpec);

      int histN = isHistogram ? 1 : 0;
      Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
        (
        Mantid::API::WorkspaceFactory::Instance().create(
        "Workspace2D",
        3,
        l_data.n + histN,
        l_data.n)
        );
      ws->setTitle("");
      ws->getAxis(0)->unit() = inputWorkspace->getAxis(0)->unit();//    UnitFactory::Instance().create("TOF");

      for(int i=0;i<3;i++)
        ws->dataX(i).assign(inputX.begin()+m_minX,inputX.begin()+m_maxX+histN);

      ws->dataY(0).assign(inputY.begin()+m_minX,inputY.begin()+m_maxX);

      MantidVec& Y = ws->dataY(1);
      MantidVec& E = ws->dataY(2);


      double* lOut = new double[l_data.n];  // to capture output from call to function()
      m_function->function( lOut, l_data.X, l_data.n);

      for(unsigned int i=0; i<l_data.n; i++) 
      {
        Y[i] = lOut[i]; 
        E[i] = l_data.Y[i] - Y[i];
      }

      delete [] lOut; 

      setProperty("OutputWorkspace",ws);

      if ( methodUsed.compare("Simplex") != 0 ) 
        gsl_matrix_free(covar);
    }



    // minimizer may have dynamically allocated memory hence make sure this memory is freed up

    delete minimizer;

    // clean up dynamically allocated gsl stuff

    delete [] l_data.X;
    delete [] l_data.sqrtWeightData;
    delete [] l_data.holdCalculatedData;
    delete l_data.costFunc;
    gsl_matrix_free (l_data.holdCalculatedJacobian);
    gsl_vector_free (initFuncArg);
    
    return;
  }

  /// Set a function for fitting
  void Fit::setFunction(API::IFunction* fun)
  {
    m_function = fun;
  }

/** Calculate the fitting function.
 *  @param in A pointer ot the input active function parameters
 *  @param out A pointer to the output fitting function buffer. The buffer must be large enough to receive nData double values.
 *        The fitting procedure will try to minimise Sum(out[i]^2)
 *  @param xValues The array of nData x-values.
 *  @param nData The size of the fitted data.
 */
  void Fit::function(const double* in, double* out, const double* xValues, const int& nData)
  {

    if (in) m_function->updateActive(in);
    m_function->function(out,xValues,nData);
  
    // Add penalty factor to function if any constraint is violated

    API::IConstraint* c = m_function->firstConstraint();
    if (!c) return;

    double penalty = c->check();
    while( (c = m_function->nextConstraint()) )
    {
      penalty += c->check();
    }

    // add penalty to first and last point and every 10th point in between
    if ( penalty != 0.0 )
    {
      out[0] += penalty;
      out[nData-1] += penalty;

      for (int i = 9; i < nData-1; i+=10)
      {
        out[i] += penalty;
      }
    }
  }

  /** Calculate derivates of fitting function
  *
  * @param in Input fitting parameter values
  * @param out Derivatives
  * @param xValues X values for data points
  * @param nData Number of data points
  */
  void Fit::functionDeriv(const double* in, Jacobian* out, const double* xValues, const int& nData)
  {
    if (in) m_function->updateActive(in);
    m_function->functionDeriv(out,xValues,nData);

    if (nData <= 0) return;

    API::IConstraint* c = m_function->firstConstraint();
    if (!c) return;

    do
    {  
      double penalty = c->checkDeriv();
      int i = m_function->getParameterIndex(*c);
      out->addNumberToColumn(penalty, m_function->activeIndex(i));
    }
    while( (c = m_function->nextConstraint()) );

    //std::cerr<<"-------------- Jacobian ---------------\n";
    //for(int i=0;i<nActive();i++)
    //  for(int j=0;j<nData;j++)
    //    std::cerr<<i<<' '<<j<<' '<<gsl_matrix_get(((JacobianImpl1*)out)->m_J,j,i)<<'\n';
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
    std::string input = getProperty("Function");
    if (input.empty()) return;

    std::string::size_type i = input.find_last_not_of(" \t\n\r");
    if (i == std::string::npos) return;
    if (i >= 0 && input[i] == ';')
    {
      input.erase(i);
    }

    std::string inputConstraints = getProperty("Constraints");
    if (!inputConstraints.empty())
    {
      if (input.find(';') != std::string::npos)
      {
        input += ";";
      }
      else
      {
        input += ",";
      }
      std::string::size_type i = inputConstraints.find_last_not_of(" \t\n\r");
      if (i >= 0 && inputConstraints[i] == ',')
      {
        inputConstraints.erase(i);
      }
      input += "constraints=("+inputConstraints+")";
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
      if (input.find(';') != std::string::npos)
      {
        input += ";";
      }
      else
      {
        input += ",";
      }
      std::string::size_type i = inputTies.find_last_not_of(" \t\n\r");
      if (i >= 0 && inputTies[i] == ',')
      {
        inputTies.erase(i);
      }
      input += "ties=("+inputTies+")";
    }

    setFunction(API::FunctionFactory::Instance().createInitialized(input));

  }

  /**
   * If i-th parameter is transformed the derivative will be != 1.0.
   * The derivative is calculated numerically.
   * @param i The index of an active parameter
   * @return The transformation derivative
   */
  double Fit::transformationDerivative(int i)
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

  /**
   * Constructor. Creates declared -> active index map
   * @param f Pointer to the Fit algorithm
   */
  FitData1::FitData1(Fit* f):fit(f)
  {
    int j = 0;
    for(int i=0;i<f->m_function->nParams();++i)
    {
      if (f->m_function->isActive(i))
      {
        J.m_index.push_back(j);
        j++;
      }
      else
        J.m_index.push_back(-1);
    }
  }

} // namespace Algorithm
} // namespace Mantid
