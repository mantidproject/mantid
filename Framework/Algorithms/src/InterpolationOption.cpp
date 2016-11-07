#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/PropertyWithValue.h"

#include <cassert>

namespace {
// The name of the interpolation property
std::string PROP_NAME("Interpolation");
std::string LINEAR_OPT("Linear");
std::string CSPLINE_OPT("CSpline");
std::vector<std::string> OPTIONS{LINEAR_OPT, CSPLINE_OPT};
}

namespace Mantid {
using HistogramData::interpolateLinearInplace;
using HistogramData::interpolateCSplineInplace;
using Kernel::Property;

namespace Algorithms {

/**
 * Constructs an object with the default interpolation method
 */
InterpolationOption::InterpolationOption() { set(Value::Linear); }

/**
 * Set the interpolation option
 * @param kind Set the type of interpolation on the call to apply
 */
void InterpolationOption::set(InterpolationOption::Value kind) {
  if (kind == Value::Linear) {
    m_impl = interpolateLinearInplace;
  } else {
    m_impl = interpolateCSplineInplace;
  }
}

/**
 * Set the interpolation option
 * @param kind Set the type of interpolation on the call to apply
 */
void InterpolationOption::set(const std::string &kind) {
  if (kind == LINEAR_OPT) {
    m_impl = interpolateLinearInplace;
  } else if (kind == CSPLINE_OPT) {
    m_impl = interpolateCSplineInplace;
  } else {
    throw std::invalid_argument(
        "InterpolationOption::set() - Unknown interpolation method '" + kind +
        "'");
  }
}

/**
 * Add the interpolation property to the given algorithm
 * @param algorithm A reference to the algorithm that wil receive the new
 * property
 */

std::unique_ptr<Property> InterpolationOption::property() const {
  using Kernel::StringListValidator;
  using StringProperty = Kernel::PropertyWithValue<std::string>;
  using StringVector = std::vector<std::string>;

  return Kernel::make_unique<StringProperty>(
      PROP_NAME, LINEAR_OPT, boost::make_shared<StringListValidator>(OPTIONS));
}

/**
 * @return The documentation string for the property
 */
std::string InterpolationOption::propertyDoc() const {
  return "Method of interpolation used to compute unsimulated values.";
}

/**
 * Apply the interpolation method to the given histogram
 * @param input
 * @param stepSize
 */
void InterpolationOption::applyInplace(HistogramData::Histogram &inOut,
                                       size_t stepSize) const {
  assert(m_impl);
  (*m_impl)(inOut, stepSize);
}

} // namespace Algorithms
} // namespace Mantid
