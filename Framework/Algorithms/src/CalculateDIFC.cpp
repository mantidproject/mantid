#include "MantidAlgorithms/CalculateDIFC.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::DataObjects::SpecialWorkspace2D;
using Mantid::DataObjects::SpecialWorkspace2D_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateDIFC)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateDIFC::name() const { return "CalculateDIFC"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateDIFC::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDIFC::category() const {
  return "Diffraction\\Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDIFC::summary() const {
  return "Calculate the DIFC for every pixel";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDIFC::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the workspace to have DIFC calculated from");
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Workspace containing DIFC for each pixel");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
          "OffsetsWorkspace", "", Direction::Input,
          Mantid::API::PropertyMode::Optional),
      "Optional: A OffsetsWorkspace containing the calibration offsets. Either "
      "this or CalibrationFile needs to be specified.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateDIFC::exec() {

  DataObjects::OffsetsWorkspace_sptr offsetsWs =
      getProperty("OffsetsWorkspace");
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  API::MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");

  if ((!bool(inputWs == outputWs)) ||
      (!bool(boost::dynamic_pointer_cast<SpecialWorkspace2D>(outputWs)))) {
    outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        boost::make_shared<SpecialWorkspace2D>(inputWs->getInstrument()));
    outputWs->setTitle("DIFC workspace");
  }

  double l1;
  Kernel::V3D beamline, samplePos;
  double beamlineNorm;

  inputWs->getInstrument()->getInstrumentParameters(l1, beamline, beamlineNorm,
                                                    samplePos);

  const auto &detectorInfo = inputWs->detectorInfo();

  API::Progress progress(this, 0, 1, detectorInfo.size());
  calculate(progress, outputWs, offsetsWs, l1, beamlineNorm, beamline,
            samplePos, detectorInfo);

  setProperty("OutputWorkspace", outputWs);
}

void CalculateDIFC::calculate(API::Progress &progress,
                              API::MatrixWorkspace_sptr &outputWs,
                              DataObjects::OffsetsWorkspace_sptr &offsetsWS,
                              double l1, double beamlineNorm,
                              Kernel::V3D &beamline, Kernel::V3D &samplePos,
                              const API::DetectorInfo &detectorInfo) {
  SpecialWorkspace2D_sptr localWS =
      boost::dynamic_pointer_cast<SpecialWorkspace2D>(outputWs);

  const auto &detectorIDs = detectorInfo.detectorIDs();

  // Now go through all
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if ((!detectorInfo.isMasked(i)) && (!detectorInfo.isMonitor(i))) {
      double offset = 0.;
      if (offsetsWS)
        offset = offsetsWS->getValue(detectorIDs[i], 0.);

      double difc = Geometry::Instrument::calcConversion(
          l1, beamline, beamlineNorm, samplePos, detectorInfo.position(i),
          offset);
      difc = 1. / difc; // calcConversion gives 1/DIFC
      localWS->setValue(detectorIDs[i], difc);
    }

    progress.report("Calculate DIFC");
  }
}

} // namespace Algorithms
} // namespace Mantid
