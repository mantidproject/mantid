#include "MantidAlgorithms/GravityCorrection.h"

#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/make_unique.h"

#include <cmath>
#include <utility>

using Mantid::API::HistogramValidator;
using Mantid::API::InstrumentValidator;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::SpectrumInfo;
using Mantid::API::WorkspaceProperty;
using Mantid::API::WorkspaceUnitValidator;
using Mantid::Geometry::IComponent_const_sptr;
using Mantid::Geometry::IComponent_sptr;
using Mantid::Geometry::ParameterMap_sptr;
using Mantid::Geometry::PointingAlong;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::Exception::InstrumentDefinitionError;
using Mantid::Kernel::make_unique;
using Mantid::Kernel::V3D;
using Mantid::PhysicalConstants::g;

using boost::make_shared;

using std::logic_error;
using std::map;
using std::pair;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::tie;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GravityCorrection)

void GravityCorrection::init() {
  auto wsValidator = make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  this->declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       wsValidator),
      "The name of the input Workspace2D. X and Y values must be "
      "TOF and counts, respectively.");
  this->declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                         Direction::Output),
                        "The name of the output Workspace2D");
  this->declareProperty("FirstSlitName", "slit1",
                        "Component name of the first slit.");
  this->declareProperty("SecondSlitName", "slit2",
                        "Component name of the second slit.");
}

/**
 * @brief GravityCorrection::validateInputs of InputWorkspace, FirstSlitName,
 * SecondSlitName
 * @return a string map containing the error messages
 */
map<string, string> GravityCorrection::validateInputs() {
  map<string, string> result;
  m_ws = this->getProperty("InputWorkspace");
  if (!m_ws)
    result["InputWorkspace"] = "InputWorkspace not defined.";
  const string slit1Name = this->getProperty("FirstSlitName");
  const string slit2Name = this->getProperty("SecondSlitName");
  // Check slit component existance
  // get a pointer to the instrument of the input workspace
  m_originalInstrument = m_ws->getInstrument();
  IComponent_const_sptr slit1 =
      m_originalInstrument->getComponentByName(slit1Name);
  if (!slit1)
    result["FirstSlitName"] =
        "Instrument component with name " + slit1Name + " does not exist.";
  IComponent_const_sptr slit2 =
      m_originalInstrument->getComponentByName(slit2Name);
  if (!slit2)
    result["SecondSlitName"] =
        "Instrument component with name " + slit2Name + " does not exist.";
  return result;
}

/**
 * @brief GravityCorrection::coordinate
 * @param componentName :: name of the instrument component
 * @param direction :: integer that specifies a direction (X=0, Y=1, Z=2)
 * @return coordinate of the instrument component
 */
double GravityCorrection::coordinate(const std::string componentName,
                                     const PointingAlong axis) const {
  IComponent_const_sptr component =
      m_originalInstrument->getComponentByName(componentName);
  double position{0.};
  switch (axis) {
  case Mantid::Geometry::X:
    position = component->getPos().X();
    break;
  case Mantid::Geometry::Y:
    position = component->getPos().Y();
    break;
  case Mantid::Geometry::Z:
    position = component->getPos().Z();
    break;
  default:
    throw runtime_error("Axis is not X/Y/Z");
    break;
  }
  return position;
}

/**
 * @brief slitCheck
 * @return true if slit position is between source and sample
 */
void GravityCorrection::slitCheck() {
  string sourceName = m_originalInstrument->getSource()->getName();
  string sampleName = m_originalInstrument->getSample()->getName();
  const string slit1{this->getPropertyValue("FirstSlitName")};
  const string slit2{this->getPropertyValue("SecondSlitName")};
  // in beam directions
  const double sourceD = coordinate(sourceName, m_beamDirection);
  const double sampleD = coordinate(sampleName, m_beamDirection);
  const double slit2D = coordinate(slit2, m_beamDirection);
  const double slit1D = coordinate(slit1, m_beamDirection);
  // Slits must be located between source and sample
  if (sourceD < sampleD) {
    // slit 1 should be the next component after source in beam direction
    if (slit2D < slit1D) {
      m_slit1Name = slit2;
      m_slit2Name = slit1;
    }
    if ((slit1D < sourceD) && (slit1D > sampleD))
      throw InstrumentDefinitionError("Slit " + m_slit1Name +
                                      " position is incorrect.");
  } else {
    // slit 1 should be the next component after source in beam direction
    if (slit2D > slit1D) {
      m_slit1Name = slit2;
      m_slit2Name = slit1;
    }
    if ((slit1D > sourceD) && (slit1D < sampleD))
      throw InstrumentDefinitionError("Slit " + m_slit2Name +
                                      " position is incorrect.");
  }
  if (m_slit1Name.empty())
    m_slit1Name = slit1;
  if (m_slit2Name.empty())
    m_slit2Name = slit2;
}

