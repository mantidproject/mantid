// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Algorithms/Fit1D.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitFactory.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_version.h>

#include <cmath>
#include <numeric>
#include <sstream>

namespace Mantid::CurveFitting::Algorithms {

using namespace Kernel;
using API::Jacobian;
using API::MatrixWorkspace;
using API::MatrixWorkspace_const_sptr;
using API::Progress;
using API::WorkspaceProperty;

/// The implementation of Jacobian
class JacobianImpl : public Jacobian {
public:
  /// Default constructor
  JacobianImpl() : Jacobian(), m_J(nullptr) {};

  /// The index map
  std::map<int, int> m_map;
  /**  Set a value to a Jacobian matrix element.
   *   @param iY :: The index of the data point.
   *   @param iP :: The index of the parameter. It does not depend on the number
   * of fixed parameters in a particular fit.
   *   @param value :: The derivative value.
   */
  void set(size_t iY, size_t iP, double value) override {
    int j = m_map[static_cast<int>(iP)];
    if (j >= 0)
      gsl_matrix_set(m_J, iY, j, value);
  }
  /** Get a value to a Jacobian matrix element.
   *   @param iY :: The index of the data point.
   *   @param iP :: The index of the parameter. It does not depend on the number
   * of fixed parameters in a particular fit.
   */
  double get(size_t iY, size_t iP) override {
    int j = m_map[static_cast<int>(iP)];
    if (j >= 0)
      return gsl_matrix_get(m_J, iY, j);
    return 0.0;
  }
  /** Zero all matrix elements.
   */
  void zero() override { gsl_matrix_set_zero(m_J); }
  /// Set the pointer to the GSL's jacobian
  void setJ(gsl_matrix *J) { m_J = J; }

private:
  /// The pointer to the GSL's internal jacobian matrix
  gsl_matrix *m_J;
};

/// Structure to contain least squares data and used by GSL
struct FitData {
  /// Constructor
  FitData(Fit1D *fit, const std::string &fixed);
  /// number of points to be fitted (size of X, Y and sigmaData arrays)
  size_t n;
  /// number of (active) fit parameters
  size_t p;
  /// the data to be fitted (abscissae)
  double *X;
  /// the data to be fitted (ordinates)
  const double *Y;
  /// the standard deviations of the Y data points
  double *sigmaData;
  /// pointer to instance of Fit1D
  Fit1D *fit1D;
  /// Needed to use the simplex algorithm within the gsl least-squared
  /// framework.
  /// Will store output function values from gsl_f
  double *forSimplexLSwrap;
  /// A copy of the parameters
  double *parameters;
  /// Holds a boolean for each parameter, true if it's active or false if it's
  /// fixed
  std::vector<bool> active;
  /// Jacobi matrix interface
  JacobianImpl J;
};

/** Fit1D GSL function wrapper
 * @param x :: Input function arguments
 * @param params :: Input data
 * @param f :: Output function values = (y_cal-y_data)/sigma for each data point
 * @return A GSL status information
 */
static int gsl_f(const gsl_vector *x, void *params, gsl_vector *f) {
  auto fitParams = static_cast<FitData *>(params);
  for (size_t i = 0, j = 0; i < fitParams->active.size(); i++)
    if (fitParams->active[i])
      fitParams->parameters[i] = x->data[j++];

  fitParams->fit1D->function(fitParams->parameters, f->data, fitParams->X, fitParams->n);

  // function() return calculated data values. Need to convert this values into
  // calculated-observed devided by error values used by GSL

  for (size_t i = 0; i < fitParams->n; i++)
    f->data[i] = (f->data[i] - fitParams->Y[i]) / fitParams->sigmaData[i];

  return GSL_SUCCESS;
}

/** Fit1D GSL derivative function wrapper
 * @param x :: Input function arguments
 * @param params :: Input data
 * @param J :: Output derivatives
 * @return A GSL status information
 */
static int gsl_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  auto fitParams = static_cast<FitData *>(params);
  for (size_t i = 0, j = 0; i < fitParams->active.size(); i++) {
    if (fitParams->active[i])
      fitParams->parameters[i] = x->data[j++];
  }

  fitParams->J.setJ(J);

  fitParams->fit1D->functionDeriv(fitParams->parameters, &fitParams->J, fitParams->X, fitParams->n);

