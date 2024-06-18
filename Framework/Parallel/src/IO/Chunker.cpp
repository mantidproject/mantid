// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <iterator>
#include <numeric>
#include <tuple>
#include <utility>

#include "MantidParallel/IO/Chunker.h"

namespace Mantid::Parallel::IO {

namespace {
/** Helper to build partition (subgroup of workers with subgroup of banks).
 *
 * Elements of `sortedSizes` are <size, original index, done flag>. The
 * `padding` argument is used to artificially increase the amount of work
 * assigned to each group. This is used to deal with cases where
 *`buildPartition` generates more groups than available workers. */
std::pair<int, std::vector<size_t>> buildPartition(const int totalWorkers, const size_t totalSize,
                                                   std::vector<std::tuple<size_t, size_t, bool>> &sortedSizes,
                                                   const size_t padding) {
  const size_t perWorkerSize = (totalSize + totalWorkers - 1) / totalWorkers;

  // 1. Find largest unprocessed item
  auto largest = std::find_if_not(sortedSizes.begin(), sortedSizes.end(),
                                  [](const std::tuple<size_t, size_t, bool> &item) { return std::get<2>(item); });
  std::vector<size_t> itemsInPartition{std::get<1>(*largest)};
  std::get<2>(*largest) = true;

  // 2. Number of workers needed for that item.
  const size_t size = std::get<0>(*largest);
  const int workers = totalSize != 0
                          ? static_cast<int>((static_cast<size_t>(totalWorkers) * size + totalSize - 1) / totalSize)
                          : totalWorkers;
  size_t remainder = workers * perWorkerSize - size + padding;

  // 3. Fill remainder with next largest fitting size(s)
  for (auto &item : sortedSizes) {
    if (std::get<2>(item))
      continue;
    if (std::get<0>(item) <= remainder) {
      std::get<2>(item) = true;
      itemsInPartition.emplace_back(std::get<1>(item));
      remainder -= std::get<0>(item);
    }
  }
  return {workers, itemsInPartition};
}

int numberOfWorkers(const std::vector<std::pair<int, std::vector<size_t>>> &partitioning) {
  return std::accumulate(partitioning.cbegin(), partitioning.cend(), 0,
                         [](const int sum, const auto &item) { return sum + std::get<0>(item); });
}

size_t taskSize(const std::pair<int, std::vector<size_t>> &partition, const std::vector<size_t> &tasks) {
  const int workers = std::get<0>(partition);
  if (workers == 0)
    return UINT64_MAX;
  const auto &indices = std::get<1>(partition);
  const size_t total = std::accumulate(indices.cbegin(), indices.cend(), static_cast<size_t>(0),
                                       [&tasks](auto sum, const auto index) { return sum + tasks[index]; });
  // Rounding *up*. Some workers in partition maybe have less work but we want
  // the maximum.
  return (total + workers - 1) / workers;
}
} // namespace

/** Create a chunker based on bank sizes and chunk size.
 *
 * The `bankSizes` define the items of work to be split up amongst the workers.
 * This is done using the given `chunkSize`, i.e., each bank size is cut into
 * pieces of size `chunkSize` and all pieces are assigned to the requested
 * number of workers. */
Chunker::Chunker(const int numWorkers, const int worker, std::vector<size_t> bankSizes, const size_t chunkSize)
    : m_worker(worker), m_chunkSize(chunkSize), m_bankSizes(std::move(bankSizes)) {
  // Create partitions based on chunk counts.
  m_chunkCounts = m_bankSizes;
  const auto sizeToChunkCount = [&](size_t &value) { value = (value + m_chunkSize - 1) / m_chunkSize; };
  std::for_each(m_chunkCounts.begin(), m_chunkCounts.end(), sizeToChunkCount);
  m_partitioning = makeBalancedPartitioning(numWorkers, m_chunkCounts);
}

size_t Chunker::chunkSize() const { return m_chunkSize; }

std::vector<std::vector<int>> Chunker::makeWorkerGroups() const {
  int worker{0};
  std::vector<std::vector<int>> workerGroups;
  for (const auto &partition : m_partitioning) {
    workerGroups.emplace_back();
    for (int i = 0; i < partition.first; ++i)
      workerGroups.back().emplace_back(worker++);
  }
  return workerGroups;
}

/** Returns a vector of LoadRanges based on parameters passed to the
 * constructor.
 *
 * The ranges are optimized such that the number of workers per bank is
 * minimized while at the same time achieving good load balance by making the
 * number of chunks to be loaded by each worker as equal as possible. The
 * current algorithm does not find the optimial solution for all edge cases but
 * should usually yield a 'good-enough' approximation. There are two reasons for
 * minimizing the number of workers per bank:
 * 1. Avoid overhead from loading event_index and event_time_zero for a bank on
 *    more workers than necessary.
 * 2. Reduce the number of banks a worker is loading from.
 * If more than one worker is used to load a subset of banks, chunks are
 * assigned in a round-robin fashion to workers. This is not reset when reaching
 * the end of a bank, i.e., the worker loading the first chunk of the banks in a
 * subset is *not* guarenteed to be the same for all banks. */
std::vector<Chunker::LoadRange> Chunker::makeLoadRanges() const {
  // Find our partition.
  size_t partitionIndex = 0;
  int firstWorkerSharingOurPartition = 0;
  for (; partitionIndex < m_partitioning.size(); ++partitionIndex) {
    const int workersInPartition = m_partitioning[partitionIndex].first;
    if (firstWorkerSharingOurPartition + workersInPartition > m_worker)
      break;
    firstWorkerSharingOurPartition += workersInPartition;
  }
  const auto workersSharingOurPartition = m_partitioning[partitionIndex].first;
  const auto &ourBanks = m_partitioning[partitionIndex].second;

  // Assign all chunks from all banks in this partition to workers in
  // round-robin manner.
  int64_t chunk = 0;
  std::vector<LoadRange> ranges;
  for (const auto bank : ourBanks) {
    size_t current = 0;
    while (current < m_bankSizes[bank]) {
      if (chunk % workersSharingOurPartition == (m_worker - firstWorkerSharingOurPartition)) {
        size_t count = std::min(current + m_chunkSize, m_bankSizes[bank]) - current;
        ranges.emplace_back(LoadRange{bank, current, count});
      }
      current += m_chunkSize;
      chunk++;
    }
  }

  // Compute maximum chunk count (on any worker).
  int64_t maxChunkCount = 0;
  for (const auto &partition : m_partitioning) {
    const size_t chunksInPartition =
        std::accumulate(partition.second.cbegin(), partition.second.cend(), static_cast<size_t>(0),
                        [this](auto sum, const auto bank) { return sum + m_chunkCounts[bank]; });
    const int workersInPartition = partition.first;
    int64_t maxChunkCountInPartition = (chunksInPartition + workersInPartition - 1) / workersInPartition;
    maxChunkCount = std::max(maxChunkCount, maxChunkCountInPartition);
  }
  ranges.resize(maxChunkCount);

  return ranges;
}

/** Returns a vector of partitions of work (sizes) for given number of workers.
 *
 * The `sizes` argument defines the amount of work for a series of tasks. Here
 * the task would be loading/processing a certain number of chunks from a file.
 * The `workers` argument gives the number of workers to be used to process all
 * tasks. The returned partitioning fulfils the following:
 * - A task may be shared among workers. If workers share a task, all tasks they
 *   are working on are shared among that group of workers.
 * - Groups of workers and distribution of tasks to groups tries to balance
 *   work, such that each worker has a roughly similar amount of work.
 * Note that this method is public and static to allow for testing. */
std::vector<std::pair<int, std::vector<size_t>>> Chunker::makeBalancedPartitioning(const int workers,
                                                                                   const std::vector<size_t> &sizes) {
  const auto totalSize = std::accumulate(sizes.begin(), sizes.end(), static_cast<size_t>(0));
  // Indexed size vector such that we can sort it but still know original index.
  // Elements are <size, original index, done flag>
  std::vector<std::tuple<size_t, size_t, bool>> sortedSizes;
  std::transform(sizes.cbegin(), sizes.cend(), std::back_inserter(sortedSizes),
                 [&sortedSizes](const auto &size) { return std::make_tuple(size, sortedSizes.size(), false); });
  std::sort(sortedSizes.begin(), sortedSizes.end(),
            [](const auto &a, const auto &b) { return std::get<0>(a) > std::get<0>(b); });

  std::vector<std::pair<int, std::vector<size_t>>> partitioning;
  size_t numProcessed = 0;
  size_t padding = 0;
  const auto originalSortedSizes(sortedSizes);
  while (numProcessed != sizes.size()) {
    partitioning.emplace_back(buildPartition(workers, totalSize, sortedSizes, padding));
    numProcessed += partitioning.back().second.size();
    if (static_cast<int>(partitioning.size()) > workers) {
      partitioning.clear();
      numProcessed = 0;
      padding += static_cast<size_t>(std::max(1.0, static_cast<double>(totalSize) * 0.01));
      sortedSizes = originalSortedSizes;
    }
  }

  // buildPartition always rounds up when computing needed workers, so we have
  // to reduce workers for some partitions such that we stay below given total
  // workers.
  int tooMany = numberOfWorkers(partitioning) - workers;
  if (tooMany != 0) {
    for (auto &item : partitioning)
      std::get<0>(item)--;
    std::vector<size_t> taskSizes;
    taskSizes.reserve(partitioning.size());
    std::transform(partitioning.cbegin(), partitioning.cend(), std::back_inserter(taskSizes),
                   [&sizes](const auto &partition) { return taskSize(partition, sizes); });
    for (int i = 0; i < tooMany; ++i) {
      const auto itemWithSmallestIncrease =
          std::distance(taskSizes.begin(), std::min_element(taskSizes.begin(), taskSizes.end()));
      std::get<0>(partitioning[itemWithSmallestIncrease])--;
      taskSizes[itemWithSmallestIncrease] = taskSize(partitioning[itemWithSmallestIncrease], sizes);
    }
    for (auto &item : partitioning)
      std::get<0>(item)++;
  }

  // In some cases there are also unused workers, assign them such that client
  // code has consistent partitioning.
  int tooFew = workers - numberOfWorkers(partitioning);
  if (tooFew != 0)
    partitioning.push_back({tooFew, {}});

  return partitioning;
}

} // namespace Mantid::Parallel::IO
