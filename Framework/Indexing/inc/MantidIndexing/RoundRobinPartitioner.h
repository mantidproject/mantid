// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_ROUNDROBINPARTITIONER_H_
#define MANTID_INDEXING_ROUNDROBINPARTITIONER_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/Partitioner.h"

namespace Mantid {
namespace Indexing {

/** A round-robin partitioning pattern, i.e., indices are assigned to partitions
  one at a time, looping over partitions.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_INDEXING_DLL RoundRobinPartitioner : public Partitioner {
public:
  using Partitioner::Partitioner;

private:
  PartitionIndex doIndexOf(const GlobalSpectrumIndex index) const override;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_ROUNDROBINPARTITIONER_H_ */
