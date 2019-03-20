// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveSpectra.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;

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

void RemoveSpectra::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::string outputWS = getPropertyValue("OutputWorkspace");
  std::vector<size_t> specList = getProperty("SpectrumList");
  auto removeMaskedSpectra = getProperty("RemoveMaskedSpectra");
  auto removeSpectraWithNoDetector = getProperty("RemoveSpectraWithNoDetector");
  auto overwriteExisting = getProperty("overwriteExisting");

  auto ashfaf = 1;
}

// std::vector<std::size_t> discoverMaskedSpectra() {}

// std::vector<std::size_t> discoverSpectraWithNoDetector() {}

} // namespace Algorithms
} // namespace Mantid