  // functionDeriv() return derivatives of calculated data values. Need to
  // convert this values into derivatives of calculated-observed devided
  // by error values used by GSL

  for (size_t iY = 0; iY < fitParams->n; iY++)
    for (size_t iP = 0; iP < fitParams->p; iP++)
      J->data[iY * fitParams->p + iP] /= fitParams->sigmaData[iY];

  return GSL_SUCCESS;
}

/** Fit1D derivatives and function GSL wrapper
 * @param x :: Input function arguments
 * @param params :: Input data
 * @param f :: Output function values = (y_cal-y_cal)/sigma for each data point
 * @param J :: Output derivatives
 * @return A GSL status information
 */
static int gsl_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  gsl_f(x, params, f);
  gsl_df(x, params, J);
  return GSL_SUCCESS;
}

/** Calculating the cost function assuming it has the least-squared
    format. Initially implemented to use the gsl simplex routine within
    the least-squared scheme.
* @param x :: Input function arguments
* @param params :: Input data
* @return Value of least squared cost function
*/
static double gsl_costFunction(const gsl_vector *x, void *params) {
  auto fitParams = static_cast<FitData *>(params);
  double *l_forSimplexLSwrap = fitParams->forSimplexLSwrap;

  for (size_t i = 0, j = 0; i < fitParams->active.size(); i++)
    if (fitParams->active[i])
      fitParams->parameters[i] = x->data[j++];

  fitParams->fit1D->function(fitParams->parameters, l_forSimplexLSwrap, fitParams->X, fitParams->n);

  // function() return calculated data values. Need to convert this values into
  // calculated-observed devided by error values used by GSL
  for (size_t i = 0; i < fitParams->n; i++)
    l_forSimplexLSwrap[i] = (l_forSimplexLSwrap[i] - fitParams->Y[i]) / fitParams->sigmaData[i];

  double retVal = 0.0;

  for (unsigned int i = 0; i < fitParams->n; i++)
    retVal += l_forSimplexLSwrap[i] * l_forSimplexLSwrap[i];

  return retVal;
}

/** Base class implementation of derivative function throws error. This is to
check if such a function is provided
    by derivative class. In the derived classes this method must return the
derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*,
const double*, const double*, const size_t))
    with respect to the fit parameters. If this method is not reimplemented the
derivative free simplex minimization
    algorithm is used.
* @param in :: Input fitting parameter values
* @param out :: Derivatives
* @param xValues :: X values for data points
* @param nData :: Number of data points
 */
void Fit1D::functionDeriv(const double *in, Jacobian *out, const double *xValues, const size_t nData) {
  UNUSED_ARG(in);
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
  throw Exception::NotImplementedError("No derivative function provided");
}

/** Overload this function if the actual fitted parameters are different from
    those the user specifies. Input is an Array of initial values of the fit
    parameters as listed in declareParameters(). By default no changes is made
    to these. If this method is overloaded, the method
modifyFinalFittedParameters()
    must also be overloaded.
* @param fittedParameter :: Values of fitting parameters in the order listed in
declareParameters()
 */
void Fit1D::modifyInitialFittedParameters(std::vector<double> &fittedParameter) {
  (void)fittedParameter; // Avoid compiler warning
}

/** If modifyInitialFittedParameters is overloaded this method must also be
overloaded
    to reverse the effect of modifyInitialFittedParameters before outputting the
results back to the user.
* @param fittedParameter :: Values of fitting parameters in the order listed in
declareParameters()
 */
void Fit1D::modifyFinalFittedParameters(std::vector<double> &fittedParameter) {
  (void)fittedParameter; // Avoid compiler warning
}

/** Initialisation method
 */
