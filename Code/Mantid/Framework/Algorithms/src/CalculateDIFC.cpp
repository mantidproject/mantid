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
/** Constructor
 */
CalculateDIFC::CalculateDIFC() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CalculateDIFC::~CalculateDIFC() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateDIFC::name() const { return "CalculateDIFC"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateDIFC::version() const {
  return 1;
}

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDIFC::category() const {
  return "Diffraction";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDIFC::summary() const {
  return "Calculate the DIFC for every pixel";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDIFC::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
      "Name of the workspace to have DIFC calculated from");
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
      "Workspace containing DIFC for each pixel");
  declareProperty(
      new WorkspaceProperty<OffsetsWorkspace>(
          "OffsetsWorkspace", "", Direction::Input, Mantid::API::PropertyMode::Optional),
      "Optional: A OffsetsWorkspace containing the calibration offsets. Either "
      "this or CalibrationFile needs to be specified.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateDIFC::exec() {
  m_offsetsWS = getProperty("OffsetsWorkspace");

  createOutputWorkspace();

  calculate();

  setProperty("OutputWorkspace", m_outputWS);
}

void CalculateDIFC::createOutputWorkspace() {
  m_inputWS = getProperty("InputWorkspace");

  m_outputWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(SpecialWorkspace2D_sptr(
          new SpecialWorkspace2D(m_inputWS->getInstrument())));
  m_outputWS->setTitle("DIFC workspace");

  return;
}

void CalculateDIFC::calculate() {
  Instrument_const_sptr instrument = m_inputWS->getInstrument();

  SpecialWorkspace2D_sptr localWS =
      boost::dynamic_pointer_cast<SpecialWorkspace2D>(m_outputWS);

  double l1;
  Kernel::V3D beamline, samplePos;
  double beamline_norm;
  instrument->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  // To get all the detector ID's
  detid2det_map allDetectors;
  instrument->getDetectors(allDetectors);

  // Now go through all
  detid2det_map::const_iterator it = allDetectors.begin();
  for (; it != allDetectors.end(); ++it) {
    Geometry::IDetector_const_sptr det = it->second;
    if ((!det->isMasked()) && (!det->isMonitor())) {
      const detid_t detID = it->first;
      double offset = 0.;
      if (m_offsetsWS)
        offset = m_offsetsWS->getValue(detID, 0.);

      double difc = Geometry::Instrument::calcConversion(
          l1, beamline, beamline_norm, samplePos, det, offset);
      difc = 1. / difc; // calcConversion gives 1/DIFC
      localWS->setValue(detID, difc);
    }
  }

}

} // namespace Algorithms
} // namespace Mantid
