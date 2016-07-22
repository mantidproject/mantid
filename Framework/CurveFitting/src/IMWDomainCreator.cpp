// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IMWDomainCreator.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/ParameterEstimator.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IEventWorkspace.h"

#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Matrix.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <algorithm>

namespace Mantid {
namespace CurveFitting {

namespace {
bool greaterIsLess(double x1, double x2) { return x1 > x2; }
}

using namespace Kernel;
using API::Workspace;
using API::Axis;
using API::MatrixWorkspace;
using API::Algorithm;
using API::Jacobian;

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
      /*m_maxSize(0), m_normalise(false),*/ m_startIndex(0) {
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
void IMWDomainCreator::declareDatasetProperties(const std::string &suffix, bool addProp) {
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

  const Mantid::MantidVec &X = m_matrixWorkspace->readX(m_workspaceIndex);
  if (X.empty()) {
    throw std::runtime_error("Workspace contains no data.");
  }

  setParameters();

  // find the fitting interval: from -> to
  Mantid::MantidVec::const_iterator from;
  Mantid::MantidVec::const_iterator to;

  bool isXAscending = X.front() < X.back();

  if (isXAscending) {
    if (m_startX == EMPTY_DBL() && m_endX == EMPTY_DBL()) {
      m_startX = X.front();
      from = X.begin();
      m_endX = X.back();
      to = X.end();
    } else if (m_startX == EMPTY_DBL() || m_endX == EMPTY_DBL()) {
      throw std::invalid_argument(
          "Both StartX and EndX must be given to set fitting interval.");
    } else {
      if (m_startX > m_endX) {
        std::swap(m_startX, m_endX);
      }
      from = std::lower_bound(X.begin(), X.end(), m_startX);
      to = std::upper_bound(from, X.end(), m_endX);
    }
  } else {
    // x is descending
    if (m_startX == EMPTY_DBL() && m_endX == EMPTY_DBL()) {
      m_startX = X.front();
      from = X.begin();
      m_endX = X.back();
      to = X.end();
    } else if (m_startX == EMPTY_DBL() || m_endX == EMPTY_DBL()) {
      throw std::invalid_argument(
          "Both StartX and EndX must be given to set fitting interval.");
    } else {
      if (m_startX < m_endX) {
        std::swap(m_startX, m_endX);
      }
      from = std::lower_bound(X.begin(), X.end(), m_startX, greaterIsLess);
      to = std::upper_bound(from, X.end(), m_endX, greaterIsLess);
    }
  }

  if (m_matrixWorkspace->isHistogramData()) {
    if (X.end() == to) {
      to = X.end() - 1;
    }
  }
  return std::make_pair(std::distance(X.begin(), from), std::distance(X.begin(), to));
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
API::MatrixWorkspace_sptr IMWDomainCreator::createEmptyResultWS(const size_t nhistograms,
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
  auto tAxis = new API::TextAxis(nhistograms);
  ws->replaceAxis(1, tAxis);

  const MantidVec &inputX = m_matrixWorkspace->readX(m_workspaceIndex);
  const MantidVec &inputY = m_matrixWorkspace->readY(m_workspaceIndex);
  const MantidVec &inputE = m_matrixWorkspace->readE(m_workspaceIndex);
  // X values for all
  for (size_t i = 0; i < nhistograms; i++) {
    ws->dataX(i).assign(inputX.begin() + m_startIndex,
                        inputX.begin() + m_startIndex + nxvalues);
  }
  // Data values for the first histogram
  ws->dataY(0).assign(inputY.begin() + m_startIndex,
                      inputY.begin() + m_startIndex + nyvalues);
  ws->dataE(0).assign(inputE.begin() + m_startIndex,
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

} // namespace Algorithm
} // namespace Mantid
