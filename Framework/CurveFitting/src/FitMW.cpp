// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/ExcludeRangeFinder.h"
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
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Matrix.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using API::MatrixWorkspace;
using API::Workspace;

namespace {

/// Helper struct for helping with joining exclusion ranges.
/// Endge points of the ranges can be wrapped in this struct
/// and sorted together without loosing their function.
struct RangePoint {
  enum Kind : char { Openning, Closing };
  /// The kind of the point: either openning or closing the range.
  Kind kind;
  /// The value of the point.
  double value;
  /// Comparison of two points.
  /// @param point :: Another point to compare with.
  bool operator<(const RangePoint &point) const {
    if (this->value == point.value) {
      // If an Openning and Closing points have the same value
      // the Openning one should go first (be "smaller").
      // This way the procedure of joinOverlappingRanges will join
      // the ranges that meet at these points.
      return this->kind == Openning;
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
    points.emplace_back(RangePoint{RangePoint::Openning, *point});
    points.emplace_back(RangePoint{RangePoint::Closing, *(point + 1)});
  }
  // Sort the points according to the operator defined in RangePoint.
  std::sort(points.begin(), points.end());

  // Clear the argument vector.
  exclude.clear();
  // Start the level counter which shows the number of unmatched openning
  // brackets.
  size_t level(0);
  for (auto &point : points) {
    if (point.kind == RangePoint::Openning) {
      if (level == 0) {
        // First openning bracket starts a new exclusion range.
        exclude.emplace_back(point.value);
      }
      // Each openning bracket increases the level
      ++level;
    } else {
      if (level == 1) {
        // The bracket that makes level 0 is an end of a range.
        exclude.emplace_back(point.value);
      }
      // Each closing bracket decreases the level
      --level;
    }
  }
}

} // namespace

/**
 * Constructor.
 * @param fit :: Property manager with properties defining the domain to be
 * created
 * @param workspacePropertyName :: Name of the workspace property.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
FitMW::FitMW(Kernel::IPropertyManager *fit, const std::string &workspacePropertyName, FitMW::DomainType domainType)
    : IMWDomainCreator(fit, workspacePropertyName, domainType), m_maxSize(0), m_normalise(false) {}

/**
 * Constructor. Methods setWorkspace, setWorkspaceIndex and setRange must be
 * called
 * set up the creator.
 * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
 */
FitMW::FitMW(FitMW::DomainType domainType)
    : IMWDomainCreator(nullptr, std::string(), domainType), m_maxSize(10), m_normalise(false) {}

/**
 * Set all parameters.
 * @throws std::runtime_error if the Exclude property has an odd number of
 * entries.
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
    joinOverlappingRanges(m_exclude);
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
    if (m_domainType != Simple && !m_manager->existsProperty(m_maxSizePropertyName)) {
      auto mustBePositive = std::make_shared<BoundedValidator<int>>();
      mustBePositive->setLower(0);
      declareProperty(new PropertyWithValue<int>(m_maxSizePropertyName, 1, mustBePositive),
                      "The maximum number of values per a simple domain.");
    }
    if (!m_manager->existsProperty(m_normalisePropertyName)) {
      declareProperty(new PropertyWithValue<bool>(m_normalisePropertyName, false),
                      "An option to normalise the histogram data (divide be the bin "
                      "width).");
    }
    if (!m_manager->existsProperty(m_excludePropertyName)) {
      auto mustBeOrderedPairs = std::make_shared<ArrayOrderedPairsValidator<double>>();
      declareProperty(new ArrayProperty<double>(m_excludePropertyName, mustBeOrderedPairs),
                      "A list of pairs of doubles that specify ranges that "
                      "must be excluded from fit.");
    }
  }
}

/// Create a domain from the input workspace
void FitMW::createDomain(std::shared_ptr<API::FunctionDomain> &domain, std::shared_ptr<API::FunctionValues> &values,
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
          throw std::runtime_error("Error of a data point is probably too small.");
        weight = 0.0;
      }
    }

    values->setFitData(j, y);
    values->setFitWeight(j, weight);
  }
  m_domain = std::dynamic_pointer_cast<API::FunctionDomain1D>(domain);
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
std::shared_ptr<API::Workspace> FitMW::createOutputWorkspace(const std::string &baseName, API::IFunction_sptr function,
                                                             std::shared_ptr<API::FunctionDomain> domain,
                                                             std::shared_ptr<API::FunctionValues> values,
                                                             const std::string &outputWorkspacePropertyName) {
  auto ws = IMWDomainCreator::createOutputWorkspace(baseName, function, domain, values, outputWorkspacePropertyName);
  auto &mws = dynamic_cast<MatrixWorkspace &>(*ws);

  if (m_normalise && m_matrixWorkspace->isHistogramData()) {
    const auto &X = mws.x(0);
    std::vector<double> binWidths(X.size());
    std::adjacent_difference(X.begin(), X.end(), binWidths.begin());
    for (size_t ispec = 1; ispec < mws.getNumberHistograms(); ++ispec) {
      auto &Y = mws.mutableY(ispec);
      std::transform(binWidths.begin() + 1, binWidths.end(), Y.begin(), Y.begin(), std::multiplies<double>());
      auto &E = mws.mutableE(ispec);
      std::transform(binWidths.begin() + 1, binWidths.end(), E.begin(), E.begin(), std::multiplies<double>());
    }
  }
  return ws;
}

} // namespace CurveFitting
} // namespace Mantid