/**
 * @brief GravityCorrection::parabola
 * @return a pair defining the shift in the beam and the upward pointing
 * direction of the parabola from source to sample via the slits
 */
pair<double, double> GravityCorrection::parabola(const double k) {
  const double beam1 = coordinate(m_slit1Name, m_beamDirection);
  const double beam2 = coordinate(m_slit2Name, m_beamDirection);
  const double up1 = coordinate(m_slit1Name, m_upDirection);
  const double up2 = coordinate(m_slit2Name, m_upDirection);
  double beamShift = (k * (pow(beam1, 2) - pow(beam2, 2)) + (up1 - up2)) /
                     (2 * k * (beam1 - beam2));
  double upShift = up1 + k * (beam1 - beam2);
  return pair<double, double>(beamShift, upShift);
}

double GravityCorrection::finalAngle(const double k) {
  double beamShift, upShift;
  tie(beamShift, upShift) = this->GravityCorrection::parabola(k);
  return atan(2. * k * sqrt(upShift / k));
}

/**
 * @brief GravityCorrection::detectorEquation y = ya + ma x. Detector must be a
 * line in xy or zy plane
 * @param det1 :: a pointer to the detectorInfo
 * @return pair y_a, m_a
 * @exception InstrumentDefinitionError if
 */
pair<double, double>
GravityCorrection::detector2DEquation(SpectrumInfo &spectrumInfo) const {
  double ma(0.0), ya(0.0);
  const auto &det1 = spectrumInfo.detector(0);
  const auto rdet1 = dynamic_cast<const Geometry::Detector *>(&det1);
  const auto &det2 = spectrumInfo.detector(1);
  const auto rdet2 = dynamic_cast<const Geometry::Detector *>(&det2);
  string det1Name = rdet1->getFullName();
  string det2Name = rdet2->getFullName();

  if (det1Name.empty())
      g_log.error("Do not know the name of the first detector");
  if (det2Name.empty())
      g_log.error("Do not know the name of the second detector");

  const double z1 = coordinate(det1Name, m_beamDirection);
  const double z2 = coordinate(det2Name, m_beamDirection);
  const double y1 = coordinate(det1Name, m_upDirection);
  const double y2 = coordinate(det2Name, m_upDirection);

  // depending on beam position
  if (z1 && z2 && y1 && y2) {
    ma = (y2 - y1) / (z2 - z1);
    ya = y2 - ma * z2;
  } else
    throw InstrumentDefinitionError("Detector ");
  // check with a further point if the detector can be modeled as line
  return pair<double, double>(ya, ma);
}

/**
 * @brief GravityCorrection::virtualInstrument defines a virtual instrument with
 * the sample at its origin x = y = z = 0 m. The original instrument and its
 * parameter map will be copied.
 * @param spectrumInfo :: reference to the SpectrumInfo of the InputWorkspace
 */
void GravityCorrection::virtualInstrument(SpectrumInfo &spectrumInfo) {
  if (m_originalInstrument->isParametrized()) {
    ParameterMap_sptr parMap = m_originalInstrument->getParameterMap();
    m_virtualInstrument =
        make_shared<Geometry::Instrument>(m_originalInstrument, parMap);
    V3D samplePos = spectrumInfo.samplePosition();
    if (samplePos.distance(V3D(0.0, 0.0, 0.0))) {
      // move instrument to ensure sample at position x = y = z = 0 m,
      samplePos *= -1.0;
      // parameter map holds parameters of all modified IComponent_const_sptr's:
      auto sample = m_originalInstrument->getSample();
      parMap->addV3D(sample.get(), sample->getName(), V3D(0.0, 0.0, 0.0));
      auto source = m_originalInstrument->getSource();
      parMap->addV3D(source.get(), source->getName(),
                     source->getPos() + samplePos);
      auto slit1 = m_originalInstrument->getComponentByName(m_slit1Name);
      parMap->addV3D(slit1.get(), slit1->getName(),
                     slit1->getPos() + samplePos);
      auto slit2 = m_originalInstrument->getComponentByName(m_slit2Name);
      parMap->addV3D(slit2.get(), slit2->getName(),
                     slit2->getPos() + samplePos);
      for (size_t i = 0; i < spectrumInfo.size(); ++i) {
        const auto &det = spectrumInfo.detector(i);
        const auto rdet = dynamic_cast<const Geometry::Detector *>(&det);
        parMap->addV3D(rdet, rdet->getName(),
                       spectrumInfo.position(i) + samplePos);
      }
    }
    if (m_virtualInstrument->isEmptyInstrument()) {
      string debugMessage1 = "Cannot create a virtual instrument.";
      g_log.debug(debugMessage1);
    }
  } else {
    string debugMessage2 =
        "Instrument of the InputWorkspace is not parametrised";
    g_log.error(debugMessage2);
  }
}

