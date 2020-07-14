// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <memory>
#include <string>

namespace Mantid {
namespace HistogramData {
class Histogram;
}
namespace Kernel {
class Property;
}
namespace Algorithms {

/**
  Class to provide a consistent interface to an interpolation option on
  algorithms.
*/
class MANTID_ALGORITHMS_DLL InterpolationOption {
public:
  // Indices must match the order in static string array
  enum class Value : uint8_t { Linear, CSpline };

  void set(Value kind);
  void set(const std::string &kind);

  std::unique_ptr<Kernel::Property> property() const;
  std::string propertyDoc() const;
  std::string validateInputSize(const size_t size) const;

  void applyInplace(HistogramData::Histogram &inOut, size_t stepSize) const;
  void applyInPlace(const HistogramData::Histogram &in,
                    HistogramData::Histogram &out) const;
  HistogramData::Histogram
  interpolateFromDetectorGrid(const double lat, const double lon,
                              const API::MatrixWorkspace &ws,
                              const std::array<size_t, 4> &indices) const;
  std::array<double, 4>
  inverseDistanceWeights(const std::array<double, 4> &distances) const;

private:
  Value m_value = Value::Linear;
};

} // namespace Algorithms
} // namespace Mantid
