#include "MantidIndexing/Partitioning.h"

#include <stdexcept>

namespace Mantid {
namespace Indexing {

bool Partitioning::isValid(const PartitionIndex &index) const {
  return index >= 0 && index < numberOfPartitions();
}

void Partitioning::checkValid(const PartitionIndex &index) const {
  if (!isValid(index))
    throw std::out_of_range(
        "PartitioningIndex is out of range 0...numberOfPartitions-1");
}

} // namespace Indexing
} // namespace Mantid
