// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/ArrayProperty.h"
#include <numeric>
#include <set>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors)

using namespace Kernel;
using namespace API;

void GroupDetectors::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "Workspace", "", Direction::InOut,
          boost::make_shared<CommonBinsValidator>()),
      "The name of the workspace2D on which to perform the algorithm");

  declareProperty(
      std::make_unique<ArrayProperty<specnum_t>>("SpectraList"),
      "An array containing a list of the indexes of the spectra to combine\n"
      "(DetectorList and WorkspaceIndexList are ignored if this is set)");

  declareProperty(
      std::make_unique<ArrayProperty<detid_t>>("DetectorList"),
      "An array of detector ID's (WorkspaceIndexList is ignored if this is\n"
      "set)");

  declareProperty(std::make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "An array of workspace indices to combine");

  declareProperty("ResultIndex", -1,
                  "The workspace index of the summed spectrum (or -1 on error)",
                  Direction::Output);
}

void GroupDetectors::exec() {
  // Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("Workspace");

  std::vector<size_t> indexList = getProperty("WorkspaceIndexList");
  std::vector<specnum_t> spectraList = getProperty("SpectraList");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if (indexList.empty() && spectraList.empty() && detectorList.empty()) {
    g_log.information(name() + ": WorkspaceIndexList, SpectraList, and "
                               "DetectorList properties are all empty, no "
                               "grouping done");
    return;
  }

  // If the spectraList property has been set, need to loop over the workspace
  // looking for the appropriate spectra number and adding the indices they
  // are linked to the list to be processed
  if (!spectraList.empty()) {
    indexList = WS->getIndicesFromSpectra(spectraList);
  } // End dealing with spectraList
  else if (!detectorList.empty()) {
    // Dealing with DetectorList
    // convert from detectors to workspace indices
    indexList = WS->getIndicesFromDetectorIDs(detectorList);
  }

  if (indexList.empty()) {
    g_log.warning("Nothing to group");
    return;
  }

  const auto firstIndex = static_cast<specnum_t>(indexList[0]);
  auto &firstSpectrum = WS->getSpectrum(firstIndex);
  setProperty("ResultIndex", firstIndex);

  // loop over the spectra to group
  Progress progress(this, 0.0, 1.0, static_cast<int>(indexList.size() - 1));

  auto outputHisto = firstSpectrum.histogram();

  for (size_t i = 0; i < indexList.size() - 1; ++i) {
    // The current spectrum
    const size_t currentIndex = indexList[i + 1];
    auto &spec = WS->getSpectrum(currentIndex);

    // Add the current detector to belong to the first spectrum
    firstSpectrum.addDetectorIDs(spec.getDetectorIDs());

    // Add up all the Y spectra and store the result in the first one
    outputHisto += spec.histogram();

    // Now zero the now redundant spectrum and set its spectraNo to indicate
    // this (using -1)
    spec.clearData();
    spec.setSpectrumNo(-1);
    spec.clearDetectorIDs();
    progress.report();
  }

  firstSpectrum.setHistogram(outputHisto);
}

} // namespace DataHandling
} // namespace Mantid
