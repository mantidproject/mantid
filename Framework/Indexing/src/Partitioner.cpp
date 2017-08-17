#include "MantidIndexing/Partitioner.h"

#include <algorithm>
#include <stdexcept>

namespace Mantid {
namespace Indexing {

Partitioner::Partitioner(const int numberOfPartitions,
                         const PartitionIndex partition,
                         const MonitorStrategy monitorStrategy,
                         std::vector<GlobalSpectrumIndex> monitors)
    : m_partitions(numberOfPartitions), m_partition(partition),
      m_monitorStrategy(monitorStrategy), m_monitors(std::move(monitors)) {
  if (numberOfNonMonitorPartitions() < 1)
    throw std::logic_error("Partitioner: Number of non-monitor partitions "
                           "must be larger than 0.");
}

int Partitioner::numberOfPartitions() const { return m_partitions; }

int Partitioner::numberOfNonMonitorPartitions() const {
  if (m_monitorStrategy == MonitorStrategy::DedicatedPartition)
    return m_partitions - 1;
  return m_partitions;
}

PartitionIndex Partitioner::indexOf(const GlobalSpectrumIndex index) const {
  if (isMonitor(index)) {
    if (m_monitorStrategy == MonitorStrategy::DedicatedPartition)
      return PartitionIndex(numberOfPartitions() - 1);
    return m_partition;
  }
  return doIndexOf(index);
}

bool Partitioner::isValid(const PartitionIndex index) const {
  return index >= 0 && index < numberOfPartitions();
}

void Partitioner::checkValid(const PartitionIndex index) const {
  if (!isValid(index))
    throw std::out_of_range(
        "PartitionerIndex is out of range 0...numberOfPartitions-1");
}

bool Partitioner::isMonitor(const GlobalSpectrumIndex index) const {
  if (m_monitorStrategy == MonitorStrategy::TreatAsNormalSpectrum)
    return false;
  return std::find(m_monitors.begin(), m_monitors.end(), index) !=
         m_monitors.end();
}

} // namespace Indexing
} // namespace Mantid
