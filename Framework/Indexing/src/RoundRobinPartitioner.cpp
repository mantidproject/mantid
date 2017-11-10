#include "MantidIndexing/RoundRobinPartitioner.h"

namespace Mantid {
namespace Indexing {

PartitionIndex
RoundRobinPartitioner::doIndexOf(const GlobalSpectrumIndex index) const {
  return PartitionIndex(static_cast<int>(static_cast<size_t>(index) %
                                         numberOfNonMonitorPartitions()));
}

} // namespace Indexing
} // namespace Mantid
