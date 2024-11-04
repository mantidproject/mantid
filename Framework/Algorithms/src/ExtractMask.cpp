// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ExtractMask.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/NullValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractMask)

using Geometry::IDetector_const_sptr;
using Kernel::Direction;
using namespace API;
using namespace Kernel;

/**
 * Declare the algorithm properties
 */
void ExtractMask::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "A workspace whose masking is to be extracted");
  declareProperty("UngroupDetectors", false, "If true, the spectra will be ungrouped and masked individually.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "A workspace containing the masked spectra as zeroes and ones.");

  declareProperty(
      std::make_unique<ArrayProperty<detid_t>>("DetectorList", std::make_shared<NullValidator>(), Direction::Output),
      "A comma separated list or array containing a list of masked "
      "detector ID's");
}

/**
 * Execute the algorithm
 */
void ExtractMask::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // convert input to a mask workspace
  auto inputMaskWS = std::dynamic_pointer_cast<const DataObjects::MaskWorkspace>(inputWS);
  auto inputWSIsSpecial = bool(inputMaskWS);
  if (inputWSIsSpecial) {
    g_log.notice() << "Input workspace is a MaskWorkspace.\n";
  }

  // List masked of detector IDs
  std::vector<detid_t> detectorList;
  const auto &detInfo = inputWS->detectorInfo();
  const auto &detIds = detInfo.detectorIDs();
  for (size_t i = 0; i < detInfo.size(); ++i) {
    if ((inputWSIsSpecial && inputMaskWS->isMasked(detIds[i])) || detInfo.isMasked(i)) {
      detectorList.emplace_back(detIds[i]);
    }
  }

  // Create a new workspace for the results, copy from the input to ensure
  // that we copy over the instrument and current masking
  std::shared_ptr<DataObjects::MaskWorkspace> maskWS;
  if (getProperty("UngroupDetectors"))
    maskWS = std::make_shared<DataObjects::MaskWorkspace>(inputWS->getInstrument());
  else
    maskWS = std::make_shared<DataObjects::MaskWorkspace>(inputWS);

  maskWS->setTitle(inputWS->getTitle());

  // set mask from from detectorList
  for (auto detectorID : detectorList)
    try {
      maskWS->setMasked(detectorID);
    } catch (std::invalid_argument const &) {
      g_log.warning() << "Detector ID = " << detectorID << " is masked but does not exist in any output spectra\n ";
    }

  g_log.information() << maskWS->getNumberMasked() << " spectra are masked\n";
  g_log.information() << detectorList.size() << " detectors are masked\n";
  setProperty("OutputWorkspace", maskWS);
  setProperty("DetectorList", detectorList);
}
} // namespace Mantid::Algorithms
