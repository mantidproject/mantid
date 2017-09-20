#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidIndexing/Extract.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid::API;
using namespace Mantid::Indexing;

namespace Mantid {
namespace DataHandling {

LoadEventNexusIndexSetup::LoadEventNexusIndexSetup(
    MatrixWorkspace_const_sptr instrumentWorkspace, const int32_t min,
    const int32_t max, const std::vector<int32_t> range)
    : m_instrumentWorkspace(instrumentWorkspace), m_min(min), m_max(max),
      m_range(range) {}

std::pair<int32_t, int32_t> LoadEventNexusIndexSetup::eventIDLimits() const {
  return {m_min, m_max};
}

IndexInfo LoadEventNexusIndexSetup::makeIndexInfo() {
  // The default 1:1 will suffice but exclude the monitors as they are always in
  // a separate workspace
  auto detIDs = m_instrumentWorkspace->getInstrument()->getDetectorIDs(true);
  const auto &detectorInfo = m_instrumentWorkspace->detectorInfo();
  std::vector<SpectrumDefinition> specDefs;
  for (const auto detID : detIDs)
    specDefs.emplace_back(detectorInfo.indexOf(detID));
  // We need to filter based on detector IDs, but use IndexInfo for filtering
  // for a unified filtering mechanism. Thus we set detector IDs as (temporary)
  // spectrum numbers.
  IndexInfo indexInfo(
      std::vector<SpectrumNumber>(detIDs.begin(), detIDs.end()));
  indexInfo.setSpectrumDefinitions(specDefs);

  auto filtered = filterIndexInfo(indexInfo);

  // If there is no actual filter, spectrum numbers are contiguous and start at
  // 1. Otherwise spectrum numbers are detector IDs. This is legacy behavior
  // adopted from EventWorkspaceCollection.
  if (filtered.size() == indexInfo.size())
    filtered.setSpectrumNumbers(1, static_cast<int32_t>(filtered.size()));

  return filtered;
}

IndexInfo LoadEventNexusIndexSetup::makeIndexInfo(
    const std::vector<std::string> &bankNames) {
  const auto &componentInfo = m_instrumentWorkspace->componentInfo();
  std::vector<SpectrumDefinition> spectrumDefinitions;
  const auto &instrument = m_instrumentWorkspace->getInstrument();
  for (const auto &bankName : bankNames) {
    const auto &bank = instrument->getComponentByName(bankName);
    std::vector<size_t> dets;
    if (bank) {
      const auto bankIndex = componentInfo.indexOf(bank->getComponentID());
      dets = componentInfo.detectorsInSubtree(bankIndex);
      for (const auto detIndex : dets)
        spectrumDefinitions.emplace_back(detIndex);
    }
    if (dets.empty())
      throw std::runtime_error("Could not find the bank named '" + bankName +
                               "' as a component assembly in the instrument "
                               "tree; or it did not contain any detectors. Try "
                               "unchecking SingleBankPixelsOnly.");
  }
  Indexing::IndexInfo indexInfo(spectrumDefinitions.size());
  indexInfo.setSpectrumDefinitions(std::move(spectrumDefinitions));
  return indexInfo;
}

IndexInfo LoadEventNexusIndexSetup::makeIndexInfo(
    const std::pair<std::vector<int32_t>, std::vector<int32_t>> &
        spectrumDetectorMapping,
    const bool monitorsOnly) {
  const auto &spec = spectrumDetectorMapping.first;
  const auto &udet = spectrumDetectorMapping.second;

  const std::vector<detid_t> monitors =
      m_instrumentWorkspace->getInstrument()->getMonitors();
  const auto &detectorInfo = m_instrumentWorkspace->detectorInfo();
  if (monitorsOnly) {
    std::vector<Indexing::SpectrumNumber> spectrumNumbers;
    std::vector<SpectrumDefinition> spectrumDefinitions;
    // Find the det_ids in the udet array.
    for (const auto id : monitors) {
      // Find the index in the udet array
      auto it = std::find(udet.begin(), udet.end(), id);
      if (it != udet.end()) {
        const specnum_t &specNo = spec[it - udet.begin()];
        spectrumNumbers.emplace_back(specNo);
        spectrumDefinitions.emplace_back(detectorInfo.indexOf(id));
      }
    }
    Indexing::IndexInfo indexInfo(spectrumNumbers);
    indexInfo.setSpectrumDefinitions(std::move(spectrumDefinitions));
    return indexInfo;
  } else {
    SpectrumDetectorMapping mapping(spec, udet, monitors);
    auto uniqueSpectra = mapping.getSpectrumNumbers();
    std::vector<SpectrumDefinition> spectrumDefinitions;
    for (const auto spec : uniqueSpectra) {
      spectrumDefinitions.emplace_back();
      for (const auto detID : mapping.getDetectorIDsForSpectrumNo(spec)) {
        try {
          spectrumDefinitions.back().add(detectorInfo.indexOf(detID));
        } catch (std::out_of_range &e) {
          // Discarding detector IDs that do not exist in the instrument.
        }
      }
    }
    Indexing::IndexInfo indexInfo(std::vector<Indexing::SpectrumNumber>(
        uniqueSpectra.begin(), uniqueSpectra.end()));
    indexInfo.setSpectrumDefinitions(std::move(spectrumDefinitions));
    return filterIndexInfo(indexInfo);
  }
}

/** Filter IndexInfo based on optional spectrum range/list provided.
 *
 * Checks the validity of user provided spectrum range/list. This method assumes
 * that spectrum numbers in `indexInfo` argument are sorted and that the
 * Parallel::StorageMode of `indexInfo` is `Cloned`. */
IndexInfo
LoadEventNexusIndexSetup::filterIndexInfo(const IndexInfo &indexInfo) {
  // Check if range [SpectrumMin, SpectrumMax] was supplied
  if (m_min != EMPTY_INT() || m_max != EMPTY_INT()) {
    if (m_max == EMPTY_INT())
      m_max =
          static_cast<int32_t>(indexInfo.spectrumNumber(indexInfo.size() - 1));
    if (m_min == EMPTY_INT())
      m_min = static_cast<int32_t>(indexInfo.spectrumNumber(0));
    // Avoid adding non-existing indices (can happen if instrument has gaps in
    // its detector IDs). IndexInfo does the filtering for use.
    const auto indices = indexInfo.makeIndexSet(m_min, m_max);
    for (const auto &index : indices)
      m_range.push_back(static_cast<int32_t>(indexInfo.spectrumNumber(index)));
  }
  // Check if SpectrumList was supplied (or filled via min/max above)
  if (!m_range.empty()) {
    const auto indices = indexInfo.makeIndexSet(
        std::vector<Indexing::SpectrumNumber>(m_range.begin(), m_range.end()));
    m_min = static_cast<int32_t>(indexInfo.spectrumNumber(*indices.begin()));
    m_max =
        static_cast<int32_t>(indexInfo.spectrumNumber(*(indices.end() - 1)));
    return Indexing::extract(indexInfo, indices);
  }
  return indexInfo;
}

} // namespace DataHandling
} // namespace Mantid
