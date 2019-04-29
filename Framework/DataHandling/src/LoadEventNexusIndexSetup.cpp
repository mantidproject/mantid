// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/Extract.h"
#include "MantidIndexing/Scatter.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid::API;
using namespace Mantid::Indexing;

namespace Mantid {
namespace DataHandling {

namespace {
void setupConsistentSpectrumNumbers(IndexInfo &filtered,
                                    const std::vector<detid_t> &detIDs) {
  std::vector<Indexing::SpectrumNumber> spectrumNumbers;
  // Temporary spectrum number in `filtered` was detector ID, now translate
  // to spectrum number, starting at 1. Note that we use detIDs and not
  // DetectorInfo for translation since we need to match the unfiltered
  // spectrum numbers, which are based on skipping monitors (which would be
  // included in DetectorInfo).
  for (int32_t i = 0; i < static_cast<int32_t>(detIDs.size()); ++i) {
    if (filtered.spectrumNumber(spectrumNumbers.size()) == detIDs[i])
      spectrumNumbers.push_back(i + 1);
    if (filtered.size() == spectrumNumbers.size())
      break;
  }
  filtered.setSpectrumNumbers(std::move(spectrumNumbers));
}
} // namespace

LoadEventNexusIndexSetup::LoadEventNexusIndexSetup(
    MatrixWorkspace_const_sptr instrumentWorkspace, const int32_t min,
    const int32_t max, const std::vector<int32_t> range,
    const Parallel::Communicator &communicator)
    : m_instrumentWorkspace(instrumentWorkspace), m_min(min), m_max(max),
      m_range(range), m_communicator(communicator) {}

std::pair<int32_t, int32_t> LoadEventNexusIndexSetup::eventIDLimits() const {
  return {m_min, m_max};
}

IndexInfo LoadEventNexusIndexSetup::makeIndexInfo() {
  // The default 1:1 will suffice but exclude the monitors as they are always in
  // a separate workspace
  auto detIDs = m_instrumentWorkspace->getInstrument()->getDetectorIDs(true);
  const auto &detectorInfo = m_instrumentWorkspace->detectorInfo();
  std::vector<SpectrumDefinition> specDefs;
  specDefs.reserve(detIDs.size());
  for (const auto detID : detIDs)
    specDefs.emplace_back(detectorInfo.indexOf(detID));
  // We need to filter based on detector IDs, but use IndexInfo for filtering
  // for a unified filtering mechanism. Thus we set detector IDs as (temporary)
  // spectrum numbers.
  IndexInfo indexInfo(std::vector<SpectrumNumber>(detIDs.begin(), detIDs.end()),
                      Parallel::StorageMode::Cloned, m_communicator);
  indexInfo.setSpectrumDefinitions(specDefs);

  auto filtered = filterIndexInfo(indexInfo);

  // Spectrum numbers are continuous and start at 1. If there is a filter,
  // spectrum numbers are set up to be consistent with the unfiltered case.
  if (filtered.size() == indexInfo.size()) {
    filtered.setSpectrumNumbers(1, static_cast<int32_t>(filtered.size()));
  } else {
    setupConsistentSpectrumNumbers(filtered, detIDs);
  }

  return scatter(filtered);
}

IndexInfo LoadEventNexusIndexSetup::makeIndexInfo(
    const std::vector<std::string> &bankNames) {
  const auto &componentInfo = m_instrumentWorkspace->componentInfo();
  const auto &detectorInfo = m_instrumentWorkspace->detectorInfo();
  std::vector<SpectrumDefinition> spectrumDefinitions;
  // Temporary spectrum numbers setup up to be detector IDs, used for finding
  // correct spectrum number to be consistent with unfiltered case.
  std::vector<SpectrumNumber> spectrumNumbers;
  const auto &instrument = m_instrumentWorkspace->getInstrument();
  for (const auto &bankName : bankNames) {
    const auto &bank = instrument->getComponentByName(bankName);
    std::vector<size_t> dets;
    if (bank) {
      const auto bankIndex = componentInfo.indexOf(bank->getComponentID());
      dets = componentInfo.detectorsInSubtree(bankIndex);
      for (const auto detIndex : dets) {
        spectrumDefinitions.emplace_back(detIndex);
        spectrumNumbers.emplace_back(detectorInfo.detectorIDs()[detIndex]);
      }
    }
    if (dets.empty())
      throw std::runtime_error("Could not find the bank named '" + bankName +
                               "' as a component assembly in the instrument "
                               "tree; or it did not contain any detectors. Try "
                               "unchecking SingleBankPixelsOnly.");
  }
  Indexing::IndexInfo indexInfo(std::move(spectrumNumbers),
                                Parallel::StorageMode::Cloned, m_communicator);
  indexInfo.setSpectrumDefinitions(std::move(spectrumDefinitions));
  setupConsistentSpectrumNumbers(indexInfo, instrument->getDetectorIDs(true));
  // Filters are ignored when selecting bank names. Reset min/max to avoid
  // unintended dropping of events in the loader.
  m_min = EMPTY_INT();
  m_max = EMPTY_INT();
  return scatter(indexInfo);
}

IndexInfo LoadEventNexusIndexSetup::makeIndexInfo(
    const std::pair<std::vector<int32_t>, std::vector<int32_t>>
        &spectrumDetectorMapping,
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
    Indexing::IndexInfo indexInfo(
        spectrumNumbers, Parallel::StorageMode::Cloned, m_communicator);
    indexInfo.setSpectrumDefinitions(std::move(spectrumDefinitions));
    return scatter(indexInfo);
  } else {
    SpectrumDetectorMapping mapping(spec, udet, monitors);
    auto uniqueSpectra = mapping.getSpectrumNumbers();
    std::vector<SpectrumDefinition> spectrumDefinitions;
    for (const auto specNo : uniqueSpectra) {
      spectrumDefinitions.emplace_back();
      for (const auto detID : mapping.getDetectorIDsForSpectrumNo(specNo)) {
        try {
          spectrumDefinitions.back().add(detectorInfo.indexOf(detID));
        } catch (std::out_of_range &) {
          // Discarding detector IDs that do not exist in the instrument.
        }
      }
    }
    Indexing::IndexInfo indexInfo(
        std::vector<Indexing::SpectrumNumber>(uniqueSpectra.begin(),
                                              uniqueSpectra.end()),
        Parallel::StorageMode::Cloned, m_communicator);
    indexInfo.setSpectrumDefinitions(std::move(spectrumDefinitions));
    return scatter(filterIndexInfo(indexInfo));
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
    const auto indices = indexInfo.makeIndexSet(
        static_cast<SpectrumNumber>(m_min), static_cast<SpectrumNumber>(m_max));
    for (const auto index : indices)
      m_range.push_back(static_cast<int32_t>(indexInfo.spectrumNumber(index)));
  }
  // Check if SpectrumList was supplied (or filled via min/max above)
  if (!m_range.empty()) {
    std::sort(m_range.begin(), m_range.end());
    const auto indices = indexInfo.makeIndexSet(
        std::vector<SpectrumNumber>(m_range.begin(), m_range.end()));
    m_min = static_cast<int32_t>(indexInfo.spectrumNumber(*indices.begin()));
    m_max =
        static_cast<int32_t>(indexInfo.spectrumNumber(*(indices.end() - 1)));
    return extract(indexInfo, indices);
  }
  return indexInfo;
}

} // namespace DataHandling
} // namespace Mantid
