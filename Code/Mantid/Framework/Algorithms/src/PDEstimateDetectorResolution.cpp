//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PDEstimateDetectorResolution.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"

#include <math.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(PDEstimateDetectorResolution)

namespace { // hide these constants
  ///
  const double MICROSEC_TO_SEC=1.0E-6;
  ///
  const double WAVELENGTH_TO_VELOCITY=1.0E-10 *
      PhysicalConstants::h / PhysicalConstants::NeutronMass;
  /// This is an absurd number for even ultra cold neutrons
  const double WAVELENGTH_MAX = 1000.;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDEstimateDetectorResolution::PDEstimateDetectorResolution() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDEstimateDetectorResolution::~PDEstimateDetectorResolution() {}

const std::string PDEstimateDetectorResolution::name() const {
  return "PDEstimateDetectorResolution";
}

const std::string PDEstimateDetectorResolution::alias() const {
  return "EstimatePDDetectorResolution";
}

const std::string PDEstimateDetectorResolution::summary() const {
  return "Estimate the resolution of each detector for a powder "
         "diffractometer. ";
}

int PDEstimateDetectorResolution::version() const { return 1; }

const std::string PDEstimateDetectorResolution::category() const {
  return "Diffraction";
}

//----------------------------------------------------------------------------------------------
void PDEstimateDetectorResolution::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                             Direction::Input),
      "Name of the workspace to have detector resolution calculated. ");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of the output workspace containing delta(d)/d of each "
                  "detector/spectrum.");

  auto positiveDeltaTOF = boost::make_shared<BoundedValidator<double> >();
  positiveDeltaTOF->setLower(0.);
  positiveDeltaTOF->setLowerExclusive(true);
  declareProperty(
      "DeltaTOF", 0., positiveDeltaTOF,
      "DeltaT as the resolution of TOF with unit microsecond (10^-6m).");

  auto positiveWavelength = boost::make_shared<BoundedValidator<double> >();
  positiveWavelength->setLower(0.);
  positiveWavelength->setLowerExclusive(true);
  declareProperty(
      "Wavelength", EMPTY_DBL(), positiveWavelength,
      "Wavelength setting in Angstroms. This overrides what is in the dataset.");
}

//----------------------------------------------------------------------------------------------
/**
  */
void PDEstimateDetectorResolution::exec() {
  processAlgProperties();

  retrieveInstrumentParameters();

  createOutputWorkspace();

  estimateDetectorResolution();

  setProperty("OutputWorkspace", m_outputWS);
}

//----------------------------------------------------------------------------------------------
/**
  */
void PDEstimateDetectorResolution::processAlgProperties() {
  m_inputWS = getProperty("InputWorkspace");

  m_deltaT = getProperty("DeltaTOF");
  m_deltaT *= MICROSEC_TO_SEC; // convert to meter
}

double PDEstimateDetectorResolution::getWavelength() {
  double wavelength = getProperty("Wavelength");
  if (!isEmpty(wavelength))
  {
    return wavelength;
  }

  Property *cwlproperty = m_inputWS->run().getProperty("LambdaRequest");
  if (!cwlproperty)
    throw runtime_error(
        "Unable to locate property LambdaRequest as central wavelength. ");

  TimeSeriesProperty<double> *cwltimeseries =
      dynamic_cast<TimeSeriesProperty<double> *>(cwlproperty);

  if (!cwltimeseries)
    throw runtime_error(
        "LambdaReqeust is not a TimeSeriesProperty in double. ");

  string unit = cwltimeseries->units();
  if (unit.compare("Angstrom") != 0) {
    throw runtime_error("Unit is not recognized: "+unit);
  }

  return cwltimeseries->timeAverageValue();
}

//----------------------------------------------------------------------------------------------
/**
  */
