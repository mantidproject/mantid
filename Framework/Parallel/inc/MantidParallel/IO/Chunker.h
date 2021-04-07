// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
class Communicator;
namespace IO {

/** Chunking class for Parallel::IO::EventLoader.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_PARALLEL_DLL Chunker {
public:
  struct LoadRange {
    size_t bankIndex;
    size_t eventOffset;
    size_t eventCount;
  };
  Chunker(const int numWorkers, const int worker, const std::vector<size_t> &bankSizes, const size_t chunkSize);

  size_t chunkSize() const;

  std::vector<std::vector<int>> makeWorkerGroups() const;
  std::vector<LoadRange> makeLoadRanges() const;

  static std::vector<std::pair<int, std::vector<size_t>>> makeBalancedPartitioning(const int workers,
                                                                                   const std::vector<size_t> &sizes);

private:
  const int m_worker;
  const size_t m_chunkSize;
  std::vector<size_t> m_bankSizes;
  std::vector<size_t> m_chunkCounts;
  std::vector<std::pair<int, std::vector<size_t>>> m_partitioning;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
