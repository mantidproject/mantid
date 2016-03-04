#ifndef MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_
#define MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_

#include "MantidDataHandling/DllConfig.h"
#include <boost/optional.hpp>
#include <vector>
#include <memory>

namespace Mantid {
namespace DataHandling {

class DataBlock;

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