#include "MantidIndexing/Partitioning.h"

#include <algorithm>
#include <stdexcept>

namespace Mantid {
namespace Indexing {

Partitioning::Partitioning(const int numberOfPartitions,
                           const PartitionIndex partition,
                           const MonitorStrategy monitorStrategy,
                           std::vector<SpectrumNumber> monitors)
    : m_partitions(numberOfPartitions), m_partition(partition),
      m_monitorStrategy(monitorStrategy), m_monitors(std::move(monitors)) {
  if (numberOfNonMonitorPartitions() < 1)
    throw std::logic_error("Partitioning: Number of non-monitor partitions "
                           "must be larger than 0.");
}

int Partitioning::numberOfPartitions() const { return m_partitions; }

int Partitioning::numberOfNonMonitorPartitions() const {
  if (m_monitorStrategy == MonitorStrategy::DedicatedPartition)
    return m_partitions - 1;
  return m_partitions;
}

PartitionIndex
Partitioning::indexOf(const SpectrumNumber spectrumNumber) const {
  if (isMonitor(spectrumNumber)) {
    if (m_monitorStrategy == MonitorStrategy::DedicatedPartition)
      return PartitionIndex(numberOfPartitions() - 1);
    return m_partition;
  }
  return doIndexOf(spectrumNumber);
}

bool Partitioning::isValid(const PartitionIndex index) const {
  return index >= 0 && index < numberOfPartitions();
}

void Partitioning::checkValid(const PartitionIndex index) const {
  if (!isValid(index))
    throw std::out_of_range(
        "PartitioningIndex is out of range 0...numberOfPartitions-1");
}

bool Partitioning::isMonitor(const SpectrumNumber spectrumNumber) const {
  return std::find(m_monitors.begin(), m_monitors.end(), spectrumNumber) !=
         m_monitors.end();
}

} // namespace Indexing
} // namespace Mantid
