// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMW.h"
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
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Matrix.h"

#include <algorithm>
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using API::Workspace;
using API::Axis;
using API::MatrixWorkspace;
using API::Algorithm;
using API::Jacobian;

namespace {

/// Helper calss that finds if a point should be excluded from fit.
class ExcludeRangeFinder {
  /// Index of current excluded range
  size_t m_exclIndex;
  /// Start of current excluded range
  double m_startExcludedRange;
  /// End of current excluded range
  double m_endExcludeRange;
  /// Reference to a list of exclusion ranges.
  const std::vector<double> &m_exclude;

public:
  /// Constructor.
  /// @param exclude :: The value of the "Exclude" property.
  /// @param startX :: The start of the overall fit interval.
  /// @param endX :: The end of the overall fit interval.
  ExcludeRangeFinder(const std::vector<double> &exclude, double startX,
                     double endX)
      : m_exclIndex(0), m_startExcludedRange(endX), m_endExcludeRange(endX),
        m_exclude(exclude) {
    if (!m_exclude.empty()) {
      if (startX < m_exclude.back() && endX > m_exclude.front()) {
        findNextExcludedRange(startX);
      }
    }
  }
  /// Find the range from m_exclude that may contain points x >= p .
  /// @param p :: An x value to use in the seach.
  void findNextExcludedRange(double p) {
    if (p >= m_exclude.back()) {
      m_exclIndex = m_exclude.size();
      return;
    }
    for (auto it = m_exclude.begin() + m_exclIndex; it != m_exclude.end();
         ++it) {
      if (*it > p) {
        m_exclIndex = static_cast<size_t>(std::distance(m_exclude.begin(), it));
        if (m_exclIndex % 2 == 0) {
          // A number at an even position in m_exclude starts an exclude
          // range
          m_startExcludedRange = *it;
          m_endExcludeRange = *(it + 1);
        } else {
          // A number at an odd position in m_exclude ends an exclude range
          m_startExcludedRange = *(it - 1);
          m_endExcludeRange = *it;
          --m_exclIndex;
        }
        break;
      }
    }
  }

  /// Check if an x-value lies in an exclusion range.
  /// @param value :: A value to check.
  bool isExcluded(double value) {
    if (m_exclIndex < m_exclude.size() && value >= m_startExcludedRange &&
        value <= m_endExcludeRange) {
      return true;
    } else if (m_exclIndex < m_exclude.size()) {
      findNextExcludedRange(value);
    }
    return false;
  }
};
}

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
    m_exclude = m_manager->getProperty(m_excludePropertyName);
    if (m_exclude.size() % 2 != 0) {
      throw std::runtime_error("Exclude property has an odd number of entries. "
                               "It has to be even as each pair specifies a "
                               "start and an end of an interval to exclude.");
    }
    std::sort(m_exclude.begin(), m_exclude.end());
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
  m_excludePropertyName = "Exclude" + suffix;

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
    if (!m_manager->existsProperty(m_excludePropertyName)) {
      declareProperty(new ArrayProperty<double>(m_excludePropertyName),
                      "A list of pairs of doubles that specify ranges that "
                      "must be excluded from fit.");
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

  // Helps find points excluded form fit.
  ExcludeRangeFinder excludeFinder(m_exclude, X.front(), X.back());

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

    if (excludeFinder.isExcluded(X[i])) {
      weight = 0.0;
    } else if (!std::isfinite(y)) {
      // nan or inf data
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
