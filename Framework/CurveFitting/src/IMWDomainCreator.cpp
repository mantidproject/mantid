// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IMWDomainCreator.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/ParameterEstimator.h"
#include "MantidCurveFitting/SeqDomain.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Matrix.h"

#include <algorithm>

namespace Mantid {
namespace CurveFitting {

namespace {
/**
 * A simple implementation of Jacobian.
 */
class SimpleJacobian : public API::Jacobian {
public:
  /// Constructor
  SimpleJacobian(size_t nData, size_t nParams)
      : m_nParams(nParams), m_data(nData * nParams) {}
  /// Setter
  void set(size_t iY, size_t iP, double value) override {
    m_data[iY * m_nParams + iP] = value;
  }
  /// Getter
  double get(size_t iY, size_t iP) override {
    return m_data[iY * m_nParams + iP];
  }
  /// Zero
  void zero() override { m_data.assign(m_data.size(), 0.0); }

private:
  size_t m_nParams;           ///< number of parameters / second dimension
  std::vector<double> m_data; ///< data storage
};

bool greaterIsLess(double x1, double x2) { return x1 > x2; }
} // namespace

using namespace Kernel;
using API::MatrixWorkspace;
using API::Workspace;

/**
 * Constructor.
 * @param fit :: Property manager with properties defining the domain to be
 * created
 * @param workspacePropertyName :: Name of the workspace property.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
IMWDomainCreator::IMWDomainCreator(Kernel::IPropertyManager *fit,
                                   const std::string &workspacePropertyName,
                                   IMWDomainCreator::DomainType domainType)
    : API::IDomainCreator(
          fit, std::vector<std::string>(1, workspacePropertyName), domainType),
      m_workspaceIndex(-1), m_startX(EMPTY_DBL()), m_endX(EMPTY_DBL()),
      m_startIndex(0) {
  if (m_workspacePropertyNames.empty()) {
    throw std::runtime_error("Cannot create FitMW: no workspace given");
  }
  m_workspacePropertyName = m_workspacePropertyNames[0];
}

/**
 * Set all parameters
 */
void IMWDomainCreator::setParameters() const {
  // if property manager is set overwrite any set parameters
  if (m_manager) {

    // get the workspace
    API::Workspace_sptr ws = m_manager->getProperty(m_workspacePropertyName);
    m_matrixWorkspace = boost::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    if (!m_matrixWorkspace) {
      throw std::invalid_argument("InputWorkspace must be a MatrixWorkspace.");
    }

    int index = m_manager->getProperty(m_workspaceIndexPropertyName);
    m_workspaceIndex = static_cast<size_t>(index);
    m_startX = m_manager->getProperty(m_startXPropertyName);
    m_endX = m_manager->getProperty(m_endXPropertyName);
  }
}

/**
 * Declare properties that specify the dataset within the workspace to fit to.
 * @param suffix :: names the dataset
 * @param addProp :: allows for the declaration of certain properties of the
 * dataset
 */
void IMWDomainCreator::declareDatasetProperties(const std::string &suffix,
                                                bool addProp) {
  m_workspaceIndexPropertyName = "WorkspaceIndex" + suffix;
  m_startXPropertyName = "StartX" + suffix;
  m_endXPropertyName = "EndX" + suffix;

  if (addProp && !m_manager->existsProperty(m_workspaceIndexPropertyName)) {
    auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
    mustBePositive->setLower(0);
    declareProperty(new PropertyWithValue<int>(m_workspaceIndexPropertyName, 0,
                                               mustBePositive),
                    "The Workspace Index to fit in the input workspace");
    declareProperty(
        new PropertyWithValue<double>(m_startXPropertyName, EMPTY_DBL()),
        "A value of x in, or on the low x boundary of, the first bin to "
        "include in\n"
        "the fit (default lowest value of x)");
    declareProperty(
        new PropertyWithValue<double>(m_endXPropertyName, EMPTY_DBL()),
        "A value in, or on the high x boundary of, the last bin the fitting "
        "range\n"
        "(default the highest value of x)");
  }
}

/**
 * Calculate size and starting iterator in the X array
 * @returns :: A pair of start iterator and size of the data.
 */
std::pair<size_t, size_t> IMWDomainCreator::getXInterval() const {

  const auto &X = m_matrixWorkspace->x(m_workspaceIndex);
  if (X.empty()) {
    throw std::runtime_error("Workspace contains no data.");
  }

  setParameters();

  // From points to the first occurrence of StartX in the workspace interval.
  // End points to the last occurrence of EndX in the workspace interval.
  // Find the fitting interval: from -> to
  Mantid::MantidVec::const_iterator from;
  Mantid::MantidVec::const_iterator to;

  bool isXAscending = X.front() < X.back();

  if (m_startX == EMPTY_DBL() && m_endX == EMPTY_DBL()) {
    m_startX = X.front();
    from = X.begin();
    m_endX = X.back();
    to = X.end();
  } else if (m_startX == EMPTY_DBL() || m_endX == EMPTY_DBL()) {
    throw std::invalid_argument(
        "Both StartX and EndX must be given to set fitting interval.");
  } else if (isXAscending) {
    if (m_startX > m_endX) {
      std::swap(m_startX, m_endX);
    }
    from = std::lower_bound(X.begin(), X.end(), m_startX);
    to = std::upper_bound(from, X.end(), m_endX);
  } else { // x is descending
    if (m_startX < m_endX) {
      std::swap(m_startX, m_endX);
    }
    from = std::lower_bound(X.begin(), X.end(), m_startX, greaterIsLess);
    to = std::upper_bound(from, X.end(), m_endX, greaterIsLess);
  }

  // Check whether the fitting interval defined by StartX and EndX is 0.
  // This occurs when StartX and EndX are both less than the minimum workspace
  // x-value or greater than the maximum workspace x-value.
  if (to - from == 0) {
    throw std::invalid_argument("StartX and EndX values do not capture a range "
                                "within the workspace interval.");
  }

  if (m_matrixWorkspace->isHistogramData()) {
    if (X.end() == to) {
      to = X.end() - 1;
    }
  }
  return std::make_pair(std::distance(X.begin(), from),
                        std::distance(X.begin(), to));
}

/**
 * Return the size of the domain to be created.
 */
size_t IMWDomainCreator::getDomainSize() const {
  setParameters();
  size_t startIndex, endIndex;
  std::tie(startIndex, endIndex) = getXInterval();
  return endIndex - startIndex;
}

/**
 * Initialize the function with the workspace.
 * @param function :: Function to initialize.
 */
void IMWDomainCreator::initFunction(API::IFunction_sptr function) {
  setParameters();
  if (!function) {
    throw std::runtime_error("Cannot initialize empty function.");
  }
  function->setWorkspace(m_matrixWorkspace);
  function->setMatrixWorkspace(m_matrixWorkspace, m_workspaceIndex, m_startX,
                               m_endX);
  setInitialValues(*function);
}

/**
 * Creates a workspace to hold the results. If the input workspace contains
 * histogram data then so will this
 * It assigns the X and input data values but no Y,E data for any functions
 * @param nhistograms The number of histograms
 * @param nyvalues The number of y values to hold
 */
API::MatrixWorkspace_sptr
IMWDomainCreator::createEmptyResultWS(const size_t nhistograms,
                                      const size_t nyvalues) {
  size_t nxvalues(nyvalues);
  if (m_matrixWorkspace->isHistogramData())
    nxvalues += 1;
  API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
      "Workspace2D", nhistograms, nxvalues, nyvalues);
  ws->setTitle("");
  ws->setYUnitLabel(m_matrixWorkspace->YUnitLabel());
  ws->setYUnit(m_matrixWorkspace->YUnit());
  ws->getAxis(0)->unit() = m_matrixWorkspace->getAxis(0)->unit();
  auto tAxis = std::make_unique<API::TextAxis>(nhistograms);
  ws->replaceAxis(1, std::move(tAxis));

