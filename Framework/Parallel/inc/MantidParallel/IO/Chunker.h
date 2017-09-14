#ifndef MANTID_PARALLEL_CHUNKER_H_
#define MANTID_PARALLEL_CHUNKER_H_

#include <vector>

#include "MantidParallel/DllConfig.h"

namespace H5 {
class H5File;
}

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
  Chunker(const Communicator &comm, const H5::H5File &file,
          const std::string &groupName,
          const std::vector<std::string> &bankNames, const size_t chunkSize);
  Chunker(const int numRanks, const int rank,
          const std::vector<size_t> &bankSizes, const size_t chunkSize);

  std::vector<std::vector<int>> makeRankGroups() const;
  std::vector<LoadRange> makeLoadRanges() const;

  static std::vector<std::pair<int, std::vector<size_t>>>
  makeBalancedPartitioning(const int workers, const std::vector<size_t> &sizes);

private:
  const int m_rank;
  const size_t m_chunkSize;
  std::vector<size_t> m_bankSizes;
  std::vector<size_t> m_chunkCounts;
  std::vector<std::pair<int, std::vector<size_t>>> m_partitioning;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_CHUNKER_H_ */
