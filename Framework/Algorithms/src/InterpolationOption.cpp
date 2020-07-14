// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/ListValidator.h"

#include <cassert>

namespace {
// The name of the interpolation property
std::string PROP_NAME("Interpolation");
std::string LINEAR_OPT("Linear");
std::string CSPLINE_OPT("CSpline");
std::vector<std::string> OPTIONS{LINEAR_OPT, CSPLINE_OPT};
} // namespace

namespace Mantid {
using HistogramData::interpolateCSplineInplace;
using HistogramData::interpolateLinearInplace;
using HistogramData::minSizeForCSplineInterpolation;
using HistogramData::minSizeForLinearInterpolation;
using Kernel::Property;

namespace Algorithms {

/**
 * Set the interpolation option
 * @param kind Set the type of interpolation on the call to apply
 */
void InterpolationOption::set(InterpolationOption::Value kind) {
  m_value = kind;
}

/**
 * Set the interpolation option
 * @param kind Set the type of interpolation on the call to apply
 */
void InterpolationOption::set(const std::string &kind) {
  if (kind == LINEAR_OPT) {
    m_value = Value::Linear;
  } else if (kind == CSPLINE_OPT) {
    m_value = Value::CSpline;
  } else {
    throw std::invalid_argument(
        "InterpolationOption::set() - Unknown interpolation method '" + kind +
        "'");
  }
}

/**
 * Create a property suitable to attach to an algorithm to support interpolation
 * @return A new Property containing the valid list of interpolation methods
 */

std::unique_ptr<Property> InterpolationOption::property() const {
  using Kernel::StringListValidator;
  using StringProperty = Kernel::PropertyWithValue<std::string>;

  return std::make_unique<StringProperty>(
      PROP_NAME, LINEAR_OPT, std::make_shared<StringListValidator>(OPTIONS));
}

/**
 * @return The documentation string for the property
 */
std::string InterpolationOption::propertyDoc() const {
  return "Method of interpolation used to compute unsimulated values.";
}

/**
 * Validate the size of input histogram
 * @param size number of points in the input histogram
 * @return an error message if there was one, otherwise an empty string
 */
std::string InterpolationOption::validateInputSize(const size_t size) const {
  size_t nMin;
  switch (m_value) {
  case Value::Linear:
    nMin = minSizeForLinearInterpolation();
    if (size < nMin) {
      return "Linear interpolation requires at least " + std::to_string(nMin) +
             " points.";
    }
    break;
  case Value::CSpline:
    nMin = minSizeForCSplineInterpolation();
    if (size < nMin) {
      return "CSpline interpolation requires at least " + std::to_string(nMin) +
             " points.";
    }
    break;
  }
  return std::string();
}

/**
 * Apply the interpolation method to the given histogram
 * @param inOut A reference to a histogram to interpolate
 * @param stepSize The step size of calculated points
 */
void InterpolationOption::applyInplace(HistogramData::Histogram &inOut,
                                       size_t stepSize) const {
  switch (m_value) {
  case Value::Linear:
    interpolateLinearInplace(inOut, stepSize);
    return;
  case Value::CSpline:
    interpolateCSplineInplace(inOut, stepSize);
    return;
  default:
    throw std::runtime_error("InterpolationOption::applyInplace() - "
                             "Unimplemented interpolation method.");
  }
}

/**
 * Apply the interpolation method to the output histogram.
 * @param in A histogram from which to interpolate
 * @param out A histogram where to store the interpolated values
 * @throw runtime_error Indicates unknown interpolatio method.
 */
void InterpolationOption::applyInPlace(const HistogramData::Histogram &in,
                                       HistogramData::Histogram &out) const {
  switch (m_value) {
  case Value::Linear:
    interpolateLinearInplace(in, out);
    return;
  case Value::CSpline:
    interpolateCSplineInplace(in, out);
    return;
  default:
    throw std::runtime_error("InterpolationOption::applyInplace() - "
                             "Unimplemented interpolation method.");
  }
}

/** Spatially interpolate a single histogram from four nearby detectors.
 *  @param lat Latitude of the interpolated detector.
 *  @param lon Longitude of the interpolated detector.
 *  @param ws A workspace containing the detectors used for the interpolation.
 *  @param indices Indices to the nearest neighbour detectors.
 *  @return An interpolated histogram.
 */
HistogramData::Histogram InterpolationOption::interpolateFromDetectorGrid(
    const double lat, const double lon, const API::MatrixWorkspace &ws,
    const std::array<size_t, 4> &indices) const {
  auto h = ws.histogram(0);
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto refFrame = ws.getInstrument()->getReferenceFrame();
  std::array<double, 4> distances;
  for (size_t i = 0; i < 4; ++i) {
    double detLat, detLong;
    std::tie(detLat, detLong) = spectrumInfo.geographicalAngles(indices[i]);
    distances[i] = spectrumInfo.greatCircleDistance(lat, lon, detLat, detLong);
  }
  const auto weights = inverseDistanceWeights(distances);
  auto weightSum = weights[0];
  h.mutableY() = weights[0] * ws.y(indices[0]);
  for (size_t i = 1; i < 4; ++i) {
    weightSum += weights[i];
    h.mutableY() += weights[i] * ws.y(indices[i]);
  }
  h.mutableY() /= weightSum;
  return h;
}

/** Calculate the inverse distance weights for the given distances.
 *  @param distances The distances.
 *  @return An array of inverse distance weights.
 */
std::array<double, 4> InterpolationOption::inverseDistanceWeights(
    const std::array<double, 4> &distances) const{
  std::array<double, 4> weights;
  for (size_t i = 0; i < weights.size(); ++i) {
    if (distances[i] == 0.0) {
      weights.fill(0.0);
      weights[i] = 1.0;
      return weights;
    }
    weights[i] = 1.0 / distances[i] / distances[i];
  }
  return weights;
}

} // namespace Algorithms
} // namespace Mantid
