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

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_blas.h>
#include <boost/tokenizer.hpp>

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
    /**  Set a value to a Jacobian matrix element.
    *   @param iY The index of the data point.
    *   @param iP The index of the active parameter.
    *   @param value The derivative value.
    */
    void set(int iY, int iP, double value)
    {
      int j = m_index[iP];
      if (j >= 0) gsl_matrix_set(m_J,iY,j,value);
    }
    /// Set the pointer to the GSL's jacobian
    void setJ(gsl_matrix * J){m_J = J;}
  };

  /// Structure to contain least squares data and used by GSL
  struct FitData1 {
    /// Constructor
    FitData1(Fit* f);
    /// number of points to be fitted (size of X, Y and sigmaData arrays)
    size_t n;
    /// number of (active) fit parameters
    size_t p;
    /// the data to be fitted (abscissae)
    double * X;
    /// the data to be fitted (ordinates)
    const double * Y;
    /// the standard deviations of the Y data points
    double * sigmaData;
    /// pointer to instance of Fit
    Fit* fit;
    /// Needed to use the simplex algorithm within the gsl least-squared framework.
    /// Will store output function values from gsl_f
    double * forSimplexLSwrap;
    /// Jacobi matrix interface
    JacobianImpl1 J;
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
      ( f->data[i] - p->Y[i] ) / p->sigmaData[i];

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
        J->data[iY*p->p + iP] /= p->sigmaData[iY];

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

  /** Calculating the cost function assuming it has the least-squared
  format. Initially implemented to use the gsl simplex routine within
  the least-squared scheme.
  * @param x Input function arguments
  * @param params Input data
  * @return Value of least squared cost function
  */
  static double gsl_costFunction(const gsl_vector * x, void *params)
  {

    struct FitData1 *p = (struct FitData1 *)params;
    double * l_forSimplexLSwrap = p->forSimplexLSwrap;

    p->fit->function (x->data,
      l_forSimplexLSwrap,
      p->X,
      p->n);

    // function() return calculated data values. Need to convert this values into
    // calculated-observed devided by error values used by GSL
    for (size_t i = 0; i<p->n; i++)
      l_forSimplexLSwrap[i] = 
      (  l_forSimplexLSwrap[i] - p->Y[i] ) / p->sigmaData[i];

    double retVal = 0.0;

    for (unsigned int i = 0; i < p->n; i++)
      retVal += l_forSimplexLSwrap[i]*l_forSimplexLSwrap[i];


    return retVal;
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

    declareProperty("InputParameters","","Parameters defining the fitting function and its initial values" );
    declareProperty("Ties","","Math expressions that tie paraemters to other parameters or to constants" );

    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("Output Status","", Direction::Output);
    declareProperty("Output Chi^2/DoF",0.0, Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    declareProperty("Output","","If not empty OutputParameters TableWorksace and OutputWorkspace will be created.");
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

    processParameters(localworkspace,histNumber,m_minX, m_maxX);

    if (m_function == NULL)
      throw std::runtime_error("Function has not been set.");

    // check if derivative defined in derived class
    bool isDerivDefined = true;
    try
    {
      const std::vector<double> inTest(nParams(),1.0);
      std::vector<double> outTest(nParams());
      const double xValuesTest = 0;
      JacobianImpl1 J;
      boost::shared_ptr<gsl_matrix> M( gsl_matrix_alloc(nParams(),1) );
      J.setJ(M.get());
      // note nData set to zero (last argument) hence this should avoid further memory problems
      functionDeriv(NULL, &J, &xValuesTest, 0);  
    }
    catch (Exception::NotImplementedError&)
    {
      isDerivDefined = false;
    }

    // create and populate GSL data container warn user if l_data.n < l_data.p 
    // since as a rule of thumb this is required as a minimum to obtained 'accurate'
    // fitting parameter values.

    FitData1 l_data(this);

    l_data.p = m_function->nActive();
    l_data.n = m_maxX - m_minX; // m_minX and m_maxX are array index markers. I.e. e.g. 0 & 19.
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
    l_data.sigmaData = new double[l_data.n];
    l_data.forSimplexLSwrap = new double[l_data.n];

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
    //l_data.sigmaData = &YErrors[m_minX];


    // check that no error is negative or zero

    for (unsigned int i = 0; i < l_data.n; ++i)
    {
      if (YErrors[i] <= 0.0)
        l_data.sigmaData[i] = 1.0;
      else
        l_data.sigmaData[i] = YErrors[i];
    }


    // set-up initial guess for fit parameters

    gsl_vector *initFuncArg;
    initFuncArg = gsl_vector_alloc(l_data.p);

    for (size_t i = 0; i < nParams(); i++)
    {
        //gsl_vector_set(initFuncArg, i, m_fittedParameter[i]);
        gsl_vector_set(initFuncArg, i, m_function->activeParameter(i));
    }


    // set-up GSL container to be used with GSL simplex algorithm

    gsl_multimin_function gslSimplexContainer;
    gslSimplexContainer.n = l_data.p;  // n here refers to number of parameters
    gslSimplexContainer.f = &gsl_costFunction;
    gslSimplexContainer.params = &l_data;


    // set-up GSL least squares container

    gsl_multifit_function_fdf f;
    f.f = &gsl_f;
    f.df = &gsl_df;
    f.fdf = &gsl_fdf;
    f.n = l_data.n;
    f.p = l_data.p;
    f.params = &l_data;

    // set-up remaining GSL machinery for least squared

    const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
    gsl_multifit_fdfsolver *s = NULL;
    if (isDerivDefined)
    {
      s = gsl_multifit_fdfsolver_alloc(T, l_data.n, l_data.p);
      gsl_multifit_fdfsolver_set(s, &f, initFuncArg);
    }

    // set-up remaining GSL machinery to use simplex algorithm

    const gsl_multimin_fminimizer_type *simplexType = gsl_multimin_fminimizer_nmsimplex;
    gsl_multimin_fminimizer *simplexMinimizer = NULL;
    gsl_vector *simplexStepSize = NULL;
    if (!isDerivDefined)
    {
      simplexMinimizer = gsl_multimin_fminimizer_alloc(simplexType, l_data.p);
      simplexStepSize = gsl_vector_alloc(l_data.p);
      gsl_vector_set_all (simplexStepSize, 1.0);  // is this always a sensible starting step size?
      gsl_multimin_fminimizer_set(simplexMinimizer, &gslSimplexContainer, initFuncArg, simplexStepSize);
    }

    // finally do the fitting

    int iter = 0;
    int status;
    double size; // for simplex algorithm
    double finalCostFuncVal;
    double dof = l_data.n - l_data.p;  // dof stands for degrees of freedom

    // Standard least-squares used if derivative function defined otherwise simplex
    Progress prog(this,0.0,1.0,maxInterations);
    if (isDerivDefined)
    {
      status = GSL_CONTINUE;
      while (status == GSL_CONTINUE && iter < maxInterations)
      {
        iter++;
        status = gsl_multifit_fdfsolver_iterate(s);

        //std::cout << "status " << gsl_strerror(status) << " number " << status << std::endl;
        //for (size_t i = 0; i < m_fittedParameter.size(); i++)
        //  std::cout << m_function->nameOfActive(i) << " = " << gsl_vector_get(s->x,i) << "  \n";

        // break if not status success
        // Note that if fitting to linear function you might get that
        // status = GSL_ETOLF, which means cannot reach the specified tolerance in function value
        // We might interpret this as a success convergence for the linear case
        if (status)  
          break;

        status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
        //std::cout << "chi-out " << gsl_blas_dnrm2(s->f) << std::endl;
        prog.report();
      }

      double chi = gsl_blas_dnrm2(s->f);
      finalCostFuncVal = chi*chi / dof;

      // put final converged fitting values back into m_fittedParameter
      for (size_t i = 0; i < nParams(); i++)
          m_function->setActiveParameter(i,gsl_vector_get(s->x,i));
    }
    else
    {
      status = GSL_CONTINUE;
      while (status == GSL_CONTINUE && iter < maxInterations)
      {
        iter++;
        status = gsl_multimin_fminimizer_iterate(simplexMinimizer);

        if (status)  // break if error
          break;

        size = gsl_multimin_fminimizer_size(simplexMinimizer);
        status = gsl_multimin_test_size(size, 1e-2);
        prog.report();
      }

      finalCostFuncVal = simplexMinimizer->fval / dof;

      // put final converged fitting values back into m_fittedParameter
      for (unsigned int i = 0; i < nParams(); i++)
          //m_fittedParameter[i] = gsl_vector_get(simplexMinimizer->x,i);
          m_function->setActiveParameter(i, gsl_vector_get(simplexMinimizer->x,i));
    }

    // Output summary to log file

    std::string reportOfFit = gsl_strerror(status);

    g_log.information() << "Iteration = " << iter << "\n" <<
      "Status = " << reportOfFit << "\n" <<
      "Chi^2/DoF = " << finalCostFuncVal << "\n";
    for (int i = 0; i < m_function->nParams(); i++)
      g_log.information() << m_function->parameterName(i) << " = " << m_function->parameter(i) << "  \n";


    // also output summary to properties

    setProperty("Output Status", reportOfFit);
    setProperty("Output Chi^2/DoF", finalCostFuncVal);


    if (isDerivDefined)
      gsl_multifit_fdfsolver_free(s);
    else
    {
      gsl_vector_free(simplexStepSize);
      gsl_multimin_fminimizer_free(simplexMinimizer);
    }

    std::string output = getProperty("Output");
    if (!output.empty())
    {

      declareProperty(
        new WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Direction::Output),
        "The name of the TableWorkspace in which to store the final fit parameters" );
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output), 
        "Name of the output Workspace holding resulting simlated spectrum");

      setPropertyValue("OutputParameters",output+"_Parameters");
      setPropertyValue("OutputWorkspace",output+"_Workspace");

      // Save the final fit parameters in the output table workspace
      Mantid::API::ITableWorkspace_sptr m_result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      m_result->addColumn("str","Name");
      m_result->addColumn("double","Value");
      for(size_t i=0;i<m_function->nParams();i++)
      {
        Mantid::API::TableRow row = m_result->appendRow();
        row << m_function->parameterName(i) << m_function->parameter(i);
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

    }

    // clean up dynamically allocated gsl stuff

    delete [] l_data.X;
    delete [] l_data.sigmaData;
    delete [] l_data.forSimplexLSwrap;

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
  }

  /** Base class implementation of derivative function throws error. This is to check if such a function is provided
  by derivative class. In the derived classes this method must return the derivatives of the resuduals function
  (defined in void Fit::function(const double*, double*, const double*, const double*, const double*, const int&))
  with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
  algorithm is used.
  * @param in Input fitting parameter values
  * @param out Derivatives
  * @param xValues X values for data points
  * @param nData Number of data points
  */
  void Fit::functionDeriv(const double* in, Jacobian* out, const double* xValues, const int& nData)
  {
    if (in) m_function->updateActive(in);
    m_function->functionDeriv(out,xValues,nData);
    //std::cerr<<"-------------- Jacobian ---------------\n";
    //for(int i=0;i<nParams();i++)
    //  for(int j=0;j<nData;j++)
    //    std::cerr<<i<<' '<<j<<' '<<gsl_matrix_get(((JacobianImpl1*)out)->m_J,j,i)<<'\n';
  }

  /**
   * Process input parameters and create the fitting function.
   */
  void Fit::processParameters(boost::shared_ptr<const DataObjects::Workspace2D> workspace,int spec,int xMin,int xMax)
  {

    // Parameters of different functions are separated by ';'. Parameters of the same function
    // are separated by ','. parameterName=value pairs are used to set a parameter value. For each function
    // "function" parameter must be set to a function name. E.g.
    // InputParameters = "function=LinearBackground,A0=0,A1=1; function = Gaussian, PeakCentre=10.,Sigma=1"
    std::string input = getProperty("InputParameters");
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

    for (tokenizer::Iterator ifun = functions.begin(); ifun != functions.end(); ++ifun)
    {
      tokenizer params(*ifun, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
      std::map<std::string,std::string> param;
      for (tokenizer::Iterator par = params.begin(); par != params.end(); ++par)
      {
        tokenizer name_value(*par, "=", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
        if (name_value.count() > 1)
        {
          std::string name = name_value[0];
          //std::transform(name.begin(), name.end(), name.begin(), toupper);
          param[name] = name_value[1];
        }
      }

      std::string functionName = param["function"];
      if (functionName.empty())
        throw std::runtime_error("Function is not defined");

      API::IFunction* fun = API::FunctionFactory::Instance().createFunction(functionName);
      fun->initialize(workspace,spec,xMin,xMax);

      if (isComposite)
        static_cast<API::CompositeFunction*>(function)->addFunction(fun);
      else
        setFunction(fun);

      std::map<std::string,std::string>::const_iterator par = param.begin();
      for(;par!=param.end();++par)
      {
        if (par->first != "function")
        {
          //fun->getParameter(par->first) = boost::lexical_cast<double>(par->second);
          fun->getParameter(par->first) = atof(par->second.c_str());
        }
      }
    }

    // Ties property is a comma separated list of formulas of the form:
    // tiedParamName = MathExpression, parameter names defined in the fitted function can be used
    // as variables in MathExpression. If the fitted function is a CompositeFunction parameter names
    // have form: f<index>.<name>, i.e. start with symbol 'f' (for function) followed by function's index 
    // in the CompositeFunction and a period '.' which is followed by the parameter name. e.g.
    // "f2.A = 2*f1.B + f5.C, f1.A=10"
    std::string inputTies = getProperty("Ties");
    if (inputTies.empty()) return;

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