void PDEstimateDetectorResolution::retrieveInstrumentParameters() {
  double centrewavelength = getWavelength();
  g_log.notice() << "Centre wavelength = " << centrewavelength << "\n";
  if (centrewavelength > WAVELENGTH_MAX)
  {
    throw runtime_error("unphysical wavelength used");
  }

  // Calculate centre neutron velocity
  m_centreVelocity = WAVELENGTH_TO_VELOCITY / centrewavelength;
  g_log.notice() << "Centre neutron velocity = " << m_centreVelocity << "\n";

  // Calculate L1 sample to source
  Instrument_const_sptr instrument = m_inputWS->getInstrument();
  V3D samplepos = instrument->getSample()->getPos();
  V3D sourcepos = instrument->getSource()->getPos();
  m_L1 = samplepos.distance(sourcepos);
  g_log.notice() << "L1 = " << m_L1 << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/**
  */
void PDEstimateDetectorResolution::createOutputWorkspace() {
  size_t numspec = m_inputWS->getNumberHistograms();

  m_outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numspec, 1, 1));
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(m_inputWS, m_outputWS,
                                                         false);
  return;
}
//----------------------------------------------------------------------------------------------
/**
  */
void PDEstimateDetectorResolution::estimateDetectorResolution() {
  Instrument_const_sptr instrument = m_inputWS->getInstrument();
  V3D samplepos = instrument->getSample()->getPos();

  size_t numspec = m_inputWS->getNumberHistograms();

  double mintwotheta = 10000;
  double maxtwotheta = 0;

  double mint3 = 1;
  double maxt3 = 0;

  size_t count_nodetsize = 0;

  for (size_t i = 0; i < numspec; ++i) {
    // Get detector
    IDetector_const_sptr det = m_inputWS->getDetector(i);

    double detdim;

    boost::shared_ptr<const Detector> realdet =
        boost::dynamic_pointer_cast<const Detector>(det);
    if (realdet) {
      double dy = realdet->getHeight();
      double dx = realdet->getWidth();
      detdim = sqrt(dx * dx + dy * dy) * 0.5;
    } else {
      // Use detector dimension as 0 as no-information
      detdim = 0;
      ++count_nodetsize;
    }

    // Get the distance from detector to source
    V3D detpos = det->getPos();
    double l2 = detpos.distance(samplepos);
    if (l2 < 0)
      throw runtime_error("L2 is negative");

    // Calculate T
    double centraltof = (m_L1 + l2) / m_centreVelocity;

    // Angle
    double r, twotheta, phi;
    detpos.getSpherical(r, twotheta, phi);
    double theta = (twotheta * 0.5) * M_PI / 180.;

    // double solidangle = m_solidangleWS->readY(i)[0];
    double solidangle = det->solidAngle(samplepos);
    double deltatheta = sqrt(solidangle);

    // Resolution
    double t1 = m_deltaT / centraltof;
    double t2 = detdim / (m_L1 + l2);
    double t3 = deltatheta * (cos(theta) / sin(theta));

    double resolution = sqrt(t1 * t1 + t2 * t2 + t3 * t3);

    m_outputWS->dataX(i)[0] = static_cast<double>(i);
    m_outputWS->dataY(i)[0] = resolution;

    if (twotheta > maxtwotheta)
      maxtwotheta = twotheta;
    else if (twotheta < mintwotheta)
      mintwotheta = twotheta;

    if (fabs(t3) < mint3)
      mint3 = fabs(t3);
    else if (fabs(t3) > maxt3)
      maxt3 = fabs(t3);

    g_log.debug() << det->type() << " " << i << "\t\t" << twotheta
                  << "\t\tdT/T = " << t1 * t1 << "\t\tdL/L = " << t2
                  << "\t\tdTheta*cotTheta = " << t3 << "\n";
  }

  g_log.notice() << "2theta range: " << mintwotheta << ", " << maxtwotheta
                 << "\n";
  g_log.notice() << "t3 range: " << mint3 << ", " << maxt3 << "\n";
  g_log.notice() << "Number of detector having NO size information = "
                 << count_nodetsize << "\n";

  return;
}

} // namespace Algorithms
} // namespace Mantid
