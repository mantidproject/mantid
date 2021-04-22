// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNexusProcessed2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusGeometry/AbstractLogger.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <H5Cpp.h>
namespace Mantid {
namespace DataHandling {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNexusProcessed2)
//----------------------------------------------------------------------------------------------

namespace {
template <typename T> int countEntriesOfType(const T &entry, const std::string &nxClass) {
  int count = 0;
  for (const auto &group : entry.groups()) {
    if (group.nxclass == nxClass)
      ++count;
  }
  return count;
}

template <typename T>
std::vector<Mantid::NeXus::NXClassInfo> findEntriesOfType(const T &entry, const std::string &nxClass) {
  std::vector<Mantid::NeXus::NXClassInfo> result;
  for (const auto &group : entry.groups()) {
    if (group.nxclass == nxClass)
      result.emplace_back(group);
  }
  return result;
}
/**
 * Determine the format/layout of the instrument block. We use this to
 * distinguish between the ESS saving schemes and the Mantid processed nexus
 * schemes
 * @param entry
 * @return
 */
InstrumentLayout instrumentFormat(Mantid::NeXus::NXEntry &entry) {
  auto result = InstrumentLayout::NotRecognised;
  const auto instrumentsCount = countEntriesOfType(entry, "NXinstrument");
  if (instrumentsCount == 1) {
    // Can now assume nexus format
    result = InstrumentLayout::NexusFormat;

    if (entry.containsGroup("instrument")) {
      auto instr = entry.openNXInstrument("instrument");
      if (instr.containsGroup("detector") ||
          (instr.containsGroup("physical_detectors") && instr.containsGroup("physical_monitors"))) {
        result = InstrumentLayout::Mantid; // 1 nxinstrument called instrument,
      }
      instr.close();
    }
    entry.close();
  }
  return result;
}

} // namespace

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadNexusProcessed2::name() const { return "LoadNexusProcessed"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadNexusProcessed2::version() const { return 2; }

void LoadNexusProcessed2::readSpectraToDetectorMapping(Mantid::NeXus::NXEntry &mtd_entry,
                                                       Mantid::API::MatrixWorkspace &ws) {

  m_instrumentLayout = instrumentFormat(mtd_entry);
  if (m_instrumentLayout == InstrumentLayout::Mantid) {
    // Now assign the spectra-detector map
    readInstrumentGroup(mtd_entry, ws);
  } else if (m_instrumentLayout == InstrumentLayout::NexusFormat) {
    extractMappingInfoNew(mtd_entry);
  } else {
    g_log.information() << "Instrument layout not recognised. Spectra mappings not loaded.";
  }
}
void LoadNexusProcessed2::extractMappingInfoNew(Mantid::NeXus::NXEntry &mtd_entry) {
  using namespace Mantid::NeXus;
  auto result = findEntriesOfType(mtd_entry, "NXinstrument");
  if (result.size() != 1) {
    g_log.warning("We are expecting a single NXinstrument. No mappings loaded");
  }
  auto inst = mtd_entry.openNXInstrument(result[0].nxname);

  auto &spectrumNumbers = m_spectrumNumbers;
  auto &detectorIds = m_detectorIds;
  auto &detectorCounts = m_detectorCounts;
  for (const auto &group : inst.groups()) {
    if (group.nxclass == "NXdetector" || group.nxclass == "NXmonitor") {
      NXDetector detgroup = inst.openNXDetector(group.nxname);

      NXInt spectra_block = detgroup.openNXInt("spectra");
      try {
        spectra_block.load();
      } catch (std::runtime_error &) { // Throws if dataset zero-sized
        detgroup.close();
        continue;
      }
      const size_t nSpecEntries = spectra_block.dim0();
      auto data = spectra_block.vecBuffer();
      size_t currentSize = spectrumNumbers.size();
      spectrumNumbers.resize(currentSize + nSpecEntries, 0);
      // Append spectrum numbers
      for (size_t i = 0; i < nSpecEntries; ++i) {
        spectrumNumbers[i + currentSize] = data[i];
      }

      NXInt det_index = detgroup.openNXInt("detector_list");
      det_index.load();
      size_t nDetEntries = det_index.dim0();
      currentSize = detectorIds.size();
      data = det_index.vecBuffer();
      detectorIds.resize(currentSize + nDetEntries, 0);
      for (size_t i = 0; i < nDetEntries; ++i) {
        detectorIds[i + currentSize] = data[i];
      }

      // Load the number of detectors per spectra
      NXInt det_counts = detgroup.openNXInt("detector_count");
      det_counts.load();
      size_t nDetCounts = det_counts.dim0();
      currentSize = detectorCounts.size();
      data = det_counts.vecBuffer();
      detectorCounts.resize(currentSize + nDetCounts, 0);
      size_t dataSum = 0;
      for (size_t i = 0; i < nDetCounts; ++i) {
        const int dataVal = data[i];
        dataSum += dataVal;
        detectorCounts[i + currentSize] = dataVal;
      }

      if (nDetCounts != nSpecEntries) {
        throw std::runtime_error("Bad file. Has different number of entries in "
                                 "spec and detector_count datasets");
      }
      if (dataSum != nDetEntries) {
        throw std::runtime_error("Bad file. detector_counts sum does not match "
                                 "the number of detectors given by number of "
                                 "detector_list entries");
      }

      detgroup.close();
    }
  }
  inst.close();
}

/**
 * Attempt to load nexus geometry. Should fail without exception if not
 * possible.
 *
 * Caveats are:
 * 1. Only works for input files where there is a single NXEntry. Does nothing
 * otherwise.
 * 2. Is only applied after attempted instrument loading in the legacy fashion
 * that happens as part of loadEntry. So you will still get warning+error
 * messages from that even if this succeeds
 *
 * @param ws : Input workspace onto which instrument will get attached
 * @param nWorkspaceEntries : number of entries
 * @param logger : to write to
 * @param filename : filename to load from.
 * @return true if successful
 */
bool LoadNexusProcessed2::loadNexusGeometry(API::Workspace &ws, const int nWorkspaceEntries, Kernel::Logger &logger,
                                            const std::string &filename) {
  if (m_instrumentLayout == InstrumentLayout::NexusFormat && nWorkspaceEntries == 1) {
    if (auto *matrixWs = dynamic_cast<API::MatrixWorkspace *>(&ws)) {
      try {
        using namespace Mantid::NexusGeometry;
        auto instrument =
            NexusGeometry::NexusGeometryParser::createInstrument(filename, NexusGeometry::makeLogger(&logger));
        matrixWs->setInstrument(Geometry::Instrument_const_sptr(std::move(instrument)));

        auto &detInfo = matrixWs->detectorInfo();
        Indexing::IndexInfo info(m_spectrumNumbers);
        std::vector<SpectrumDefinition> definitions;
        definitions.reserve(m_spectrumNumbers.size());
        size_t detCounter = 0;
        for (size_t i = 0; i < m_spectrumNumbers.size(); ++i) {
          // counts gives number of detectors per spectrum
          size_t counts = m_detectorCounts[i];
          SpectrumDefinition def;
          // Add the number of detectors known to be associated with this
          // spectrum
          for (size_t j = 0; j < counts; ++j, ++detCounter) {
            def.add(detInfo.indexOf(m_detectorIds[detCounter]));
          }
          definitions.emplace_back(def);
        }
        info.setSpectrumDefinitions(definitions);
        matrixWs->setIndexInfo(info);
        return true;
      } catch (std::exception &e) {
        logger.warning(e.what());
      } catch (H5::Exception &e) {
        logger.warning(e.getDetailMsg());
      }
    }
  }
  return false;
}

int LoadNexusProcessed2::confidence(Kernel::NexusDescriptor &descriptor) const {
  if (descriptor.pathExists("/mantid_workspace_1"))
    return LoadNexusProcessed::confidence(descriptor) + 1; // incrementally better than v1.
  else
    return 0;
}

} // namespace DataHandling
} // namespace Mantid
