#include "MantidAlgorithms/ExtractMask.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractMask)

using Kernel::Direction;
using Geometry::IDetector_const_sptr;
using namespace API;
using namespace Kernel;

/**
 * Declare the algorithm properties
 */
void ExtractMask::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "A workspace whose masking is to be extracted");
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "",
                                                      Direction::Output),
      "A workspace containing the masked spectra as zeroes and ones.");

  declareProperty(make_unique<ArrayProperty<detid_t>>(
                      "DetectorList", boost::make_shared<NullValidator>(),
                      Direction::Output),
                  "A comma separated list or array containing a list of masked "
                  "detector ID's");
}

/**
 * Execute the algorithm
 */
void ExtractMask::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // convert input to a mask workspace
  auto inputMaskWS =
      boost::dynamic_pointer_cast<const DataObjects::MaskWorkspace>(inputWS);
  bool inputWSIsSpecial = bool(inputMaskWS);
  if (inputWSIsSpecial) {
    g_log.notice() << "Input workspace is a MaskWorkspace.\n";
  }

  // List masked of detector IDs
  std::vector<detid_t> detectorList;

  // Create a new workspace for the results, copy from the input to ensure
  // that we copy over the instrument and current masking
  auto maskWS = boost::make_shared<DataObjects::MaskWorkspace>(inputWS);
  maskWS->setTitle(inputWS->getTitle());

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const int64_t nHist = static_cast<int64_t>(inputWS->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, nHist);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *maskWS))
  for (int64_t i = 0; i < nHist; ++i) {
    PARALLEL_START_INTERUPT_REGION
    bool inputIsMasked(false);
    if (spectrumInfo.hasDetectors(i)) {
      // special workspaces can mysteriously have the mask bit set
      inputIsMasked = (inputWSIsSpecial && inputMaskWS->isMaskedIndex(i)) ||
                      spectrumInfo.isMasked(i);
      if (inputIsMasked) {
        detid_t id = spectrumInfo.detector(i).getID();
        PARALLEL_CRITICAL(name) { detectorList.push_back(id); }
      }
    }
    maskWS->setMaskedIndex(i, inputIsMasked);
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.information() << maskWS->getNumberMasked() << " spectra are masked\n";
  g_log.information() << detectorList.size() << " detectors are masked\n";
  setProperty("OutputWorkspace", maskWS);
  setProperty("DetectorList", detectorList);
}
}
}
