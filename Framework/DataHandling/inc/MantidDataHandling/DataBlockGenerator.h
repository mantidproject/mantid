// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_
#define MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_

#include "MantidDataHandling/DllConfig.h"
#include <boost/optional.hpp>
#include <memory>
#include <vector>

namespace Mantid {
namespace DataHandling {

class DataBlock;

/** DataBlockGenerator: The DataBlockGenerator class provides increasing
    int64_t numbers from a collection of intervals which are being input
    into the generator at construction.
*/
class DLLExport DataBlockGenerator {
public:
  DataBlockGenerator(const std::vector<std::pair<int64_t, int64_t>> &intervals);
  class DataBlock;
  DataBlockGenerator &operator++();
  DataBlockGenerator operator++(int);
  bool isDone();
  int64_t getValue();
  void next();

public:
  std::vector<std::pair<int64_t, int64_t>> m_intervals;
  int64_t m_currentSpectrum;

  boost::optional<size_t> m_currentIntervalIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif
