// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_LINEARGENERATOR_H_
#define MANTID_HISTOGRAMDATA_LINEARGENERATOR_H_

#include "MantidHistogramData/DllConfig.h"

namespace Mantid {
namespace HistogramData {

/** LinearGenerator : A helper functor to generate linearly increasing
  series of double values.
*/
class LinearGenerator {
public:
  LinearGenerator(double start, double increment)
      : start(start), increment(increment) {}

  double operator()() {
    return start + increment * static_cast<double>(count++);
  }

private:
  double start;
  double increment;
  size_t count{0};
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_LINEARGENERATOR_H_ */
