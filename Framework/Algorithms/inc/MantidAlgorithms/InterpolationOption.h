// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_INTERPOLATIONOPTION_H_
#define MANTID_ALGORITHMS_INTERPOLATIONOPTION_H_

#include "MantidAlgorithms/DllConfig.h"
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

private:
  Value m_value = Value::Linear;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_INTERPOLATIONOPTION_H_ */