void Fit1D::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Name of the input Workspace");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "The Workspace to fit, uses the workspace numbering of the "
                  "spectra (default 0)");
  declareProperty("StartX", EMPTY_DBL(),
                  "A value of x in, or on the low x "
                  "boundary of, the first bin to "
                  "include in\n"
                  "the fit (default lowest value of x)");
  declareProperty("EndX", EMPTY_DBL(),
                  "A value in, or on the high x boundary "
                  "of, the last bin the fitting range\n"
                  "(default the highest value of x)");

  size_t i0 = getProperties().size();

  // declare parameters specific to a given fitting function
  declareParameters();

  // load the name of these specific parameter into a vector for later use
  const std::vector<Property *> props = getProperties();
  for (size_t i = i0; i < props.size(); i++) {
    m_parameterNames.emplace_back(props[i]->name());
  }

  declareProperty("Fix", "",
                  "A list of comma separated parameter names which "
                  "should be fixed in the fit");
  declareProperty("MaxIterations", 500, mustBePositive,
                  "Stop after this number of iterations if a good fit is not found");
  declareProperty("OutputStatus", "", Direction::Output);
  declareProperty("OutputChi2overDoF", 0.0, Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();

  declareAdditionalProperties();

  declareProperty("Output", "",
                  "If not empty OutputParameters TableWorksace "
                  "and OutputWorkspace will be created.");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Fit1D::exec() {

  // Custom initialization
  prepare();

  // check if derivative defined in derived class
  bool isDerivDefined = true;
  gsl_matrix *M = nullptr;
  try {
    const std::vector<double> inTest(m_parameterNames.size(), 1.0);
    std::vector<double> outTest(m_parameterNames.size());
    const double xValuesTest = 0;
    JacobianImpl J;
    M = gsl_matrix_alloc(m_parameterNames.size(), 1);
    J.setJ(M);
    // note nData set to zero (last argument) hence this should avoid further
    // memory problems
    functionDeriv(&(inTest.front()), &J, &xValuesTest, 0);
  } catch (Exception::NotImplementedError &) {
    isDerivDefined = false;
  }
  gsl_matrix_free(M);

  // Try to retrieve optional properties
  int histNumber = getProperty("WorkspaceIndex");
  const int maxInterations = getProperty("MaxIterations");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  // number of histogram is equal to the number of spectra
  const size_t numberOfSpectra = localworkspace->getNumberHistograms();
  // Check that the index given is valid
  if (histNumber >= static_cast<int>(numberOfSpectra)) {
    g_log.warning("Invalid Workspace index given, using first Workspace");
    histNumber = 0;
  }

  // Retrieve the spectrum into a vector
  const MantidVec &XValues = localworkspace->readX(histNumber);
  const MantidVec &YValues = localworkspace->readY(histNumber);
  const MantidVec &YErrors = localworkspace->readE(histNumber);

  // Read in the fitting range data that we were sent
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  // check if the values had been set, otherwise use defaults
  if (isEmpty(startX)) {
    startX = XValues.front();
    modifyStartOfRange(startX); // does nothing by default but derived class may
                                // provide a more intelligent value
  }
  if (isEmpty(endX)) {
    endX = XValues.back();
    modifyEndOfRange(endX); // does nothing by default but derived class may
                            // previde a more intelligent value
  }

  int m_minX;
  int m_maxX;

  // Check the validity of startX
  if (startX < XValues.front()) {
    g_log.warning("StartX out of range! Set to start of frame.");
    startX = XValues.front();
  }
  // Get the corresponding bin boundary that comes before (or coincides with)
  // this value
  for (m_minX = 0; XValues[m_minX + 1] < startX; ++m_minX) {
  }

  // Check the validity of endX and get the bin boundary that come after (or
  // coincides with) it
  if (endX >= XValues.back() || endX < startX) {
    g_log.warning("EndX out of range! Set to end of frame");
    endX = XValues.back();
    m_maxX = static_cast<int>(YValues.size());
  } else {
    for (m_maxX = m_minX; XValues[m_maxX] < endX; ++m_maxX) {
    }
  }

  afterDataRangedDetermined(m_minX, m_maxX);

  // create and populate GSL data container warn user if l_data.n < l_data.p
  // since as a rule of thumb this is required as a minimum to obtained
  // 'accurate'
  // fitting parameter values.

  FitData l_data(this, getProperty("Fix"));

  l_data.n = m_maxX - m_minX; // m_minX and m_maxX are array index markers. I.e. e.g. 0 & 19.
  if (l_data.n == 0) {
    g_log.error("The data set is empty.");
    throw std::runtime_error("The data set is empty.");
  }
  if (l_data.n < l_data.p) {
    g_log.error("Number of data points less than number of parameters to be fitted.");
    throw std::runtime_error("Number of data points less than number of parameters to be fitted.");
  }
  l_data.X = new double[l_data.n];
  l_data.sigmaData = new double[l_data.n];
  l_data.forSimplexLSwrap = new double[l_data.n];
  l_data.parameters = new double[nParams()];

  // check if histogram data in which case use mid points of histogram bins

  const bool isHistogram = localworkspace->isHistogramData();
  for (unsigned int i = 0; i < l_data.n; ++i) {
    if (isHistogram)
      l_data.X[i] = 0.5 * (XValues[m_minX + i] + XValues[m_minX + i + 1]); // take mid-point if histogram bin
    else
      l_data.X[i] = XValues[m_minX + i];
  }

  l_data.Y = &YValues[m_minX];

  // check that no error is negative or zero
  for (unsigned int i = 0; i < l_data.n; ++i) {
    if (YErrors[m_minX + i] <= 0.0) {
      l_data.sigmaData[i] = 1.0;
    } else
      l_data.sigmaData[i] = YErrors[m_minX + i];
  }

  // create array of fitted parameter. Take these to those input by the user.
  // However, for doing the
  // underlying fitting it might be more efficient to actually perform the
  // fitting on some of other
  // form of the fitted parameters. For instance, take the Gaussian sigma
  // parameter. In practice it
  // in fact more efficient to perform the fitting not on sigma but 1/sigma^2.
  // The methods
  // modifyInitialFittedParameters() and modifyFinalFittedParameters() are used
  // to allow for this;
  // by default these function do nothing.

  m_fittedParameter.clear();
  for (size_t i = 0; i < nParams(); i++) {
    m_fittedParameter.emplace_back(getProperty(m_parameterNames[i]));
  }
  modifyInitialFittedParameters(m_fittedParameter); // does nothing except if overwritten by derived class
  for (size_t i = 0; i < nParams(); i++) {
    l_data.parameters[i] = m_fittedParameter[i];
  }

  // set-up initial guess for fit parameters

  gsl_vector *initFuncArg;
  initFuncArg = gsl_vector_alloc(l_data.p);

  for (size_t i = 0, j = 0; i < nParams(); i++) {
    if (l_data.active[i])
      gsl_vector_set(initFuncArg, j++, m_fittedParameter[i]);
  }

  // set-up GSL container to be used with GSL simplex algorithm

  gsl_multimin_function gslSimplexContainer;
  gslSimplexContainer.n = l_data.p; // n here refers to number of parameters
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
  gsl_multifit_fdfsolver *s = nullptr;
  if (isDerivDefined) {
    s = gsl_multifit_fdfsolver_alloc(T, l_data.n, l_data.p);
    gsl_multifit_fdfsolver_set(s, &f, initFuncArg);
  }

  // set-up remaining GSL machinery to use simplex algorithm

  const gsl_multimin_fminimizer_type *simplexType = gsl_multimin_fminimizer_nmsimplex;
  gsl_multimin_fminimizer *simplexMinimizer = nullptr;
  gsl_vector *simplexStepSize = nullptr;
  if (!isDerivDefined) {
    simplexMinimizer = gsl_multimin_fminimizer_alloc(simplexType, l_data.p);
    simplexStepSize = gsl_vector_alloc(l_data.p);
    gsl_vector_set_all(simplexStepSize,
                       1.0); // is this always a sensible starting step size?
    gsl_multimin_fminimizer_set(simplexMinimizer, &gslSimplexContainer, initFuncArg, simplexStepSize);
  }

  // finally do the fitting

  int iter = 0;
  int status;
  double finalCostFuncVal;
  auto dof = static_cast<double>(l_data.n - l_data.p); // dof stands for degrees of freedom

  // Standard least-squares used if derivative function defined otherwise
  // simplex
  Progress prog(this, 0.0, 1.0, maxInterations);
  if (isDerivDefined) {

    do {
      iter++;
      status = gsl_multifit_fdfsolver_iterate(s);

      if (status) // break if error
        break;

      status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
      prog.report();
    } while (status == GSL_CONTINUE && iter < maxInterations);

    double chi = gsl_blas_dnrm2(s->f);
    finalCostFuncVal = chi * chi / dof;

    // put final converged fitting values back into m_fittedParameter
    for (size_t i = 0, j = 0; i < nParams(); i++)
      if (l_data.active[i])
        m_fittedParameter[i] = gsl_vector_get(s->x, j++);
  } else {
    do {
      iter++;
      status = gsl_multimin_fminimizer_iterate(simplexMinimizer);

      if (status) // break if error
        break;

      double size = gsl_multimin_fminimizer_size(simplexMinimizer);
      status = gsl_multimin_test_size(size, 1e-2);
      prog.report();
    } while (status == GSL_CONTINUE && iter < maxInterations);

    finalCostFuncVal = simplexMinimizer->fval / dof;

    // put final converged fitting values back into m_fittedParameter
    for (unsigned int i = 0, j = 0; i < m_fittedParameter.size(); i++)
      if (l_data.active[i])
        m_fittedParameter[i] = gsl_vector_get(simplexMinimizer->x, j++);
  }

  modifyFinalFittedParameters(m_fittedParameter); // do nothing except if overwritten by derived class

  // Output summary to log file

  std::string reportOfFit = gsl_strerror(status);

  g_log.information() << "Iteration = " << iter << "\n"
                      << "Status = " << reportOfFit << "\n"
                      << "Chi^2/DoF = " << finalCostFuncVal << "\n";
  for (size_t i = 0; i < m_fittedParameter.size(); i++)
    g_log.information() << m_parameterNames[i] << " = " << m_fittedParameter[i] << "  \n";

  // also output summary to properties

  setProperty("OutputStatus", reportOfFit);
  setProperty("OutputChi2overDoF", finalCostFuncVal);
  for (size_t i = 0; i < m_fittedParameter.size(); i++)
    setProperty(m_parameterNames[i], m_fittedParameter[i]);

  std::string output = getProperty("Output");
  if (!output.empty()) {
    // calculate covariance matrix if derivatives available

    gsl_matrix *covar(nullptr);
    std::vector<double> standardDeviations;
    std::vector<double> sdExtended;
    if (isDerivDefined) {
      covar = gsl_matrix_alloc(l_data.p, l_data.p);
#if GSL_MAJOR_VERSION < 2
      gsl_multifit_covar(s->J, 0.0, covar);
#else
      gsl_matrix *J = gsl_matrix_alloc(l_data.n, l_data.p);
      gsl_multifit_fdfsolver_jac(s, J);
      gsl_multifit_covar(J, 0.0, covar);
      gsl_matrix_free(J);
#endif

      int iPNotFixed = 0;
      for (size_t i = 0; i < nParams(); i++) {
        sdExtended.emplace_back(1.0);
        if (l_data.active[i]) {
          sdExtended[i] = sqrt(gsl_matrix_get(covar, iPNotFixed, iPNotFixed));
          iPNotFixed++;
        }
      }
      modifyFinalFittedParameters(sdExtended);
      for (size_t i = 0; i < nParams(); i++)
        if (l_data.active[i])
          standardDeviations.emplace_back(sdExtended[i]);

      declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputNormalisedCovarianceMatrix", "",
                                                                                Direction::Output),
                      "The name of the TableWorkspace in which to store the final "
                      "covariance matrix");
      setPropertyValue("OutputNormalisedCovarianceMatrix", output + "_NormalisedCovarianceMatrix");

      Mantid::API::ITableWorkspace_sptr m_covariance =
          Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      m_covariance->addColumn("str", "Name");
      std::vector<std::string> paramThatAreFitted; // used for populating 1st "name" column
      for (size_t i = 0; i < nParams(); i++) {
        if (l_data.active[i]) {
          m_covariance->addColumn("double", m_parameterNames[i]);
          paramThatAreFitted.emplace_back(m_parameterNames[i]);
        }
      }

      for (size_t i = 0; i < l_data.p; i++) {

        Mantid::API::TableRow row = m_covariance->appendRow();
        row << paramThatAreFitted[i];
        for (size_t j = 0; j < l_data.p; j++) {
          if (j == i)
            row << 1.0;
          else {
            row << 100.0 * gsl_matrix_get(covar, i, j) /
                       sqrt(gsl_matrix_get(covar, i, i) * gsl_matrix_get(covar, j, j));
          }
        }
      }

      setProperty("OutputNormalisedCovarianceMatrix", m_covariance);
    }

    declareProperty(
        std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputParameters", "", Direction::Output),
        "The name of the TableWorkspace in which to store the "
        "final fit parameters");
    declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                    "Name of the output Workspace holding resulting simlated spectrum");

    setPropertyValue("OutputParameters", output + "_Parameters");
    setPropertyValue("OutputWorkspace", output + "_Workspace");

    // Save the final fit parameters in the output table workspace
    Mantid::API::ITableWorkspace_sptr m_result =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_result->addColumn("str", "Name");
    m_result->addColumn("double", "Value");
    if (isDerivDefined)
      m_result->addColumn("double", "Error");
    Mantid::API::TableRow firstRow = m_result->appendRow();
    firstRow << "Chi^2/DoF" << finalCostFuncVal;

    for (size_t i = 0; i < nParams(); i++) {
      Mantid::API::TableRow row = m_result->appendRow();
      row << m_parameterNames[i] << m_fittedParameter[i];
      if (isDerivDefined && l_data.active[i]) {
        // perhaps want to scale standard deviations with sqrt(finalCostFuncVal)
        row << sdExtended[i];
      }
    }
    setProperty("OutputParameters", m_result);

    // Save the fitted and simulated spectra in the output workspace
    MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
    int iSpec = getProperty("WorkspaceIndex");
    const MantidVec &inputX = inputWorkspace->readX(iSpec);
    const MantidVec &inputY = inputWorkspace->readY(iSpec);

    int histN = isHistogram ? 1 : 0;
    Mantid::DataObjects::Workspace2D_sptr ws = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
        Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 3, l_data.n + histN, l_data.n));
    ws->setTitle("");
    ws->getAxis(0)->unit() = inputWorkspace->getAxis(0)->unit(); //    UnitFactory::Instance().create("TOF");

    for (int i = 0; i < 3; i++)
      ws->dataX(i).assign(inputX.begin() + m_minX, inputX.begin() + m_maxX + histN);

    ws->dataY(0).assign(inputY.begin() + m_minX, inputY.begin() + m_maxX);

    MantidVec &Y = ws->dataY(1);
    MantidVec &E = ws->dataY(2);

    auto lOut = new double[l_data.n];                 // to capture output from call to function()
    modifyInitialFittedParameters(m_fittedParameter); // does nothing except if
                                                      // overwritten by derived
                                                      // class
    function(&m_fittedParameter[0], lOut, l_data.X, l_data.n);
    modifyInitialFittedParameters(m_fittedParameter); // reverse the effect of
    // modifyInitialFittedParameters - if any

    for (unsigned int i = 0; i < l_data.n; i++) {
      Y[i] = lOut[i];
      E[i] = l_data.Y[i] - Y[i];
    }

    delete[] lOut;

    setProperty("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(ws));

    if (isDerivDefined)
      gsl_matrix_free(covar);
  }

  // clean up dynamically allocated gsl stuff

  if (isDerivDefined)
    gsl_multifit_fdfsolver_free(s);
  else {
    gsl_vector_free(simplexStepSize);
    gsl_multimin_fminimizer_free(simplexMinimizer);
  }

  delete[] l_data.X;
  delete[] l_data.sigmaData;
  delete[] l_data.forSimplexLSwrap;
  delete[] l_data.parameters;
  gsl_vector_free(initFuncArg);
}

/**  Constructor.
 *   @param fit :: A pointer to the Fit1D class
 *   @param fixed :: A list of comma separated names of the fixed parameters.
 */
FitData::FitData(Fit1D *fit, const std::string &fixed)
    : n(0), X(nullptr), Y(nullptr), sigmaData(nullptr), fit1D(fit), forSimplexLSwrap(nullptr), parameters(nullptr) {
  Mantid::Kernel::StringTokenizer namesStrTok(fixed, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  active.insert(active.begin(), fit1D->m_parameterNames.size(), true);
  for (const auto &name : namesStrTok) {
    std::vector<std::string>::const_iterator i =
        std::find(fit1D->m_parameterNames.begin(), fit1D->m_parameterNames.end(), name);
    if (i != fit1D->m_parameterNames.end()) {
      active[i - fit1D->m_parameterNames.begin()] = false;
    } else
      throw std::invalid_argument("Attempt to fix non-existing parameter " + name);
  }
  p = 0;
  for (int i = 0; i < int(active.size()); i++)
    if (active[i])
      J.m_map[i] = static_cast<int>(p++);
    else
      J.m_map[i] = -1;
}

} // namespace Mantid::CurveFitting::Algorithms
