// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidIndexing/RoundRobinPartitioner.h"

namespace Mantid {
namespace Indexing {

PartitionIndex RoundRobinPartitioner::doIndexOf(const GlobalSpectrumIndex index) const {
  return PartitionIndex(static_cast<int>(static_cast<size_t>(index) % numberOfNonMonitorPartitions()));
}

} // namespace Indexing
} // namespace Mantid
