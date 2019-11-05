// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
//----------------------------------------------------------------------

#include "MantidCurveFitting/TableWorkspaceDomainCreator.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidCurveFitting/ExcludeRangeFinder.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidCurveFitting/ParameterEstimator.h"
#include "MantidCurveFitting/SeqDomain.h"

#include "MantidDataObjects/TableColumn.h"

#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"

namespace Mantid {
namespace CurveFitting {

namespace {

/// Helper struct for helping with joining exclusion ranges.
/// Edge points of the ranges can be wrapped in this struct
/// and sorted together without loosing their function.
struct RangePoint {
  enum Kind : char { Opening, Closing };
  /// The kind of the point: either opening or closing the range.
  Kind kind;
  /// The value of the point.
  double value;
  /// Comparison of two points.
  /// @param point :: Another point to compare with.
  bool operator<(const RangePoint &point) const {
    if (this->value == point.value) {
      // If an Opening and Closing points have the same value
      // the Opening one should go first (be "smaller").
      // This way the procedure of joinOverlappingRanges will join
      // the ranges that meet at these points.
      return this->kind == Opening;
    }
    return this->value < point.value;
  }
};

/// Find any overlapping ranges in a vector and join them.
/// @param[in,out] exclude :: A vector with the ranges some of which may
/// overlap. On output all overlapping ranges are joined and the vector contains
/// increasing series of doubles (an even number of them).
void joinOverlappingRanges(std::vector<double> &exclude) {
  if (exclude.empty()) {
    return;
  }
  // The situation here is similar to matching brackets in an expression.
  // If we sort all the points in the input vector remembering their kind
  // then a separate exclusion region starts with the first openning bracket
  // and ends with the matching closing bracket. All brackets (points) inside
  // them can be dropped.

  // Wrap the points into helper struct RangePoint
  std::vector<RangePoint> points;
  points.reserve(exclude.size());
  for (auto point = exclude.begin(); point != exclude.end(); point += 2) {
    points.push_back(RangePoint{RangePoint::Opening, *point});
    points.push_back(RangePoint{RangePoint::Closing, *(point + 1)});
  }
  // Sort the points according to the operator defined in RangePoint.
  std::sort(points.begin(), points.end());

  // Clear the argument vector.
  exclude.clear();
  // Start the level counter which shows the number of unmatched openning
  // brackets.
  size_t level(0);
  for (auto &point : points) {
    if (point.kind == RangePoint::Opening) {
      if (level == 0) {
        // First openning bracket starts a new exclusion range.
        exclude.push_back(point.value);
      }
      // Each openning bracket increases the level
      ++level;
    } else {
      if (level == 1) {
        // The bracket that makes level 0 is an end of a range.
        exclude.push_back(point.value);
      }
      // Each closing bracket decreases the level
      --level;
    }
  }
}

bool greaterIsLess(double x1, double x2) { return x1 > x2; }
} // namespace

using namespace Kernel;

/**
 * Constructor.
 * @param fit :: Property manager with properties defining the domain to be
 * created
 * @param workspacePropertyName :: Name of the workspace property.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
TableWorkspaceDomainCreator::TableWorkspaceDomainCreator(
    Kernel::IPropertyManager *fit, const std::string &workspacePropertyName,
    TableWorkspaceDomainCreator::DomainType domainType)
    : IDomainCreator(fit, std::vector<std::string>(1, workspacePropertyName),
                     domainType),
      m_startX(EMPTY_DBL()), m_endX(EMPTY_DBL()), m_maxSize(0),
      m_noErrCol(false) {
  if (m_workspacePropertyNames.empty()) {
    throw std::runtime_error("Cannot create FitMW: no workspace given");
  }
  m_workspacePropertyName = m_workspacePropertyNames[0];
}

/**
 * Constructor. Methods setWorkspace and setRange must becalled
 * set up the creator.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
TableWorkspaceDomainCreator::TableWorkspaceDomainCreator(
    TableWorkspaceDomainCreator::DomainType domainType)
    : IDomainCreator(nullptr, std::vector<std::string>(), domainType),
      m_startX(EMPTY_DBL()), m_endX(EMPTY_DBL()), m_maxSize(10),
      m_noErrCol(false) {}

/**
 * Declare properties that specify the dataset within the workspace to fit to.
 * @param suffix :: names the dataset
 * @param addProp :: allows for the declaration of certain properties of the
 * dataset
 */
void TableWorkspaceDomainCreator::declareDatasetProperties(
    const std::string &suffix, bool addProp) {

  m_startXPropertyName = "StartX" + suffix;
  m_endXPropertyName = "EndX" + suffix;
  m_maxSizePropertyName = "MaxSize" + suffix;
  m_excludePropertyName = "Exclude" + suffix;
  m_xColumnPropertyName = "XColumn" + suffix;
  m_yColumnPropertyName = "YColumn" + suffix;
  m_errorColumnPropertyName = "ErrColumn" + suffix;

  if (addProp) {
    auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
    mustBePositive->setLower(0);
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
    if (m_domainType != Simple &&
        !m_manager->existsProperty(m_maxSizePropertyName)) {
      auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
      mustBePositive->setLower(0);
      declareProperty(
          new PropertyWithValue<int>(m_maxSizePropertyName, 1, mustBePositive),
          "The maximum number of values per a simple domain.");
    }
    if (!m_manager->existsProperty(m_excludePropertyName)) {
      auto mustBeOrderedPairs =
          boost::make_shared<ArrayOrderedPairsValidator<double>>();
      declareProperty(
          new ArrayProperty<double>(m_excludePropertyName, mustBeOrderedPairs),
          "A list of pairs of doubles that specify ranges that must be "
          "excluded from fit.");
    }
    declareProperty(
        new PropertyWithValue<std::string>(m_xColumnPropertyName, ""),
        "The name of the X column.");
    declareProperty(
        new PropertyWithValue<std::string>(m_yColumnPropertyName, ""),
        "The name of the Y column.");
    declareProperty(
        new PropertyWithValue<std::string>(m_errorColumnPropertyName, ""),
        "The name of the error column.");
  }
}

/// Create a domain from the input workspace
void TableWorkspaceDomainCreator::createDomain(
    boost::shared_ptr<API::FunctionDomain> &domain,
    boost::shared_ptr<API::FunctionValues> &values, size_t i0) {

  setParameters();

  auto rowCount = m_tableWorkspace->rowCount();
  if (rowCount == 0) {
    throw std::runtime_error("Workspace contains no data.");
  }

  auto X = m_tableWorkspace->getColumn(m_xColName);
  std::vector<double> xData;
  xData.reserve(m_tableWorkspace->rowCount());
  for (size_t i = 0; i < m_tableWorkspace->rowCount(); ++i) {
    xData.push_back(X->toDouble(i));
  }

  // find the fitting interval: from -> to
  size_t endRowNo = 0;
  std::tie(m_startRowNo, endRowNo) = getXInterval(xData);
  auto from = xData.begin() + m_startRowNo;
  auto to = xData.begin() + endRowNo;
  auto n = endRowNo - m_startRowNo;

  if (m_domainType != Simple) {
    if (m_maxSize < n) {
      SeqDomain *seqDomain = SeqDomain::create(m_domainType);
      domain.reset(seqDomain);
      size_t m = 0;
      while (m < n) {
        // create a simple creator
        auto creator = new TableWorkspaceDomainCreator;
        creator->setWorkspace(m_tableWorkspace);
        creator->setColumnNames(m_xColName, m_yColName, m_errColName);
        size_t k = m + m_maxSize;
        if (k > n)
          k = n;
        creator->setRange(*(from + m), *(from + k - 1));
        seqDomain->addCreator(API::IDomainCreator_sptr(creator));
        m = k;
      }
      values.reset();
      return;
    }
    // else continue with simple domain
  }

  // set function domain
  domain.reset(new API::FunctionDomain1DVector(from, to));

  if (!values) {
    values.reset(new API::FunctionValues(*domain));
  } else {
    values->expand(i0 + domain->size());
  }

  // set the data to fit to
  assert(n == domain->size());
  auto Y = m_tableWorkspace->getColumn(m_yColName);
  if (endRowNo > Y->size()) {
    throw std::runtime_error(
        "TableWorkspaceDomainCreator: Inconsistent TableWorkspace");
  }

  // Helps find points excluded form fit.
  ExcludeRangeFinder excludeFinder(m_exclude, xData.front(), xData.back());

  auto errors = m_tableWorkspace->getColumn(m_errColName);
  for (size_t i = m_startRowNo; i < endRowNo; ++i) {
    size_t j = i - m_startRowNo + i0;
    auto y = Y->toDouble(i);
    auto error = errors->toDouble(i);
    double weight = 0.0;

    if (excludeFinder.isExcluded(xData[i])) {
      weight = 0.0;
    } else if (!std::isfinite(y)) {
      // nan or inf data
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
      y = 0.0; // leaving inf or nan would break the fit
    } else if (m_noErrCol) {
      weight = 1.0;
    } else if (!std::isfinite(error)) {
      // nan or inf error
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
    } else if (error <= 0) {
      if (!m_ignoreInvalidData)
        weight = 1.0;
    } else {
      weight = 1.0 / error;
      if (!std::isfinite(weight)) {
        if (!m_ignoreInvalidData)
          throw std::runtime_error(
              "Error of a data point is probably too small.");
        weight = 0.0;
      }
    }

    values->setFitData(j, y);
    values->setFitWeight(j, weight);
  }
  m_domain = boost::dynamic_pointer_cast<API::FunctionDomain1D>(domain);
  m_values = values;
}

/**
 * Create an output workspace with the calculated values.
 * @param baseName :: Specifies the name of the output workspace
 * @param function :: A Pointer to the fitting function
 * @param domain :: The domain containing x-values for the function
 * @param values :: A API::FunctionValues instance containing the fitting data
 * @param outputWorkspacePropertyName :: The property name
 */
boost::shared_ptr<API::Workspace>
TableWorkspaceDomainCreator::createOutputWorkspace(
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
        new API::WorkspaceProperty<API::MatrixWorkspace>(
            outputWorkspacePropertyName, "", Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
    m_manager->setPropertyValue(outputWorkspacePropertyName,
                                baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, ws);
  }

  return ws;
}

/**
 * @param functionList The current list of functions to append to
 * @param function A function that may or not be composite
 */
void TableWorkspaceDomainCreator::appendCompositeFunctionMembers(
    std::list<API::IFunction_sptr> &functionList,
    const API::IFunction_sptr &function) const {
  // if function is a Convolution then output of convolved model's members may
  // be required
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
void TableWorkspaceDomainCreator::appendConvolvedCompositeFunctionMembers(
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
void TableWorkspaceDomainCreator::addFunctionValuesToWS(
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
    Jacobian J(nData, nParams);
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

/**
 * Creates a workspace to hold the results. If the input workspace contains
 * histogram data then so will this
 * It assigns the X and input data values but no Y,E data for any functions
 * @param nhistograms The number of histograms
 * @param nyvalues The number of y values to hold
 */
API::MatrixWorkspace_sptr
TableWorkspaceDomainCreator::createEmptyResultWS(const size_t nhistograms,
                                                 const size_t nyvalues) {
  size_t nxvalues(nyvalues);
  API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
      "Workspace2D", nhistograms, nxvalues, nyvalues);
  ws->setTitle(m_tableWorkspace->getTitle());
  auto tAxis = std::make_unique<API::TextAxis>(nhistograms);
  ws->replaceAxis(1, std::move(tAxis));

  auto inputX = m_tableWorkspace->getColumn(m_xColName);
  auto inputY = m_tableWorkspace->getColumn(m_yColName);
  auto inputE = m_tableWorkspace->getColumn(m_errColName);

  std::vector<double> xData;
  std::vector<double> yData;
  std::vector<double> eData;
  xData.reserve(m_tableWorkspace->rowCount());
  yData.reserve(m_tableWorkspace->rowCount());
  eData.reserve(m_tableWorkspace->rowCount());

  for (size_t i = 0; i < m_tableWorkspace->rowCount(); ++i) {
    xData.push_back(inputX->toDouble(i));
    yData.push_back(inputY->toDouble(i));
    eData.push_back(inputE->toDouble(i));
  }

  // X values for all
  for (size_t i = 0; i < nhistograms; i++) {
    ws->mutableX(i).assign(xData.begin() + m_startRowNo,
                           xData.begin() + m_startRowNo + nxvalues);
  }
  // Data values for the first histogram
  ws->mutableY(0).assign(yData.begin() + m_startRowNo,
                         yData.begin() + m_startRowNo + nyvalues);
  ws->mutableE(0).assign(eData.begin() + m_startRowNo,
                         eData.begin() + m_startRowNo + nyvalues);

  return ws;
}

/**
 * Return the size of the domain to be created.
 */
size_t TableWorkspaceDomainCreator::getDomainSize() const {
  setParameters();
  auto X = m_tableWorkspace->getColumn(m_xColName);
  std::vector<double> xData;
  xData.reserve(m_tableWorkspace->rowCount());
  for (size_t i = 0; i < m_tableWorkspace->rowCount(); ++i) {
    xData.push_back(X->toDouble(i));
  }
  size_t startIndex, endIndex;
  std::tie(startIndex, endIndex) = getXInterval(xData);
  return endIndex - startIndex;
}

/**
 * Initialize the function with the workspace.
 * @param function :: Function to initialize.
 */
void TableWorkspaceDomainCreator::initFunction(API::IFunction_sptr function) {
  setParameters();
  if (!function) {
    throw std::runtime_error("Cannot initialize empty function.");
  }
  function->setWorkspace(m_tableWorkspace);
  setInitialValues(*function);
}

/**
 * Set initial values for parameters with default values.
 * @param function : A function to set parameters for.
 */
void TableWorkspaceDomainCreator::setInitialValues(API::IFunction &function) {
  auto domain = m_domain.lock();
  auto values = m_values.lock();
  if (domain && values) {
    ParameterEstimator::estimate(function, *domain, *values);
  }
}

/**
 * Calculate size and starting iterator in the X array
 * @returns :: A pair of start iterator and size of the data.
 */
std::pair<size_t, size_t>
TableWorkspaceDomainCreator::getXInterval(std::vector<double> xData) const {
  const auto sizeOfData = xData.size();
  if (sizeOfData == 0) {
    throw std::runtime_error("Workspace contains no data.");
  }

  setParameters();

  // From points to the first occurrence of StartX in the workspace interval.
  // End points to the last occurrence of EndX in the workspace interval.
  // Find the fitting interval: from -> to
  Mantid::MantidVec::iterator from;
  Mantid::MantidVec::iterator to;

  bool isXAscending = xData.front() < xData.back();

  if (m_startX == EMPTY_DBL() && m_endX == EMPTY_DBL()) {
    m_startX = xData.front();
    from = xData.begin();
    m_endX = xData.back();
    to = xData.end();
  } else if (m_startX == EMPTY_DBL() || m_endX == EMPTY_DBL()) {
    throw std::invalid_argument(
        "Both StartX and EndX must be given to set fitting interval.");
  } else if (isXAscending) {
    if (m_startX > m_endX) {
      std::swap(m_startX, m_endX);
    }
    from = std::lower_bound(xData.begin(), xData.end(), m_startX);
    to = std::upper_bound(from, xData.end(), m_endX);
  } else { // x is descending
    if (m_startX < m_endX) {
      std::swap(m_startX, m_endX);
    }
    from =
        std::lower_bound(xData.begin(), xData.end(), m_startX, greaterIsLess);
    to = std::upper_bound(from, xData.end(), m_endX, greaterIsLess);
  }

  // Check whether the fitting interval defined by StartX and EndX is 0.
  // This occurs when StartX and EndX are both less than the minimum workspace
  // x-value or greater than the maximum workspace x-value.
  if (to - from == 0) {
    throw std::invalid_argument("StartX and EndX values do not capture a range "
                                "within the workspace interval.");
  }

  return std::make_pair(std::distance(xData.begin(), from),
                        std::distance(xData.begin(), to));
}

/**
 * Set all parameters.
 * @throws std::runtime_error if the Exclude property has an odd number of
 * entries.
 */
void TableWorkspaceDomainCreator::setParameters() const {
  // if property manager is set overwrite any set parameters
  if (m_manager) {
    // get the workspace
    API::Workspace_sptr ws = m_manager->getProperty(m_workspacePropertyName);
    setAndValidateWorkspace(ws);
    if (m_domainType != Simple) {
      const int maxSizeInt = m_manager->getProperty(m_maxSizePropertyName);
      m_maxSize = static_cast<size_t>(maxSizeInt);
    }
    m_exclude = m_manager->getProperty(m_excludePropertyName);
    if (m_exclude.size() % 2 != 0) {
      throw std::runtime_error("Exclude property has an odd number of entries. "
                               "It has to be even as each pair specifies a "
                               "start and an end of an interval to exclude.");
    }
    m_startX = m_manager->getProperty(m_startXPropertyName);
    m_endX = m_manager->getProperty(m_endXPropertyName);
    joinOverlappingRanges(m_exclude);
  }
}

/**
 * Get the X, Y and Error column names as a vector. If no error column is found
 * there will only be 2 entries in the vector. Gets the name from arguments if
 * specified otherwise it check if any columns have been set as plot type X, Y
 * or Yerror. If none found it will return an error
 * @returns :: a vector containing the X and Y column names and the error column
 * if it has been set
 * @throws std::invalid_argument if no X or Y value can be found or are of the
 * wrong type.
 */

void TableWorkspaceDomainCreator::setXYEColumnNames(
    API::ITableWorkspace_sptr ws) const {

  auto columnNames = ws->getColumnNames();

  std::string xColName = m_manager->getProperty(m_xColumnPropertyName);
  if (xColName != "") {
    auto column = ws->getColumn(xColName);
    if (!column->isNumber()) {
      throw std::invalid_argument(xColName + " does not contain numbers");
    }
  }

  std::string yColName = m_manager->getProperty(m_yColumnPropertyName);
  if (yColName != "") {
    auto column = ws->getColumn(yColName);
    if (!column->isNumber()) {
      throw std::invalid_argument(yColName + " does not contain numbers");
    }
  }

  std::string eColName = m_manager->getProperty(m_errorColumnPropertyName);
  if (eColName != "") {
    auto column = ws->getColumn(eColName);
    if (!column->isNumber()) {
      throw std::invalid_argument(eColName + " does not contain numbers");
    }
  }

  // get the column name from plot type
  for (const auto &name : columnNames) {
    auto column = ws->getColumn(name);
    if (xColName == "" && column->getPlotType() == 1) {
      xColName = column->name();
    }
    if (yColName == "" && column->getPlotType() == 2) {
      yColName = column->name();
    }
    if (eColName == "" && column->getPlotType() == 5) {
      eColName = column->name();
    }
  }

  if (xColName != "" && yColName != "") {
    setColumnNames(xColName, yColName, eColName);
  } else {
    throw std::invalid_argument("No valid input for X or Y column names");
  }
}

/**
 * Set the table workspace. Clones the input workspace and makes workspace
 * into form: x data | y data | error data
 * @throws std::runtime_error if the Exclude property has an odd number of
 * entries.
 */
void TableWorkspaceDomainCreator::setAndValidateWorkspace(
    API::Workspace_sptr ws) const {
  auto tableWorkspace = boost::dynamic_pointer_cast<API::ITableWorkspace>(ws);
  if (!tableWorkspace) {
    throw std::invalid_argument("InputWorkspace must be a TableWorkspace.");
  }
  setXYEColumnNames(tableWorkspace);
  std::vector<std::string> columnNames;
  columnNames.push_back(m_xColName);
  columnNames.push_back(m_yColName);
  if (m_errColName != "")
    columnNames.push_back(m_errColName);
  // table workspace is cloned so it can be changed within the domain
  m_tableWorkspace = tableWorkspace->cloneColumns(columnNames);

  // if no error column has been found a column is added with 0 errors
  if (columnNames.size() == 2) {
    m_errColName = "AddedErrorColumn";
    m_noErrCol = true;
    auto columnAdded = m_tableWorkspace->addColumn("double", m_errColName);
    if (!columnAdded)
      throw std::invalid_argument("No error column provided.");
  }

  if (m_tableWorkspace->columnCount() != 3) {
    throw std::invalid_argument("X or Y Columns not found");
  }
}
} // namespace CurveFitting
} // namespace Mantid
