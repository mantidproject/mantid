#include <H5Cpp.h>

#include "MantidParallel/Communicator.h"
#include "MantidParallel/IO/Chunker.h"

namespace Mantid {
namespace Parallel {
namespace IO {
namespace Chunker {

namespace {
std::vector<size_t> readBankSizes(const H5::H5File &file,
                                  const std::string &groupName,
                                  const std::vector<std::string> &bankNames) {
  std::vector<size_t> bankSizes;
  for (const auto &bankName : bankNames) {
    const H5::DataSet dataset =
        file.openDataSet(groupName + "/" + bankName + "/event_id");
    const H5::DataSpace dataSpace = dataset.getSpace();
    bankSizes.push_back(dataSpace.getSelectNpoints());
  }
  return bankSizes;
}

std::pair<size_t, std::vector<size_t>>
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
}

/// Returns a vector of LoadRanges for the given banks in file using chunkSize.
std::vector<LoadRange>
determineLoadRanges(const H5::H5File &file, const std::string &groupName,
                    const std::vector<std::string> &bankNames,
                    const size_t chunkSize) {
  const auto &bankSizes = readBankSizes(file, groupName, bankNames);
  Communicator comm;
  return determineLoadRanges(comm.size(), comm.rank(), bankSizes, chunkSize);
}

/** Returns a vector of LoadRanges for the given bank sizes using chunkSize.
 *
 * Currently there is no guarentee that the returned vector of ranges is of
 * equal length on all ranks. If more than one rank is used to load a subset of
 * banks, chunks are assigned in a round-robin fashion to ranks. This is not
 * reset when reaching the end of a bank, i.e., the rank loading the first chunk
 * of the banks in a subset is *not* guarenteed to be the same for all banks. */
std::vector<LoadRange> MANTID_PARALLEL_DLL
determineLoadRanges(const int numRanks, const int rank,
                    const std::vector<size_t> &bankSizes,
                    const size_t chunkSize) {
  std::vector<size_t> chunkCounts(bankSizes);
  const auto sizeToChunkCount =
      [&](size_t &value) { value = (value + chunkSize - 1) / chunkSize; };
  std::for_each(chunkCounts.begin(), chunkCounts.end(), sizeToChunkCount);

  const auto partitioning = makeBalancedPartitioning(numRanks, chunkCounts);
  size_t partitionIndex = 0;
  int firstRankSharingOurPartition = 0;
  for (; partitionIndex < partitioning.size(); ++partitionIndex) {
    const int ranksInPartition = partitioning[partitionIndex].first;
    if (firstRankSharingOurPartition + ranksInPartition > rank)
      break;
    firstRankSharingOurPartition += ranksInPartition;
  }

  const auto ranksSharingOurPartition = partitioning[partitionIndex].first;
  const auto &ourBanks = partitioning[partitionIndex].second;

  int64_t chunk = 0;
  std::vector<LoadRange> ranges;
  for (const auto bank : ourBanks) {
    size_t current = 0;
    while (current < bankSizes[bank]) {
      if (chunk % ranksSharingOurPartition ==
          (rank - firstRankSharingOurPartition)) {
        hsize_t count =
            std::min(current + chunkSize, bankSizes[bank]) - current;
        ranges.push_back(LoadRange{bank, current, count});
      }
      current += chunkSize;
      chunk++;
    }
  }
  return ranges;
}

std::vector<std::pair<int, std::vector<size_t>>>
makeBalancedPartitioning(const int workers, const std::vector<size_t> &sizes) {
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

  std::vector<std::pair<int, std::vector<size_t>>> result;
  size_t numProcessed = 0;
  while (numProcessed != sizes.size()) {
    result.emplace_back(buildPartition(workers, totalSize, sortedSizes));
    numProcessed += result.back().second.size();
  }
  return result;
}

} // namespace Chunker
} // namespace IO
} // namespace Parallel
} // namespace Mantid
