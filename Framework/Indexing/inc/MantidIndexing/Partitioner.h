// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/PartitionIndex.h"

#include <vector>

namespace Mantid {
namespace Indexing {

/** Partitioner defines a partitioning of a contiguous range of indices into a
  given number of partitions. A partition would typically correspond to an MPI
  rank, but use is not restricted to MPI.

  Partitioner is a base class and specific partitioning patterns are implemented
  as child classes.

  @author Simon Heybrock
  @date 2017

  The main intention of this class is defining partitioning of all spectrum
  numbers into subsets for an MPI-based Mantid run.
*/
class MANTID_INDEXING_DLL Partitioner {
public:
  enum class MonitorStrategy { TreatAsNormalSpectrum, CloneOnEachPartition, DedicatedPartition };

  Partitioner(const int numberOfPartitions, const PartitionIndex partition, const MonitorStrategy monitorStrategy,
              std::vector<GlobalSpectrumIndex> monitors = {});

  virtual ~Partitioner() = default;

  int numberOfPartitions() const;
  PartitionIndex indexOf(const GlobalSpectrumIndex index) const;

  bool isValid(const PartitionIndex index) const;
  void checkValid(const PartitionIndex index) const;

protected:
  bool isMonitor(const GlobalSpectrumIndex index) const;
  int numberOfNonMonitorPartitions() const;

private:
  virtual PartitionIndex doIndexOf(const GlobalSpectrumIndex index) const = 0;

  const int m_partitions;
  const PartitionIndex m_partition;
  const MonitorStrategy m_monitorStrategy;
  const std::vector<GlobalSpectrumIndex> m_monitors;
};

} // namespace Indexing
} // namespace Mantid