void GravityCorrection::exec() {
  // Progress reports & cancellation
  m_progress = make_unique<Progress>(this, 0.0, 1.0, 3); // or size() ?

  const auto refFrame = m_originalInstrument->getReferenceFrame();
  m_upDirection = refFrame->pointingUp();
  m_beamDirection = refFrame->pointingAlongBeam();
  m_horizontalDirection = refFrame->pointingHorizontal();
  m_progress->report("Check slits ...");
  this->slitCheck();

  MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");
  outWS = m_ws->clone();

  auto spectrumInfo = m_ws->spectrumInfo();
  m_progress->report("Create virtual instrument ...");
  this->virtualInstrument(spectrumInfo);

  if (m_virtualInstrument->getSample()->getPos() != V3D(0.0, 0.0, 0.0))
    runtime_error("Not moved");
  if (m_originalInstrument->getSample()->getPos() == V3D(0.0, 0.0, 0.0))
    runtime_error("Moved");

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {

    stringstream istr;
    istr << static_cast<int>(i);

    // Always ignore monitors and ignore masked detectors if requested.
    bool masked = m_keepMask && spectrumInfo.isMasked(i);
    if (spectrumInfo.isMonitor(i))
      g_log.debug("Ignore monitor spectrum " + i);
    if (masked)
      g_log.debug("Ignore masked spectrum " + i);

    // get spectrum
    // const auto &spectrum = m_ws->getSpectrum(i);
    // get detector IDs for this spectrum
    // std::set<detid_t> detids = spectrum.getDetectorIDs();

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.error("No detector(s) found for spectrum " + i);
      runtime_error("No detectors found for spectrum " + i);
    }

    // take neutrons that hit the detector of spectrum i
    V3D pos = spectrumInfo.position(i);
    V3D dist = spectrumInfo.sourcePosition();
    double s = dist.distance(pos);
    // double v = std::sqrt( s / t );
    double v{1.0};

    double k = g / (2 * std::pow(v, 2));

    double finalAngle = this->finalAngle(k);
    double ya{0.0}, ma{0.0};

    pair<double, double>(ya, ma) = this->detector2DEquation(spectrumInfo);

    // corrected hit
    double xCorrected = ya / (tan(finalAngle) - ma);
    double yCorrected = xCorrected * tan(finalAngle);

    // is the new detector position valid? Search

    /*
    // can be a group
    Geometry::IDetector_const_sptr det = m_ws->getDetector(i);

    if (det->type() == "DetectorComponent") {
      g_log.debug("Found DetectorComponent for detector " + det->getID());
    } else if (det->type().compare("RectangularDetectorPixel")) {
      g_log.debug("Found RectangularDetectorPixel for detector " +
                  det->getID());

      Geometry::IComponent_const_sptr parent = det->getParent();

      Geometry::RectangularDetector_const_sptr rect =
          boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(
              parent);

      // where does the neutron hit the detector?

      // which detector got hit?
      detid_t detid = rect->getAtXY();
      detid_t detid = rect->getDetectorIDAtXY();
      // where the neutron should hit the detector?
      // which detector the neutron should hit?
      std::pair<int, int>(xRect, yRect) = rect->getXYForDetectorID();
      // move counts and (related error?) from detector that got hit to the
    detector that should be hit
      */
  }

  // not needed
  /*
  std::vector<specnum_t> spectra =
      m_ws->getSpectraFromDetectorIDs(std::vector<detid_t>(*i, det->getID()));

  if (spectra.empty()) {
    throw Kernel::Exception::NotFoundError("Cannot find spectrum number for "
                                           "detector ",
                                           det->getID());
  }*/

  this->setProperty("OutputWorkspace", outWS);
}

// move counts and errors
// counts
// setCounts
// setHistogram

/*
// initial TOF x values
auto tof0 = m_ws->x(0);

// initial final angles (y axis, spectra)
const SpectrumInfo &spectrum = m_ws->spectrumInfo();
std::vector<double> finalAngles(0.0);
for (size_t i = 0; i < m_ws->size(); ++i)
  finalAngles.push_back(spectrum.signedTwoTheta(i));
*/
} // namespace Algorithms
} // namespace Mantid

// EditInstrumentGeometry
// SparseInstrument
// MoveInstrumentComponent
// RotateInstruementComponent
// Clear/Copy/SetInstrumentParameter

/*
int pointNo = 0;
for (const auto i : indices) {
  const specnum_t spectrum = m_spectrumNumbers[i];
  V3D pos = m_spectrumInfo.position(i) / m_scale;
  dataPoints[pointNo][0] = pos.X();
  dataPoints[pointNo][1] = pos.Y();
  dataPoints[pointNo][2] = pos.Z();
  Vertex vertex = boost::add_vertex(spectrum, m_graph);
  pointNoToVertex[pointNo] = vertex;
  m_specToVertex[spectrum] = vertex;
  ++pointNo;
}
*/
