// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

/// Helper calss that finds if a point should be excluded from fit.
/// It keeps the boundaries of the relevant exclusion region for
/// the last checked value. A relevant region is the one which either
/// includes the value or the nearest one with the left boundary greater
/// than the value.
/// The class also keeps the index of the region (its left boundary) for
/// efficient search.
class ExcludeRangeFinder {
  /// Index of current excluded range
  size_t m_exclIndex;
  /// Start of current excluded range
  double m_startExcludedRange;
  /// End of current excluded range
  double m_endExcludeRange;
  /// Reference to a list of exclusion ranges.
  const std::vector<double> &m_exclude;
  /// Size of m_exclude.
  const size_t m_size;

public:
  /// Constructor.
  /// @param exclude :: The value of the "Exclude" property.
  /// @param startX :: The start of the overall fit interval.
  /// @param endX :: The end of the overall fit interval.
  ExcludeRangeFinder(const std::vector<double> &exclude, double startX,
                     double endX)
      : m_exclIndex(exclude.size()), m_startExcludedRange(),
        m_endExcludeRange(), m_exclude(exclude), m_size(exclude.size()) {
    // m_exclIndex is initialised with exclude.size() to be the default when
    // there are no exclusion ranges defined.
    if (!m_exclude.empty()) {
      if (startX < m_exclude.back() && endX > m_exclude.front()) {
        // In this case there are some ranges, the index starts with 0
        // and first range is found.
        m_exclIndex = 0;
        findNextExcludedRange(startX);
      }
    }
  }

  /// Check if an x-value lies in an exclusion range.
  /// @param value :: A value to check.
  /// @returns true if the value lies in an exclusion range and should be
  /// excluded from fit.
  bool isExcluded(double value) {
    if (m_exclIndex < m_size) {
      if (value < m_startExcludedRange) {
        // If a value is below the start of the current interval
        // it is not in any other interval by the workings of
        // findNextExcludedRange
        return false;
      } else if (value <= m_endExcludeRange) {
        // value is inside
        return true;
      } else {
        // Value is past the current range. Find the next one or set the index
        // to m_exclude.size() to stop further searches.
        findNextExcludedRange(value);
        // The value can find itself inside another range.
        return isExcluded(value);
      }
    }
    return false;
  }

private:
  /// Find the range from m_exclude that may contain points x >= p .
  /// @param p :: An x value to use in the seach.
  void findNextExcludedRange(double p) {
    if (p > m_exclude.back()) {
      // If the value is past the last point stop any searches or checks.
      m_exclIndex = m_size;
      return;
    }
    // Starting with the current index m_exclIndex find the first value in
    // m_exclude that is greater than p. If this point is a start than the
    // end will be the following point. If it's an end then the start is
    // the previous point. Keep index m_exclIndex pointing to the start.
    for (auto it = m_exclude.begin() + m_exclIndex; it != m_exclude.end();
         ++it) {
      if (*it >= p) {
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
    // No need for additional checks as p < m_exclude.back()
    // and m_exclude[m_exclIndex] < p due to conditions at the calls
    // so the break statement will always be reached.
  }
};

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
    points.push_back(RangePoint{RangePoint::Openning, *point});
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
    if (point.kind == RangePoint::Openning) {
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

} // namespace

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
      auto mustBeOrderedPairs =
          boost::make_shared<ArrayOrderedPairsValidator<double>>();
      declareProperty(
          new ArrayProperty<double>(m_excludePropertyName, mustBeOrderedPairs),
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
    std::vector<double> binWidths(X.size());
    std::adjacent_difference(X.begin(), X.end(), binWidths.begin());
    for (size_t ispec = 1; ispec < mws.getNumberHistograms(); ++ispec) {
      auto &Y = mws.mutableY(ispec);
      std::transform(binWidths.begin() + 1, binWidths.end(), Y.begin(), Y.begin(), std::multiplies<double>());
    }
  }
  return ws;
}

} // namespace CurveFitting
} // namespace Mantid
