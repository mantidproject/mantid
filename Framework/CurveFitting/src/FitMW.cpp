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

#include <cmath>
#include <algorithm>

namespace Mantid {
namespace CurveFitting {

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
    : IMWDomainCreator(fit, workspacePropertyName, domainType), m_maxSize(0),
      m_normalise(false) {}

/**
 * Constructor. Methods setWorkspace, setWorkspaceIndex and setRange must be
 * called
 * set up the creator.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
FitMW::FitMW(FitMW::DomainType domainType)
    : IMWDomainCreator(nullptr, std::string(), domainType), m_maxSize(10),
      m_normalise(false) {}

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
    if (m_domainType != Simple &&
        !m_manager->existsProperty(m_maxSizePropertyName)) {
      auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
      mustBePositive->setLower(0);
      declareProperty(
          new PropertyWithValue<int>(m_maxSizePropertyName, 1, mustBePositive),
          "The maximum number of values per a simple domain.");
    }
    if (!m_manager->existsProperty(m_normalisePropertyName)) {
      declareProperty(
          new PropertyWithValue<bool>(m_normalisePropertyName, false),
          "An option to normalise the histogram data (divide be the bin "
          "width).");
    }
  }
}

/// Create a domain from the input workspace
void FitMW::createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                         boost::shared_ptr<API::FunctionValues> &values,
                         size_t i0) {
  setParameters();

  const auto &X = m_matrixWorkspace->x(m_workspaceIndex);

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
  const auto &Y = m_matrixWorkspace->y(m_workspaceIndex);
  const auto &E = m_matrixWorkspace->e(m_workspaceIndex);
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

    if (!std::isfinite(y)) // nan or inf data
    {
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
      y = 0.0; // leaving inf or nan would break the fit
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
FitMW::createOutputWorkspace(const std::string &baseName,
                             API::IFunction_sptr function,
                             boost::shared_ptr<API::FunctionDomain> domain,
                             boost::shared_ptr<API::FunctionValues> values,
                             const std::string &outputWorkspacePropertyName) {
  auto ws = IMWDomainCreator::createOutputWorkspace(
      baseName, function, domain, values, outputWorkspacePropertyName);
  auto &mws = dynamic_cast<MatrixWorkspace &>(*ws);

  if (m_normalise && m_matrixWorkspace->isHistogramData()) {
    const auto &X = mws.x(0);
    auto &Ycal = mws.mutableY(1);
    auto &Diff = mws.mutableY(2);
    const size_t nData = values->size();
    for (size_t i = 0; i < nData; ++i) {
      double binWidth = X[i + 1] - X[i];
      Ycal[i] *= binWidth;
      Diff[i] *= binWidth;
    }
  }
  return ws;
}

} // namespace Algorithm
} // namespace Mantid
