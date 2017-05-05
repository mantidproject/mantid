#include "MantidIndexing/MasterOnlyPartitioner.h"

namespace Mantid {
namespace Indexing {

PartitionIndex
MasterOnlyPartitioner::doIndexOf(const GlobalSpectrumIndex index) const {
  static_cast<void>(index);
  return PartitionIndex(0);
}

} // namespace Indexing
} // namespace Mantid
