#include "MantidAlgorithms/CalculateDIFC.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
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
};

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDIFC::category() const {
  return "Diffraction";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDIFC::summary() const {
  return "Calculate the DIFC for every pixel";
};

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
  size_t numspec = m_inputWS->getNumberHistograms();

  m_outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numspec, 1, 1));
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(m_inputWS, m_outputWS,
                                                         false);
  return;
}

void CalculateDIFC::calculate() {
  Instrument_const_sptr instrument = m_inputWS->getInstrument();

  double l1;
  Kernel::V3D beamline, samplePos;
  double beamline_norm;
  instrument->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  const size_t NUM_SPEC = m_outputWS->getNumberHistograms();
  for (size_t i = 0; i < NUM_SPEC; ++i) {
    Geometry::IDetector_const_sptr det = m_inputWS->getDetector(i);

    double offset = 0.;
    if (m_offsetsWS)
      offset = m_offsetsWS->getValue(det->getID(), 0.);

    double difc = Geometry::Instrument::calcConversion(l1, beamline, beamline_norm,
                                               samplePos, det, offset);
    difc = 1./difc; // calcConversion gives 1/DIFC

    m_outputWS->dataX(i)[0] = static_cast<double>(i);
    m_outputWS->dataY(i)[0] = difc;
  }

}

} // namespace Algorithms
} // namespace Mantid
