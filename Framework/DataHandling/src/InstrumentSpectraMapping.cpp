// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/InstrumentSpectraMapping.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace Mantid::DataHandling {

using namespace API;
using Kernel::Logger;

const std::string SPECTRA_MAP_SOURCE = "spectra-map-source";
const std::string SPECTRA_MAP_FROM_INSTRUMENT = "instrument";

namespace {

/// Whether the instrument definition enables this correction.
bool isCorrectionEnabled(const Geometry::Instrument &instrument) {
  const auto values = instrument.getStringParameter(SPECTRA_MAP_SOURCE);
  return !values.empty() && values.front() == SPECTRA_MAP_FROM_INSTRUMENT;
}

/// Corrects a workspace, or every member of a group of them.
void correctWorkspace(const Workspace_sptr &workspace, Logger &log) {
  if (!workspace)
    return;

  if (const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
    for (int i = 0; i < group->getNumberOfEntries(); ++i)
      correctWorkspace(group->getItem(i), log);
    return;
  }

  if (const auto matrixWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(workspace))
    correctSpectraMapping(*matrixWorkspace, log);
}

} // namespace

bool correctSpectraMapping(MatrixWorkspace &workspace, Logger &log) {
  const auto instrument = workspace.getInstrument();
  if (!instrument || !isCorrectionEnabled(*instrument))
    return false;

  const auto detectorIDs = instrument->getDetectorIDs(false);
  if (detectorIDs.empty())
    return false;
  const std::unordered_set<detid_t> knownIDs(detectorIDs.cbegin(), detectorIDs.cend());

  // A spectrum needs correcting when the file gave it a detector the instrument does not define. It is mapped to the
  // detector of its own number where one exists, and otherwise left with its histogram and no detectors. Spectra whose
  // file detectors are all known are not touched, so a mapping the file got right survives even if it is not
  // one-to-one.
  std::size_t corrected = 0;
  std::size_t unmappable = 0;
  for (std::size_t i = 0; i < workspace.getNumberHistograms(); ++i) {
    auto &spectrum = workspace.getSpectrum(i);
    const auto &fileIDs = spectrum.getDetectorIDs();
    if (std::all_of(fileIDs.cbegin(), fileIDs.cend(), [&knownIDs](const detid_t id) { return knownIDs.count(id) > 0; }))
      continue;

    const auto matchingID = static_cast<detid_t>(spectrum.getSpectrumNo());
    if (knownIDs.count(matchingID)) {
      spectrum.setDetectorID(matchingID);
      ++corrected;
    } else {
      spectrum.clearDetectorIDs();
      ++unmappable;
    }
  }

  if (corrected == 0 && unmappable == 0)
    return false;

  log.warning() << instrument->getName() << ": " << corrected
                << " spectra referenced detectors not in the instrument definition and were mapped to the detector "
                   "of the same ID.\n";
  if (unmappable > 0)
    log.warning() << unmappable << " spectra had no detector of a matching ID and were left without detectors.\n";

  return true;
}

void correctLoadedWorkspaces(API::Algorithm &loader, Logger &log) {
  // A group's members are reached through the group, so the per-period properties need no separate visit.
  for (const auto *name : {"OutputWorkspace", "MonitorWorkspace"}) {
    if (!loader.existsProperty(name))
      continue;
    const Workspace_sptr workspace = loader.getProperty(name);
    correctWorkspace(workspace, log);
  }
}

} // namespace Mantid::DataHandling
