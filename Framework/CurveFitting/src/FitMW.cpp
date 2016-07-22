// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMW.h"
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

private:
  size_t m_nParams;           ///< number of parameters / second dimension
  std::vector<double> m_data; ///< data storage
};

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
FitMW::FitMW(Kernel::IPropertyManager *fit,
             const std::string &workspacePropertyName,
             FitMW::DomainType domainType)
    : IMWDomainCreator(fit, workspacePropertyName, domainType),
      m_maxSize(0), m_normalise(false) {}

/**
 * Constructor. Methods setWorkspace, setWorkspaceIndex and setRange must be
 * called
 * set up the creator.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
FitMW::FitMW(FitMW::DomainType domainType)
    : IMWDomainCreator(nullptr, std::string(), domainType),
      m_maxSize(10), m_normalise(false) {}

/**
 * Set all parameters
 */
void FitMW::setParameters() const {
  IMWDomainCreator::setParameters();
  // if property manager is set overwrite any set parameters
  if (m_manager) {

    if (m_domainType != Simple) {
      const int maxSizeInt = m_manager->getProperty(m_maxSizePropertyName);
      m_maxSize = static_cast<size_t>(maxSizeInt);
    }
    m_normalise = m_manager->getProperty(m_normalisePropertyName);
  }
}

/**
 * Declare properties that specify the dataset within the workspace to fit to.
 * @param suffix :: names the dataset
 * @param addProp :: allows for the declaration of certain properties of the
 * dataset
 */
void FitMW::declareDatasetProperties(const std::string &suffix, bool addProp) {
  IMWDomainCreator::declareDatasetProperties(suffix, addProp);
  m_maxSizePropertyName = "MaxSize" + suffix;
  m_normalisePropertyName = "Normalise" + suffix;

  if (addProp) {
    if (m_domainType != Simple) {
      auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
      mustBePositive->setLower(0);
      declareProperty(new PropertyWithValue<int>(m_maxSizePropertyName, 1,
                                                 mustBePositive),
                      "The maximum number of values per a simple domain.");
    }
    declareProperty(
        new PropertyWithValue<bool>(m_normalisePropertyName, false),
        "An option to normalise the histogram data (divide be the bin width).");
  }
}

/// Create a domain from the input workspace
void FitMW::createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                         boost::shared_ptr<API::FunctionValues> &values,
                         size_t i0) {
  setParameters();

  const Mantid::MantidVec &X = m_matrixWorkspace->readX(m_workspaceIndex);

  // find the fitting interval: from -> to
  size_t endIndex = 0;
  std::tie(m_startIndex, endIndex) = getXInterval();
  auto from = X.begin() + m_startIndex;
  auto to = X.begin() + endIndex;
  auto n = endIndex - m_startIndex;

  if (m_domainType != Simple) {
    if (m_maxSize < n) {
      SeqDomain *seqDomain = SeqDomain::create(m_domainType);
      domain.reset(seqDomain);
      size_t m = 0;
      while (m < n) {
        // create a simple creator
        auto creator = new FitMW;
        creator->setWorkspace(m_matrixWorkspace);
        creator->setWorkspaceIndex(m_workspaceIndex);
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
  if (m_matrixWorkspace->isHistogramData()) {
    std::vector<double> x(static_cast<size_t>(to - from));
    auto it = from;
    for (size_t i = 0; it != to; ++it, ++i) {
      x[i] = (*it + *(it + 1)) / 2;
    }
    domain.reset(new API::FunctionDomain1DSpectrum(m_workspaceIndex, x));
    x.clear();
  } else {
    domain.reset(new API::FunctionDomain1DSpectrum(m_workspaceIndex, from, to));
  }

  if (!values) {
    values.reset(new API::FunctionValues(*domain));
  } else {
    values->expand(i0 + domain->size());
  }

  bool shouldNormalise = m_normalise && m_matrixWorkspace->isHistogramData();

  // set the data to fit to
  assert(n == domain->size());
  const Mantid::MantidVec &Y = m_matrixWorkspace->readY(m_workspaceIndex);
  const Mantid::MantidVec &E = m_matrixWorkspace->readE(m_workspaceIndex);
  if (endIndex > Y.size()) {
    throw std::runtime_error("FitMW: Inconsistent MatrixWorkspace");
  }
  // bool foundZeroOrNegativeError = false;
  for (size_t i = m_startIndex; i < endIndex; ++i) {
    size_t j = i - m_startIndex + i0;
    double y = Y[i];
    double error = E[i];
    double weight = 0.0;

    if (shouldNormalise) {
      double binWidth = X[i + 1] - X[i];
      if (binWidth == 0.0) {
        throw std::runtime_error("Zero width bin found, division by zero.");
      }
      y /= binWidth;
      error /= binWidth;
    }

    if (!boost::math::isfinite(y)) // nan or inf data
    {
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
      y = 0.0; // leaving inf or nan would break the fit
    } else if (!boost::math::isfinite(error)) // nan or inf error
    {
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
    } else if (error <= 0) {
      if (!m_ignoreInvalidData)
        weight = 1.0;
    } else {
      weight = 1.0 / error;
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
FitMW::createOutputWorkspace(const std::string &baseName,
                             API::IFunction_sptr function,
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

  bool shouldDeNormalise = m_normalise && m_matrixWorkspace->isHistogramData();

  // Set the difference spectrum
  const MantidVec &X = ws->readX(0);
  MantidVec &Ycal = ws->dataY(1);
  MantidVec &Diff = ws->dataY(2);
  const size_t nData = values->size();
  for (size_t i = 0; i < nData; ++i) {
    Diff[i] = values->getFitData(i) - Ycal[i];
    if (shouldDeNormalise) {
      double binWidth = X[i + 1] - X[i];
      Ycal[i] *= binWidth;
      Diff[i] *= binWidth;
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

//--------------------------------------------------------------------------------------------------------------
/**
 * @param functionList The current list of functions to append to
 * @param function A function that may or not be composite
 */
void FitMW::appendCompositeFunctionMembers(
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
void FitMW::appendConvolvedCompositeFunctionMembers(
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
void FitMW::addFunctionValuesToWS(
    const API::IFunction_sptr &function,
    boost::shared_ptr<API::MatrixWorkspace> &ws, const size_t wsIndex,
    const boost::shared_ptr<API::FunctionDomain> &domain,
    boost::shared_ptr<API::FunctionValues> resultValues) const {
  const size_t nData = resultValues->size();
  resultValues->zeroCalculated();

  // Function value
  function->function(*domain, *resultValues);

  size_t nParams = function->nParams();
  // and errors
  SimpleJacobian J(nData, nParams);
  try {
    function->functionDeriv(*domain, J);
  } catch (...) {
    function->calNumericalDeriv(*domain, J);
  }

  // the function should contain the parameter's covariance matrix
  auto covar = function->getCovarianceMatrix();

  if (covar) {
    // if the function has a covariance matrix attached - use it for the errors
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
    MantidVec &yValues = ws->dataY(wsIndex);
    MantidVec &eValues = ws->dataE(wsIndex);
    for (size_t i = 0; i < nData; i++) {
      yValues[i] = resultValues->getCalculated(i);
      eValues[i] = std::sqrt(E[i] * chi2);
    }

  } else {
    // otherwise use the parameter errors which is OK for uncorrelated
    // parameters
    MantidVec &yValues = ws->dataY(wsIndex);
    MantidVec &eValues = ws->dataE(wsIndex);
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
}

} // namespace Algorithm
} // namespace Mantid
