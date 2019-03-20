// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveSpectra.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;

using VectorSizeT_sptr = std::shared_ptr<std::vector<std::size_t>>

DECLARE_ALGORITHM(RemoveSpectra)

    void RemoveSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
      "InputWorkspace", "", Direction::Input));
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
      "OutputWorkspace", "", Direction::Output));
  declareProperty(
      std::make_unique<ArrayProperty<std::size_t>>("SpectrumList"),
      "A comma-separated list of individual spectra numbers to remove");
  declareProperty("RemoveMaskedSpectra", false,
                  "Whether or not to remove spectra that have been masked from "
                  "the inputworkspace.",
                  Direction::Input);
  declareProperty(
      "RemoveSpectraWithNoDetector", false,
      "Whether or not to remove spectra that have no attached detector.",
      Direction::Input);
  declareProperty("OverwriteExisting", true,
                  "If true any existing workspaces with the output name will be"
                  " overwritten.",
                  Direction::Input);
}

std::map<std::string, std::string> RemoveSpectra::validateInputs() {
  std::map<std::string, std::string> errors;
  bool overwriteExisting = getProperty("OverwriteExisting");
  if (ads.isValid(outputWS) && !overwriteExisting) {
    errors.insert("Cannot overwrite existing output workspace")
  }
}

void RemoveSpectra::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  std::string outputWorkspaceName = getPropertyValue("OutputWorkspace");
  VectorSizeT_sptr specList = getProperty("SpectrumList");
  bool removeMaskedSpectra = getProperty("RemoveMaskedSpectra");
  bool removeSpectraWithNoDetector = getProperty("RemoveSpectraWithNoDetector");
  bool overwriteExisting = getProperty("OverwriteExisting");

  if (specList.empty() && removeMaskedSpectra && removeSpectraWithNoDetector) {
    g_log.warning("Nothing passed to the RemoveSpectra algorithm to remove so "
                  "nothing happened");
    return
  }

  if (removeMaskedSpectra) {
    auto maskedSpectra = discoverMaskedSpectra(inputWS);
    specList->insert(specList->end(), maskedSpectra->begin(),
                     maskedSpectra->end());
  }

  if (removeSpectraWithNoDetector) {
    auto detectorLessSpectra = discoverSpectraWithNoDetector(inputWS);
    specList->insert(specList->end(), detectorLessSpectr->begin(),
                     detectorLessSpectra->end());
  }

  if (specList->empty(){
    g_log.debug("No spectra to delete in RemoveSpectra");
    return
  }

  auto outputWS = copySpectraFromInputToOutput(inputWS, specList);
  
  if (overwriteExisting)
    ADS.addOrReplace(outputWS, outputWorkspaceName);
  else
    ADS.add(outputWS, outputWorkspaceName);

  setProperty("OutputWorkspace", outputWS);
  return outputWS;
}

VectorSizeT_sptr
RemoveSpectra::discoverMaskedSpectra(MatrixWorkspace_const_sptr &inputWS) {}

VectorSizeT_sptr RemoveSpectra::discoverSpectraWithNoDetector(
    MatrixWorkspace_const_sptr &inputWS) {
  VectorSizeT_sptr fun;
  for ()
}

MatrixWorkspace_sptr
RemoveSpectra::copySpectraFromInputToOutput(MatrixWorkspace_const_sptr &inputWS,
                                            VectorSizeT_sptr &specList) {}

} // namespace Algorithms
} // namespace Mantid