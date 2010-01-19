//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit.h"
#include <sstream>
#include <numeric>
#include <math.h>
#include <iomanip>
#include "MantidKernel/Exception.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"
#include "MantidCurveFitting/PRConjugateGradientMinimizer.h"
#include "MantidCurveFitting/BFGS_Minimizer.h"

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

  /// The implementation of Jacobian
  class JacobianImpl1: public Jacobian
  {
  public:
    /// The pointer to the GSL's internal jacobian matrix
    gsl_matrix * m_J;
    /// Maps declared indeces to active. For fixed (tied) parameters holds -1
    std::vector<int> m_index;

    /// Set the pointer to the GSL's jacobian
    void setJ(gsl_matrix * J){m_J = J;}

    /// overwrite base method
    ///  @throw runtime_error Thrown if column of Jacobian to add number to does not exist
    void addNumberToColumn(const double& value, const int& iActiveP) 
    {
      if (iActiveP < m_J->size2)
      {
        // add penalty to first and last point and every 10th point in between
        m_J->data[iActiveP] += value;
        m_J->data[(m_J->size1-1)*m_J->size2 + iActiveP] += value;
        for (size_t iY = 9; iY < m_J->size1-1; iY++) 
          m_J->data[iY*m_J->size2 + iActiveP] += value;
      }
      else
      {
        throw std::runtime_error("Try to add number to column of Jacobian matrix which does not exist.");
      }   
    }
    /// overwrite base method
    void set(int iY, int iP, double value)
    {
      int j = m_index[iP];
      if (j >= 0) gsl_matrix_set(m_J,iY,j,value);
    }
  };

  /// Structure to contain least squares data and used by GSL
  struct FitData1 {
    /// Constructor
    FitData1(Fit* f);
    /// number of points to be fitted (size of X, Y and sqrtWeightData arrays)
    size_t n;
    /// number of (active) fit parameters
    size_t p;
    /// the data to be fitted (abscissae)
    double * X;
    /// the data to be fitted (ordinates)
    const double * Y;
    /// the standard deviations of the Y data points
    double * sqrtWeightData;
    /// pointer to instance of Fit
    Fit* fit;
    /// Jacobi matrix interface
    JacobianImpl1 J;
    /// To use the none least-squares gsl algorithms within the gsl least-squared framework
    /// include here container for calculuated data and calculated jacobian. 
    double * holdCalculatedData;
    gsl_matrix * holdCalculatedJacobian;
  };

  /** Fit GSL function wrapper
  * @param x Input function arguments
  * @param params Input data
  * @param f Output function values = (y_cal-y_data)/sigma for each data point
  * @return A GSL status information
  */
  static int gsl_f(const gsl_vector * x, void *params, gsl_vector * f) {

    struct FitData1 *p = (struct FitData1 *)params;
    p->fit->function (x->data, f->data, p->X, p->n);

    // function() return calculated data values. Need to convert this values into
    // calculated-observed devided by error values used by GSL

    for (size_t i = 0; i<p->n; i++)
      f->data[i] = 
      ( f->data[i] - p->Y[i] ) * p->sqrtWeightData[i];

    return GSL_SUCCESS;
  }

  /** Fit GSL derivative function wrapper
  * @param x Input function arguments
  * @param params Input data
  * @param J Output derivatives
  * @return A GSL status information
  */
  static int gsl_df(const gsl_vector * x, void *params, gsl_matrix * J) {

    struct FitData1 *p = (struct FitData1 *)params;

    p->J.setJ(J);

    p->fit->functionDeriv (x->data, &p->J, p->X, p->n);

    // functionDeriv() return derivatives of calculated data values. Need to convert this values into
    // derivatives of calculated-observed devided by error values used by GSL

    for (size_t iY = 0; iY < p->n; iY++) 
      for (size_t iP = 0; iP < p->p; iP++) 
        J->data[iY*p->p + iP] *= p->sqrtWeightData[iY];

    return GSL_SUCCESS;
  }

  /** Fit derivatives and function GSL wrapper
  * @param x Input function arguments
  * @param params Input data
  * @param f Output function values = (y_cal-y_cal)/sigma for each data point
  * @param J Output derivatives
  * @return A GSL status information
  */
  static int gsl_fdf(const gsl_vector * x, void *params,
    gsl_vector * f, gsl_matrix * J) {
      gsl_f(x, params, f);
      gsl_df(x, params, J);
      return GSL_SUCCESS;
  }

  /** Calculating least-squared cost function from fitting function
  *
  * @param x Input function arguments
  * @param params Input data
  * @return Value of least squared cost function
  */
  static double gsl_costFunction(const gsl_vector * x, void *params)
  {

    struct FitData1 *p = (struct FitData1 *)params;
    double * l_holdCalculatedData = p->holdCalculatedData;

    p->fit->function (x->data, l_holdCalculatedData, p->X, p->n);

    // function() return calculated data values. Need to convert this values into
    // calculated-observed devided by error values used by GSL
    for (size_t i = 0; i<p->n; i++)
      l_holdCalculatedData[i] = 
      (  l_holdCalculatedData[i] - p->Y[i] ) * p->sqrtWeightData[i];

    double retVal = 0.0;

    for (unsigned int i = 0; i < p->n; i++)
      retVal += l_holdCalculatedData[i]*l_holdCalculatedData[i];


    return retVal;
  }

  /** Calculating derivatives of least-squared cost function
  *
  * @param x Input function arguments
  * @param params Input data
  * @param df Derivatives cost function
  */
  static void gsl_costFunction_df(const gsl_vector * x, void *params, gsl_vector *df)
  {

    struct FitData1 *p = (struct FitData1 *)params;
    double * l_holdCalculatedData = p->holdCalculatedData;

    p->fit->function (x->data, l_holdCalculatedData, p->X, p->n);

    p->J.setJ(p->holdCalculatedJacobian);
    p->fit->functionDeriv (x->data, &p->J, p->X, p->n);

    for (size_t iP = 0; iP < p->p; iP++) 
    {
      df->data[iP] = 0.0;
      for (size_t iY = 0; iY < p->n; iY++) 
      {
        df->data[iP] += 2.0*(l_holdCalculatedData[iY]-p->Y[iY]) * p->holdCalculatedJacobian->data[iY*p->p + iP] 
                        * p->sqrtWeightData[iY]*p->sqrtWeightData[iY];
      }
    }
  }

  /** Return both derivatives and function value of least-squared cost function. This function is
  *   required by the GSL none least squares multidimensional fitting framework
  *
  * @param x Input function arguments
  * @param params Input data
  * @param f cost function value
  * @param df Derivatives of cost function
  */
  static void gsl_costFunction_fdf(const gsl_vector * x, void *params, double *f, gsl_vector *df)
  {
    *f = gsl_costFunction(x, params);
    gsl_costFunction_df(x, params, df); 
  }

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

    declareProperty("Function","","Parameters defining the fitting function and its initial values" );
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
    try
    {
      const std::vector<double> inTest(nActive(),1.0);
      std::vector<double> outTest(nActive());
      const double xValuesTest = 0;
      JacobianImpl1 J;
      boost::shared_ptr<gsl_matrix> M( gsl_matrix_alloc(nActive(),1) );
      J.setJ(M.get());
      // note nData set to zero (last argument) hence this should avoid further memory problems
      functionDeriv(NULL, &J, &xValuesTest, 0);  
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
      Mantid::API::MatrixWorkspace::MaskList mlist = localworkspace->maskedBins(histNumber);
      Mantid::API::MatrixWorkspace::MaskList::iterator it = mlist.begin();
      for(;it!=mlist.end();it++)
      {
        l_data.sqrtWeightData[it->first] = 0.;
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
    int status;
    //bool simplexFallBack = false; // set to true if levenberg-marquardt fails
    double finalCostFuncVal;
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
      g_log.information() << m_function->parameterName(i) << " = " << m_function->parameter(i) << "  \n";


    // also output summary to properties

    setProperty("Output Status", reportOfFit);
    setProperty("Output Chi^2/DoF", finalCostFuncVal);
    setProperty("Minimizer", methodUsed);
    

    // if Output property is specified output additional workspaces

    std::string output = getProperty("Output");
    if (!output.empty())
    {
      gsl_matrix *covar;
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
        for(size_t i=0; i < m_function->nParams(); i++)
        {
          standardDeviations.push_back(1.0);
          if (m_function->isActive(i))
          {
            standardDeviations[i] = sqrt(gsl_matrix_get(covar,iPNotFixed,iPNotFixed));
            if (m_function->activeParameter(iPNotFixed) != m_function->parameter(m_function->indexOfActive(iPNotFixed)))
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
        for(size_t i=0; i < m_function->nParams(); i++) 
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
              row << standardDeviations[i];
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

      for(size_t i=0;i<m_function->nParams();i++)
      {
        Mantid::API::TableRow row = m_result->appendRow();
        row << m_function->parameterName(i) << m_function->parameter(i);
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
      function(NULL, lOut, l_data.X, l_data.n);

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
    gsl_matrix_free (l_data.holdCalculatedJacobian);

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
    m_function->functionWithConstraint(out,xValues,nData);
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
    m_function->functionDerivWithConstraint(out,xValues,nData);
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

    typedef Poco::StringTokenizer tokenizer;
    tokenizer functions(input, ";", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

    bool isComposite = functions.count() > 1;
    API::IFunction* function;

    if (isComposite)
    {
      function = API::FunctionFactory::Instance().createFunction("CompositeFunction");
      setFunction(function);
    }

    // Loop over functions. 
    for (tokenizer::Iterator ifun = functions.begin(); ifun != functions.end(); ++ifun)
    {
      std::string functionName;
      tokenizer params(*ifun, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
      std::vector<std::pair<std::string,std::string> > param;
      // Loop over function parameters to fill in param map: param[<name>]=<init_value>
      for (tokenizer::Iterator par = params.begin(); par != params.end(); ++par)
      {
        tokenizer name_value(*par, "=", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
        if (name_value.count() > 1)
        {
          std::string name = name_value[0];
          //std::transform(name.begin(), name.end(), name.begin(), toupper);
          if (name == "name")
          {
            functionName = name_value[1];
          }
          else
          {
            param.push_back(std::pair<std::string,std::string>(name,name_value[1]));
          }
        }
      }

      // param["name"] gives the name(type) of the function
      //std::string functionName = param["name"];
      if (functionName.empty())
        throw std::runtime_error("Function is not defined");

      API::IFunction* fun = API::FunctionFactory::Instance().createFunction(functionName);

      // Loop over param to set the initial values and constraints
      std::vector<std::pair<std::string,std::string> >::const_iterator par = param.begin();
      for(;par!=param.end();++par)
      {
        const std::string& parName(par->first);
        std::string parValue = par->second;
        if (fun->hasAttribute(parName))
        {
          fun->setAttribute(parName,parValue);
        }
        else
        {
          // The init value part may contain a constraint setting string in brackets 
          size_t i = parValue.find_first_of('(');
          if (i != std::string::npos)
          {
            size_t j = parValue.find_last_of(')');
            if (j != std::string::npos && j > i+1)
            {
              tokenizer constraint(parValue.substr(i+1,j-i-1), ":", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
              if (constraint.count()>0)
              {
                BoundaryConstraint* con = new BoundaryConstraint(parName);
                if (constraint.count() >= 1)
                {
                  con->setLower(atof(constraint[0].c_str()));
                }
                if (constraint.count() > 1)
                {
                  con->setUpper(atof(constraint[1].c_str()));
                }
                fun->addConstraint(con);
              }
            }
            parValue.erase(i);
          }
          fun->getParameter(parName) = atof(parValue.c_str());
        }
      }
      // Attach the new function
      if (isComposite)
      {
        static_cast<API::CompositeFunction*>(function)->addFunction(fun);
      }
      else
      {
        setFunction(fun);
      }
    }// for(ifun)

    // Ties property is a comma separated list of formulas of the form:
    // tiedParamName = MathExpression, parameter names defined in the fitted function can be used
    // as variables in MathExpression. If the fitted function is a CompositeFunction parameter names
    // have form: f<index>.<name>, i.e. start with symbol 'f' (for function) followed by function's index 
    // in the CompositeFunction and a period '.' which is followed by the parameter name. e.g.
    // "f2.A = 2*f1.B + f5.C, f1.A=10"
    std::string inputTies = getProperty("Ties");
    if (!inputTies.empty())
    {
      tokenizer ties(inputTies, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
      for (tokenizer::Iterator tie = ties.begin(); tie != ties.end(); ++tie)
      {
        tokenizer name_value(*tie, "=", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
        if (name_value.count() > 1)
        {
          m_function->tie(name_value[0],name_value[1]);
        }
      }
    }

    std::string inputConstraints = getProperty("Constraints");
    if (!inputConstraints.empty())
    {
      tokenizer cons(inputConstraints, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
      for (tokenizer::Iterator con = cons.begin(); con != cons.end(); ++con)
      {
        tokenizer name_value(*con, "=", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
        if (name_value.count() > 1)
        {
          std::string conValue = name_value[1];
          // constraint value may be wrapped in brackets
          size_t i = conValue.find_first_of('(');
          if (i != std::string::npos)
          {
            size_t j = conValue.find_last_of(')');
            if (j != std::string::npos)
            {
              conValue = conValue.substr(i+1,j-i-1);
            }
            else
            {
              conValue.erase(i,1);
            }
          }
          
          tokenizer constraint(conValue, ":", tokenizer::TOK_TRIM);
          if (constraint.count()>0)
          {
            std::string parName = name_value[0];
            BoundaryConstraint* con = 0;
            API::CompositeFunction* cf = dynamic_cast<API::CompositeFunction*>(m_function);
            if (cf)
            {
              int iPar = cf->parameterIndex(parName);
              int iFun = cf->functionIndex(iPar);
              parName = cf->parameterLocalName(iPar);
              con = new BoundaryConstraint(parName);
              cf->getFunction(iFun)->addConstraint(con);
            }
            else
            {
              con = new BoundaryConstraint(parName);
              m_function->addConstraint(con);
            }

            if (constraint.count() >= 1 && !constraint[0].empty())
            {
              con->setLower(atof(constraint[0].c_str()));
            }
            if (constraint.count() > 1 && !constraint[1].empty())
            {
              con->setUpper(atof(constraint[1].c_str()));
            }
          }
        }
      }
    }

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
    double p0 = m_function->parameter(j);
    double ap0 = m_function->activeParameter(i);
    double dap = ap0 != 0.0? ap0 * 0.001 : 0.001;
    m_function->setActiveParameter(i,ap0 + dap);
    double deriv = ( m_function->parameter(j) - p0 ) / dap;
    m_function->parameter(j) = p0;
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
