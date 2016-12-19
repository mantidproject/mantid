#ifndef MANTID_INDEXING_PARTITIONING_H_
#define MANTID_INDEXING_PARTITIONING_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/PartitionIndex.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <vector>

namespace Mantid {
namespace Indexing {

/** Partitioning : TODO: DESCRIPTION

  The main intention of this class is defining partitioning of all spectrum
  numbers into subsets for an MPI-based Mantid run.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_INDEXING_DLL Partitioning {
public:
  enum class MonitorStrategy { CloneOnEachPartition, DedicatedPartition };

  Partitioning(const int numberOfPartitions, const PartitionIndex partition,
               const MonitorStrategy monitorStrategy,
               std::vector<SpectrumNumber> monitors);

  virtual ~Partitioning() = default;

  int numberOfPartitions() const;
  PartitionIndex indexOf(const SpectrumNumber spectrumNumber) const;

  bool isValid(const PartitionIndex index) const;
  void checkValid(const PartitionIndex index) const;

protected:
  bool isMonitor(const SpectrumNumber spectrumNumber) const;
  int numberOfNonMonitorPartitions() const;

private:
  virtual PartitionIndex
  doIndexOf(const SpectrumNumber spectrumNumber) const = 0;

  const int m_partitions;
  const PartitionIndex m_partition;
  const MonitorStrategy m_monitorStrategy;
  const std::vector<SpectrumNumber> m_monitors;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_PARTITIONING_H_ */