  auto &inputX = m_matrixWorkspace->x(m_workspaceIndex);
  auto &inputY = m_matrixWorkspace->y(m_workspaceIndex);
  auto &inputE = m_matrixWorkspace->e(m_workspaceIndex);
  // X values for all
  for (size_t i = 0; i < nhistograms; i++) {
    ws->mutableX(i).assign(inputX.begin() + m_startIndex,
                           inputX.begin() + m_startIndex + nxvalues);
  }
  // Data values for the first histogram
  ws->mutableY(0).assign(inputY.begin() + m_startIndex,
                         inputY.begin() + m_startIndex + nyvalues);
  ws->mutableE(0).assign(inputE.begin() + m_startIndex,
                         inputE.begin() + m_startIndex + nyvalues);

  return ws;
}

/**
 * Set initial values for parameters with default values.
 * @param function : A function to set parameters for.
 */
void IMWDomainCreator::setInitialValues(API::IFunction &function) {
  auto domain = m_domain.lock();
  auto values = m_values.lock();
  if (domain && values) {
    ParameterEstimator::estimate(function, *domain, *values);
  }
}

/**
 * Create an output workspace with the calculated values.
 * @param baseName :: Specifies the name of the output workspace
 * @param function :: A Pointer to the fitting function
 * @param domain :: The domain containing x-values for the function
 * @param values :: A API::FunctionValues instance containing the fitting data
 * @param outputWorkspacePropertyName :: The property name
 */
boost::shared_ptr<API::Workspace> IMWDomainCreator::createOutputWorkspace(
    const std::string &baseName, API::IFunction_sptr function,
    boost::shared_ptr<API::FunctionDomain> domain,
    boost::shared_ptr<API::FunctionValues> values,
    const std::string &outputWorkspacePropertyName) {
  if (!values) {
    throw std::logic_error("FunctionValues expected");
  }

  // Compile list of functions to output. The top-level one is first
  std::list<API::IFunction_sptr> functionsToDisplay(1, function);
  if (m_outputCompositeMembers) {
    appendCompositeFunctionMembers(functionsToDisplay, function);
  }

  // Nhist = Data histogram, Difference Histogram + nfunctions
  const size_t nhistograms = functionsToDisplay.size() + 2;
  const size_t nyvalues = values->size();
  auto ws = createEmptyResultWS(nhistograms, nyvalues);
  // The workspace was constructed with a TextAxis
  API::TextAxis *textAxis = static_cast<API::TextAxis *>(ws->getAxis(1));
  textAxis->setLabel(0, "Data");
  textAxis->setLabel(1, "Calc");
  textAxis->setLabel(2, "Diff");

  // Add each calculated function
  auto iend = functionsToDisplay.end();
  size_t wsIndex(1); // Zero reserved for data
  for (auto it = functionsToDisplay.begin(); it != iend; ++it) {
    if (wsIndex > 2)
      textAxis->setLabel(wsIndex, (*it)->name());
    addFunctionValuesToWS(*it, ws, wsIndex, domain, values);
    if (it == functionsToDisplay.begin())
      wsIndex += 2; // Skip difference histogram for now
    else
      ++wsIndex;
  }

  // Set the difference spectrum
  auto &Ycal = ws->mutableY(1);
  auto &Diff = ws->mutableY(2);
  const size_t nData = values->size();
  for (size_t i = 0; i < nData; ++i) {
    if (values->getFitWeight(i) != 0.0) {
      Diff[i] = values->getFitData(i) - Ycal[i];
    } else {
      Diff[i] = 0.0;
    }
  }

  if (!outputWorkspacePropertyName.empty()) {
    declareProperty(
        new API::WorkspaceProperty<MatrixWorkspace>(outputWorkspacePropertyName,
                                                    "", Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
    m_manager->setPropertyValue(outputWorkspacePropertyName,
                                baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, ws);
  }

  // If the input is a not an EventWorkspace and is a distrubution, then convert
  // the output also to a distribution
  if (!boost::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(
          m_matrixWorkspace)) {
    if (m_matrixWorkspace->isDistribution()) {
      ws->setDistribution(true);
    }
  }

  return ws;
}

/**
 * @param functionList The current list of functions to append to
 * @param function A function that may or not be composite
 */
void IMWDomainCreator::appendCompositeFunctionMembers(
    std::list<API::IFunction_sptr> &functionList,
    const API::IFunction_sptr &function) const {
  // if function is a Convolution then output of convolved model's mebers may be
  // required
  if (m_convolutionCompositeMembers &&
      boost::dynamic_pointer_cast<Functions::Convolution>(function)) {
    appendConvolvedCompositeFunctionMembers(functionList, function);
  } else {
    const auto compositeFn =
        boost::dynamic_pointer_cast<API::CompositeFunction>(function);
    if (!compositeFn)
      return;

    const size_t nlocals = compositeFn->nFunctions();
    for (size_t i = 0; i < nlocals; ++i) {
      auto localFunction = compositeFn->getFunction(i);
      auto localComposite =
          boost::dynamic_pointer_cast<API::CompositeFunction>(localFunction);
      if (localComposite)
        appendCompositeFunctionMembers(functionList, localComposite);
      else
        functionList.insert(functionList.end(), localFunction);
    }
  }
}

/**
 * If the fit function is Convolution and flag m_convolutionCompositeMembers is
 * set and Convolution's
 * second function (the model) is composite then use members of the model for
 * the output.
 * @param functionList :: A list of Convolutions constructed from the
 * resolution of the fitting function (index 0)
 *   and members of the model.
 * @param function A Convolution function which model may or may not be a
 * composite function.
 * @return True if all conditions are fulfilled and it is possible to produce
 * the output.
 */
void IMWDomainCreator::appendConvolvedCompositeFunctionMembers(
    std::list<API::IFunction_sptr> &functionList,
    const API::IFunction_sptr &function) const {
  boost::shared_ptr<Functions::Convolution> convolution =
      boost::dynamic_pointer_cast<Functions::Convolution>(function);

  const auto compositeFn = boost::dynamic_pointer_cast<API::CompositeFunction>(
      convolution->getFunction(1));
  if (!compositeFn) {
    functionList.insert(functionList.end(), convolution);
  } else {
    auto resolution = convolution->getFunction(0);
    const size_t nlocals = compositeFn->nFunctions();
    for (size_t i = 0; i < nlocals; ++i) {
      auto localFunction = compositeFn->getFunction(i);
      boost::shared_ptr<Functions::Convolution> localConvolution =
          boost::make_shared<Functions::Convolution>();
      localConvolution->addFunction(resolution);
      localConvolution->addFunction(localFunction);
      functionList.insert(functionList.end(), localConvolution);
    }
  }
}

/**
 * Add the calculated function values to the workspace. Estimate an error for
 * each calculated value.
 * @param function The function to evaluate
 * @param ws A workspace to fill
 * @param wsIndex The index to store the values
 * @param domain The domain to calculate the values over
 * @param resultValues A presized values holder for the results
 */
void IMWDomainCreator::addFunctionValuesToWS(
    const API::IFunction_sptr &function,
    boost::shared_ptr<API::MatrixWorkspace> &ws, const size_t wsIndex,
    const boost::shared_ptr<API::FunctionDomain> &domain,
    boost::shared_ptr<API::FunctionValues> resultValues) const {
  const size_t nData = resultValues->size();
  resultValues->zeroCalculated();

  // Function value
  function->function(*domain, *resultValues);

  size_t nParams = function->nParams();

  // the function should contain the parameter's covariance matrix
  auto covar = function->getCovarianceMatrix();
  bool hasErrors = false;
  if (!covar) {
    for (size_t j = 0; j < nParams; ++j) {
      if (function->getError(j) != 0.0) {
        hasErrors = true;
        break;
      }
    }
  }

  if (covar || hasErrors) {
    // and errors
    SimpleJacobian J(nData, nParams);
    try {
      function->functionDeriv(*domain, J);
    } catch (...) {
      function->calNumericalDeriv(*domain, J);
    }
    if (covar) {
      // if the function has a covariance matrix attached - use it for the
      // errors
      const Kernel::Matrix<double> &C = *covar;
      // The formula is E = J * C * J^T
      // We don't do full 3-matrix multiplication because we only need the
      // diagonals of E
      std::vector<double> E(nData);
      for (size_t k = 0; k < nData; ++k) {
        double s = 0.0;
        for (size_t i = 0; i < nParams; ++i) {
          double tmp = J.get(k, i);
          s += C[i][i] * tmp * tmp;
          for (size_t j = i + 1; j < nParams; ++j) {
            s += J.get(k, i) * C[i][j] * J.get(k, j) * 2;
          }
        }
        E[k] = s;
      }

      double chi2 = function->getChiSquared();
      auto &yValues = ws->mutableY(wsIndex);
      auto &eValues = ws->mutableE(wsIndex);
      for (size_t i = 0; i < nData; i++) {
        yValues[i] = resultValues->getCalculated(i);
        eValues[i] = std::sqrt(E[i] * chi2);
      }

    } else {
      // otherwise use the parameter errors which is OK for uncorrelated
      // parameters
      auto &yValues = ws->mutableY(wsIndex);
      auto &eValues = ws->mutableE(wsIndex);
      for (size_t i = 0; i < nData; i++) {
        yValues[i] = resultValues->getCalculated(i);
        double err = 0.0;
        for (size_t j = 0; j < nParams; ++j) {
          double d = J.get(i, j) * function->getError(j);
          err += d * d;
        }
        eValues[i] = std::sqrt(err);
      }
    }
  } else {
    // No errors
    auto &yValues = ws->mutableY(wsIndex);
    for (size_t i = 0; i < nData; i++) {
      yValues[i] = resultValues->getCalculated(i);
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
