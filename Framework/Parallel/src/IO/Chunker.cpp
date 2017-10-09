#include <numeric>

#include "MantidParallel/Communicator.h"
#include "MantidParallel/IO/Chunker.h"

namespace Mantid {
namespace Parallel {
namespace IO {

namespace {
/// Helper to build partition (subgroup of ranks with subgroup of banks).
std::pair<int, std::vector<size_t>>
buildPartition(const int totalWorkers, const size_t totalSize,
               std::vector<std::tuple<size_t, size_t, bool>> &sortedSizes) {
  const size_t perWorkerSize = (totalSize + totalWorkers - 1) / totalWorkers;

  // 1. Find largest unprocessed item
  auto largest =
      std::find_if_not(sortedSizes.begin(), sortedSizes.end(),
                       [](const std::tuple<size_t, size_t, bool> &item) {
                         return std::get<2>(item);
                       });
  std::vector<size_t> itemsInPartition{std::get<1>(*largest)};
  std::get<2>(*largest) = true;

  // 2. Number of workers needed for that item.
  const size_t size = std::get<0>(*largest);
  const int workers = static_cast<int>(
      (static_cast<size_t>(totalWorkers) * size + totalSize - 1) / totalSize);
  size_t remainder = workers * perWorkerSize - size;

  // 3. Fill remainder with next largest fitting size(s)
  for (auto &item : sortedSizes) {
    if (std::get<2>(item))
      continue;
    if (std::get<0>(item) <= remainder) {
      std::get<2>(item) = true;
      itemsInPartition.push_back(std::get<1>(item));
      remainder -= std::get<0>(item);
      if (remainder == 0)
        break;
    }
  }
  return {workers, itemsInPartition};
}

int numberOfWorkers(
    const std::vector<std::pair<int, std::vector<size_t>>> &partitioning) {
  int workers = 0;
  for (const auto &item : partitioning)
    workers += std::get<0>(item);
  return workers;
}

size_t taskSize(const std::pair<int, std::vector<size_t>> &partition,
                const std::vector<size_t> &tasks) {
  const int workers = std::get<0>(partition);
  if (workers == 0)
    return UINT64_MAX;
  const auto &indices = std::get<1>(partition);
  size_t total = 0;
  for (const auto index : indices)
    total += tasks[index];
  // Rounding *up*. Some workers in partition maybe have less work but we want
  // the maximum.
  return (total + workers - 1) / workers;
}
}

/// Create a chunker based on bank sizes and chunk size.
Chunker::Chunker(const int numRanks, const int rank,
                 const std::vector<size_t> &bankSizes, const size_t chunkSize)
    : m_rank(rank), m_chunkSize(chunkSize), m_bankSizes(bankSizes) {
  // Create partitions based on chunk counts.
  m_chunkCounts = m_bankSizes;
  const auto sizeToChunkCount =
      [&](size_t &value) { value = (value + m_chunkSize - 1) / m_chunkSize; };
  std::for_each(m_chunkCounts.begin(), m_chunkCounts.end(), sizeToChunkCount);
  m_partitioning = makeBalancedPartitioning(numRanks, m_chunkCounts);
}

size_t Chunker::chunkSize() const { return m_chunkSize; }

std::vector<std::vector<int>> Chunker::makeRankGroups() const {
  int rank{0};
  std::vector<std::vector<int>> rankGroups;
  for (const auto &partition : m_partitioning) {
    rankGroups.emplace_back();
    for (int i = 0; i < partition.first; ++i)
      rankGroups.back().push_back(rank++);
  }
  return rankGroups;
}

/** Returns a vector of LoadRanges based on parameters passed to the
 * constructor.
 *
 * The ranges are optimized such that the number of ranks per bank is minimized
 * while at the same time achieving good load balance by making the number of
 * chunks to be loaded by each rank as equal as possible. The current algorithm
 * does not find the optimial solution for all edge cases but should usually
 * yield a 'good-enough' approximation. There are two reasons for minimizing the
 * number of ranks per bank:
 * 1. Avoid overhead from loading event_index and event_time_zero for a bank on
 *    more ranks than necessary.
 * 2. Reduce the number of banks a rank is loading from to allow more flexible
 *    ordering when redistributing data with MPI in the loader.
 * If more than one rank is used to load a subset of banks, chunks are assigned
 * in a round-robin fashion to ranks. This is not reset when reaching the end of
 * a bank, i.e., the rank loading the first chunk of the banks in a subset is
 * *not* guarenteed to be the same for all banks. */
std::vector<Chunker::LoadRange> Chunker::makeLoadRanges() const {
  // Find our partition.
  size_t partitionIndex = 0;
  int firstRankSharingOurPartition = 0;
  for (; partitionIndex < m_partitioning.size(); ++partitionIndex) {
    const int ranksInPartition = m_partitioning[partitionIndex].first;
    if (firstRankSharingOurPartition + ranksInPartition > m_rank)
      break;
    firstRankSharingOurPartition += ranksInPartition;
  }
  const auto ranksSharingOurPartition = m_partitioning[partitionIndex].first;
  const auto &ourBanks = m_partitioning[partitionIndex].second;

  // Assign all chunks from all banks in this partition to ranks in round-robin
  // manner.
  int64_t chunk = 0;
  std::vector<LoadRange> ranges;
  for (const auto bank : ourBanks) {
    size_t current = 0;
    while (current < m_bankSizes[bank]) {
      if (chunk % ranksSharingOurPartition ==
          (m_rank - firstRankSharingOurPartition)) {
        size_t count =
            std::min(current + m_chunkSize, m_bankSizes[bank]) - current;
        ranges.push_back(LoadRange{bank, current, count});
      }
      current += m_chunkSize;
      chunk++;
    }
  }

  // Compute maximum chunk count (on any rank).
  int64_t maxChunkCount = 0;
  for (const auto &partition : m_partitioning) {
    size_t chunksInPartition = 0;
    for (const auto bank : partition.second)
      chunksInPartition += m_chunkCounts[bank];
    int ranksInPartition = partition.first;
    int64_t maxChunkCountInPartition =
        (chunksInPartition + ranksInPartition - 1) / ranksInPartition;
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
std::vector<std::pair<int, std::vector<size_t>>>
Chunker::makeBalancedPartitioning(const int workers,
                                  const std::vector<size_t> &sizes) {
  const auto totalSize =
      std::accumulate(sizes.begin(), sizes.end(), static_cast<size_t>(0));
  // Indexed size vector such that we can sort it but still know original index.
  // Elements are <size, original index, done flag>
  std::vector<std::tuple<size_t, size_t, bool>> sortedSizes;
  for (const auto size : sizes)
    sortedSizes.emplace_back(size, sortedSizes.size(), false);
  std::sort(sortedSizes.begin(), sortedSizes.end(),
            [](const std::tuple<size_t, size_t, bool> &a,
               const std::tuple<size_t, size_t, bool> &b) {
              return std::get<0>(a) > std::get<0>(b);
            });

  std::vector<std::pair<int, std::vector<size_t>>> partitioning;
  size_t numProcessed = 0;
  while (numProcessed != sizes.size()) {
    partitioning.emplace_back(buildPartition(workers, totalSize, sortedSizes));
    numProcessed += partitioning.back().second.size();
  }

  // buildPartition always rounds up when computing needed workers, so we have
  // to reduce workers for some partitions such that we stay below given total
  // workers.
  int tooMany = numberOfWorkers(partitioning) - workers;
  if (tooMany != 0) {
    for (auto &item : partitioning)
      std::get<0>(item)--;
    std::vector<size_t> taskSizes;
    for (const auto &partition : partitioning)
      taskSizes.push_back(taskSize(partition, sizes));
    for (int i = 0; i < tooMany; ++i) {
      const auto itemWithSmallestIncrease =
          std::distance(taskSizes.begin(),
                        std::min_element(taskSizes.begin(), taskSizes.end()));
      std::get<0>(partitioning[itemWithSmallestIncrease])--;
      taskSizes[itemWithSmallestIncrease] =
          taskSize(partitioning[itemWithSmallestIncrease], sizes);
    }
    for (auto &item : partitioning)
      std::get<0>(item)++;
  }

  return partitioning;
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
