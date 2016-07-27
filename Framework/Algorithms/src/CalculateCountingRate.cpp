#include "MantidAlgorithms/CalculateCountingRate.h"

namespace Mantid {
namespace Algorithms {


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateCountingRate)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateCountingRate::name() const { return "CalculateCountingRate"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateCountingRate::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateCountingRate::category() const {
  return "Inelastic\\Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateCountingRate::summary() const {
  return "Calculates instrument counting rate as the function of the experiment time and add appropriate logs to the source workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateCountingRate::init() {
    /*
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
      */
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateCountingRate::exec() {

/*
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

  Instrument_const_sptr instrument = inputWs->getInstrument();

  double l1;
  Kernel::V3D beamline, samplePos;
  double beamlineNorm;

  instrument->getInstrumentParameters(l1, beamline, beamlineNorm, samplePos);

  // To get all the detector ID's
  detid2det_map allDetectors;
  instrument->getDetectors(allDetectors);

  API::Progress progress(this, 0, 1, allDetectors.size());
  calculate(progress, outputWs, offsetsWs, l1, beamlineNorm, beamline,
            samplePos, allDetectors);

  setProperty("OutputWorkspace", outputWs);
  */
}

/*
void CalculateCountingRate::calculate(API::Progress &progress,
                              API::MatrixWorkspace_sptr &outputWs,
                              DataObjects::OffsetsWorkspace_sptr &offsetsWS,
                              double l1, double beamlineNorm,
                              Kernel::V3D &beamline, Kernel::V3D &samplePos,
                              detid2det_map &allDetectors) {
  SpecialWorkspace2D_sptr localWS =
      boost::dynamic_pointer_cast<SpecialWorkspace2D>(outputWs);

  // Now go through all
  detid2det_map::const_iterator it = allDetectors.begin();
  for (; it != allDetectors.end(); ++it) {
    Geometry::IDetector_const_sptr det = it->second;
    if ((!det->isMasked()) && (!det->isMonitor())) {
      const detid_t detID = it->first;
      double offset = 0.;
      if (offsetsWS)
        offset = offsetsWS->getValue(detID, 0.);

      double difc = Geometry::Instrument::calcConversion(
          l1, beamline, beamlineNorm, samplePos, det, offset);
      difc = 1. / difc; // calcConversion gives 1/DIFC
      localWS->setValue(detID, difc);
    }

    progress.report("Calculate DIFC");
  }
 */
}

} // namespace Algorithms
} // namespace Mantid
