#include "MantidIndexing/Scatter.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidParallel/Communicator.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace Indexing {

/// Returns a scattered copy of `indexInfo` with storage mode `Distributed`.
IndexInfo scatter(const Indexing::IndexInfo &indexInfo) {
  using namespace Parallel;
  if (indexInfo.communicator().size() == 1 ||
      indexInfo.storageMode() == Parallel::StorageMode::Distributed)
    return indexInfo;
  if (indexInfo.storageMode() == Parallel::StorageMode::MasterOnly)
    throw std::runtime_error(
        "Cannot scatter IndexInfo with unsupported storage mode " +
        toString(StorageMode::MasterOnly));

  std::vector<SpectrumNumber> spectrumNumbers;
  for (size_t i = 0; i < indexInfo.size(); ++i)
    spectrumNumbers.push_back(indexInfo.spectrumNumber(i));
  IndexInfo scattered(spectrumNumbers, Parallel::StorageMode::Distributed,
                      indexInfo.communicator());
  const auto &globalSpectrumDefinitions = indexInfo.spectrumDefinitions();
  std::vector<SpectrumDefinition> spectrumDefinitions;
  for (size_t i = 0; i < indexInfo.size(); ++i)
    if (scattered.isOnThisPartition(static_cast<GlobalSpectrumIndex>(i)))
      spectrumDefinitions.emplace_back((*globalSpectrumDefinitions)[i]);
  scattered.setSpectrumDefinitions(spectrumDefinitions);
  return scattered;
}

} // namespace Indexing
} // namespace Mantid
