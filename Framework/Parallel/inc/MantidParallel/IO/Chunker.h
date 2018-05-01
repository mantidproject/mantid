#ifndef MANTID_PARALLEL_CHUNKER_H_
#define MANTID_PARALLEL_CHUNKER_H_

#include <vector>

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
class Communicator;
namespace IO {

/** Chunking class for Parallel::IO::EventLoader.

  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_PARALLEL_DLL Chunker {
public:
  struct LoadRange {
    size_t bankIndex;
    size_t eventOffset;
    size_t eventCount;
  };
  Chunker(const int numWorkers, const int worker,
          const std::vector<size_t> &bankSizes, const size_t chunkSize);

  size_t chunkSize() const;

  std::vector<std::vector<int>> makeWorkerGroups() const;
  std::vector<LoadRange> makeLoadRanges() const;

  static std::vector<std::pair<int, std::vector<size_t>>>
  makeBalancedPartitioning(const int workers, const std::vector<size_t> &sizes);

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

#endif /* MANTID_PARALLEL_CHUNKER_H_ */
