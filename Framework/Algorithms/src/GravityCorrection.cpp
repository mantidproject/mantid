#include "MantidAlgorithms/GravityCorrection.h"

#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/HistogramValidator.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Points.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Quat.h"

#include <cmath>
#include <iterator>
#include <memory>
#include <utility>

using Mantid::API::AlgorithmHistories;
using Mantid::API::AlgorithmHistory_const_sptr;
using Mantid::API::HistogramValidator;
using Mantid::API::InstrumentValidator;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::SpectrumInfo;
using Mantid::API::WorkspaceHistory;
using Mantid::API::WorkspaceProperty;
using Mantid::API::WorkspaceUnitValidator;
using Mantid::DataObjects::create;
using Mantid::DataObjects::Workspace2D;
using Mantid::Geometry::ComponentInfo;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::Detector;
using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::IComponent_const_sptr;
using Mantid::Geometry::IComponent_sptr;
using Mantid::Geometry::PointingAlong;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::Points;
using Mantid::Indexing::IndexInfo;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::make_unique;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using Mantid::PhysicalConstants::g;

using boost::const_pointer_cast;
using boost::make_shared;

using std::find_if;
using std::invalid_argument;
using std::logic_error;
using std::map;
using std::pair;
using std::pow;
using std::vector;
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
  // InputWorkspace
  this->m_ws = this->getProperty("InputWorkspace");
  if (this->m_ws == nullptr)
    result["InputWorkspace"] = "InputWorkspace not defined.";
  const WorkspaceHistory history = this->m_ws->getHistory();
  const AlgorithmHistories &histories = history.getAlgorithmHistories();
  // iterator to first occurence of the algorithm name
  const auto it =
      find_if(histories.cbegin(), histories.cend(),
              [this](const auto &i) { return i->name() == this->name(); });
  if (it != histories.end())
    result["InputWorkspace"] = "GravityCorrection did already execute "
                               "(check workspace history).";
  // Slits (name empty? component exists? positions differ?)
  // Get a pointer to the instrument of the input workspace
  const auto instrument = this->m_ws->getInstrument();
  const auto refFrame = instrument->getReferenceFrame();
  m_beamDirection = refFrame->pointingAlongBeam();
  m_upDirection = refFrame->pointingUp();
  m_horizontalDirection = refFrame->pointingHorizontal();
  map<string, string> slits;
  const string slit1 = this->getProperty("FirstSlitName");
  const string slit2 = this->getProperty("SecondSlitName");
  slits["FirstSlitName"] = slit1;
  slits["SecondSlitName"] = slit2;
  double iposition{0.}, last{0.};
  for (auto mapit = slits.begin(); mapit != slits.end(); ++mapit) {
    if (mapit->second.empty())
      result[mapit->first] = "Provide a name for the second slit.";
    IComponent_const_sptr slit = instrument->getComponentByName(mapit->second);
    if (slit == nullptr)
      result[mapit->first] = "Instrument component with name " +
                             (mapit->second) + " does not exist. ";
    else {
      last = this->coordinate(mapit->second, m_beamDirection, instrument);
      iposition += last;
      if ((mapit->first == "SecondSlitName") && (iposition / 2 == last))
        result["SecondSlitName"] = "Position of slits must differ.";
    }
  }
  return result;
}

/**
 * @brief GravityCorrection::coordinate
 * @param componentName :: name of the instrument component
 * @return coordinate of the instrument component in beam direction
 */
double GravityCorrection::coordinate(
    const string componentName, PointingAlong direction,
    Mantid::Geometry::Instrument_const_sptr instrument) const {
  double position{0.};
  IComponent_const_sptr component;
  if (instrument == nullptr)
    component = this->m_virtualInstrument->getComponentByName(componentName);
  else
    component = instrument->getComponentByName(componentName);
  if (component == nullptr)
    this->g_log.error("Cannot get instrument component with name " +
                      componentName);
  else {
    switch (direction) {
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
      g_log.error("Axis is not X/Y/Z");
      throw runtime_error("Axis is not X/Y/Z");
      break;
    }
  }
  return position;
}

double GravityCorrection::coordinate(DetectorInfo &detectorInfo, size_t i,
                                     PointingAlong direction) const {
  // in general, the ReferenceFrame does not change for each other detector
  // a possible solution would be to return the PointingAlong variable
  double position{0.};
  switch (direction) {
  case Mantid::Geometry::X:
    position = detectorInfo.position(i).X();
    break;
  case Mantid::Geometry::Y:
    position = detectorInfo.position(i).Y();
    break;
  case Mantid::Geometry::Z:
    position = detectorInfo.position(i).Z();
    break;
  default:
    g_log.error("Axis is not X/Y/Z");
    throw runtime_error("Axis is not X/Y/Z");
    break;
  }
  return position;
}

double GravityCorrection::coordinate(SpectrumInfo &spectrumInfo, size_t i,
                                     PointingAlong direction) const {
  // in general, the ReferenceFrame does not change for each other detector
  // a possible solution would be to return the PointingAlong variable
  double position{0.};
  switch (direction) {
  case Mantid::Geometry::X:
    position = spectrumInfo.position(i).X();
    break;
  case Mantid::Geometry::Y:
    position = spectrumInfo.position(i).Y();
    break;
  case Mantid::Geometry::Z:
    position = spectrumInfo.position(i).Z();
    break;
  default:
    g_log.error("Axis is not X/Y/Z");
    throw runtime_error("Axis is not X/Y/Z");
    break;
  }
  return position;
}

/**
 * @brief GravityCorrection::setCoordinate
 */
void GravityCorrection::setCoordinate(V3D &pos, PointingAlong direction,
                                      double coor) const {
  switch (direction) {
  case Mantid::Geometry::X:
    pos.setX(pos.X() + coor);
    break;
  case Mantid::Geometry::Y:
    pos.setY(pos.Y() + coor);
    break;
  case Mantid::Geometry::Z:
    pos.setZ(pos.Z() + coor);
    break;
  default:
    g_log.error("Axis is not X/Y/Z");
    throw runtime_error("Axis is not X/Y/Z");
    break;
  }
}

/**
 * @brief GravityCorrection::slitCheck
 * @return true if slit position is between source and sample
 */
void GravityCorrection::slitCheck() {
  const string sourceName = this->m_virtualInstrument->getSource()->getName();
  const string sampleName = this->m_virtualInstrument->getSample()->getName();
  const string slit1{this->getPropertyValue("FirstSlitName")};
  const string slit2{this->getPropertyValue("SecondSlitName")};
  // in beam directions
  const double sourceD = this->coordinate(sourceName, this->m_beamDirection);
  const double sampleD = this->coordinate(sampleName, this->m_beamDirection);
  const double slit1D = this->coordinate(slit1, this->m_beamDirection);
  const double slit2D = this->coordinate(slit2, this->m_beamDirection);
  // Slits must be located between source and sample
  if (sourceD < sampleD) {
    // slit 1 should be the next component after source in beam direction
    if (slit2D < slit1D) {
      this->m_slit1Name = slit2;
      this->m_slit2Name = slit1;
    }
    if ((slit1D < sourceD) && (slit1D > sampleD))
      this->g_log.error("Slit " + this->m_slit1Name +
                        " position is incorrect.");
  } else {
    // slit 1 should be the next component after source in beam direction
    if (slit2D > slit1D) {
      this->m_slit1Name = slit2;
      this->m_slit2Name = slit1;
    }
    if ((slit1D > sourceD) && (slit1D < sampleD))
      this->g_log.error("Position of " + this->m_slit2Name + " is incorrect.");
  }
  if (this->m_slit1Name.empty())
    this->m_slit1Name = slit1;
  if (this->m_slit2Name.empty())
    this->m_slit2Name = slit2;
}

/**
 * @brief GravityCorrection::parabola
 * @param k ::
 * @param i ::
 * @return a pair defining the shift in the beam and the upward pointing
 * direction of the parabola from source to sample via the slits
 */
pair<double, double> GravityCorrection::parabola(const double k, size_t i) {
  const double beam1 =
      this->coordinate(this->m_slit1Name, this->m_beamDirection);
  const double beam2 =
      this->coordinate(this->m_slit2Name, this->m_beamDirection);
  // calculate slit pointing up coordinate
  const double up1 =
      beam1 * tan(this->m_ws->spectrumInfo().signedTwoTheta(i) / 2.);
  const double up2 =
      beam2 * tan(this->m_ws->spectrumInfo().signedTwoTheta(i) / 2.);
  // potential divide by zero avoided by input validation beam1 != beam2
  double beamShift = (k * (pow(beam1, 2) - pow(beam2, 2)) + (up1 - up2)) /
                     (2 * k * (beam1 - beam2));
  if (up1 == 0.)
    g_log.error("Zero final scattering angle.");
  double upShift = up1 + k * (beam1 - beam2);
  return pair<double, double>(beamShift, upShift);
}

/**
 * @brief GravityCorrection::finalAngle computes the update on the final angle
 * taking gravity into account
 * @param k
 * @param i
 * @return a new, corrected final angle
 */
double GravityCorrection::finalAngle(const double k, size_t i) {
  double beamShift, upShift;
  tie(beamShift, upShift) = this->GravityCorrection::parabola(k, i);
  return atan(2. * k * sqrt(std::abs(upShift / k)));
}

/**
 * @brief GravityCorrection::virtualInstrument defines a virtual instrument
 * with
 * the sample at its origin x = y = z = 0 m. The original instrument and its
 * parameter map will be copied.
 */
void GravityCorrection::virtualInstrument() {

  const auto instrument = this->m_ws->getInstrument();

  if (instrument->isParametrized()) {

    auto ws = create<Workspace2D>(
        instrument, this->m_ws->indexInfo().globalSize(), Points(1));

    const V3D &samplePos = instrument->getSample()->getPos();
    const V3D &nullVec = V3D(0., 0., 0.);

    // check if the instrument is rotated: then the up direction and horizontal
    // directions are not zero:
    bool rotated = false;
    const string sourceName{instrument->getSource()->getName()};
    double sourceX =
        this->coordinate(sourceName, m_horizontalDirection, instrument);
    double sourceY = this->coordinate(sourceName, m_upDirection, instrument);
    if (sourceX != 0. || sourceY != 0.)
      rotated = true;

    if (samplePos.distance(nullVec) || rotated) {

      auto &componentInfo = ws->mutableComponentInfo();
      auto &detectorInfo = ws->mutableDetectorInfo();

      const string sampleName{instrument->getSample()->getName()};

      vector<IComponent_const_sptr> comps = {
          instrument->getComponentByName(sourceName),
          instrument->getComponentByName(sampleName),
          instrument->getComponentByName(this->getProperty("FirstSlitName")),
          instrument->getComponentByName(this->getProperty("SecondSlitName"))};

      // translate instrument
      if (samplePos.distance(nullVec) > 1e-10) {
        // move instrument to ensure sample at position x = y = z = 0 m,
        for (vector<IComponent_const_sptr>::iterator compit = comps.begin();
             compit != comps.end(); ++compit) {
          const auto compID1 = (*compit)->getComponentID();
          componentInfo.setPosition(componentInfo.indexOf(compID1),
                                    (*compit)->getPos() - samplePos);
        }
        for (size_t i = 0; i < detectorInfo.size(); ++i)
          detectorInfo.setPosition(i, detectorInfo.position(i) - samplePos);
      }

      // rotate instrument (update positions)
      if (rotated) {
        double tanAngle{0.}; // will hold tan(rotation angle)
        // update: is the instrument rotated? Source up still zero?
        sourceY = this->coordinate(sourceName, m_upDirection, instrument);
        if (sourceY != 0.) {
          // calculate first rotation angle:
          tanAngle = sourceY /
                     this->coordinate(sourceName, m_beamDirection, instrument);
          for (auto compit = comps.begin(); compit != comps.end(); ++compit) {
            const auto compID2 = (*compit)->getComponentID();
            // up direction must now be zero
            V3D position = (*compit)->getPos();
            this->setCoordinate(position, m_upDirection, 0.);
            double coordUp = this->coordinate((*compit)->getName(),
                                              m_upDirection, instrument);
            this->setCoordinate(position, m_beamDirection, tanAngle * coordUp);
            componentInfo.setPosition(componentInfo.indexOf(compID2), position);
          }
          for (size_t i = 0; i < detectorInfo.size(); ++i) {
            // up direction must now be zero
            V3D position = detectorInfo.position(i);
            this->setCoordinate(position, m_upDirection, 0.);
            this->setCoordinate(
                position, m_beamDirection,
                tanAngle * this->coordinate(detectorInfo, i, m_upDirection));
            detectorInfo.setPosition(i, position);
            // rotate detectors
            const V3D &vector =
                instrument->getReferenceFrame()->vecPointingUp();
            const Quat &rot = Quat(atan(tanAngle), vector);
            detectorInfo.setRotation(i, detectorInfo.rotation(i) * rot);
          }
        }
        // update: is the instrument rotated? Source horizontal still zero?
        sourceX =
            this->coordinate(sourceName, m_horizontalDirection, instrument);
        if (sourceX != 0.) {
          // calculate second rotation angle:
          tanAngle = sourceX /
                     this->coordinate(sourceName, m_beamDirection, instrument);
          for (auto compit = comps.begin(); compit != comps.end(); ++compit) {
            const auto compID2 = (*compit)->getComponentID();
            // horizontal direction will be set to zero
            V3D position = (*compit)->getPos();
            this->setCoordinate(position, m_horizontalDirection, 0.);
            double coordUp = this->coordinate(
                (*compit)->getName(), m_horizontalDirection, instrument);
            this->setCoordinate(position, m_beamDirection, tanAngle * coordUp);
            componentInfo.setPosition(componentInfo.indexOf(compID2), position);
          }
          for (size_t i = 0; i < detectorInfo.size(); ++i) {
            // horizontal direction will be set to zero
            V3D position = detectorInfo.position(i);
            this->setCoordinate(position, m_horizontalDirection, 0.);
            this->setCoordinate(
                position, m_beamDirection,
                tanAngle *
                    this->coordinate(detectorInfo, i, m_horizontalDirection));
            detectorInfo.setPosition(i, position);
            // rotate detectors
            const V3D &vector =
                instrument->getReferenceFrame()->vecPointingUp();
            const Quat &rot = Quat(atan(tanAngle), vector);
            detectorInfo.setRotation(i, detectorInfo.rotation(i) * rot);
          }
        }
      }
    }

    this->m_virtualInstrument = ws->getInstrument();

    if (this->m_virtualInstrument->isEmptyInstrument())
      this->g_log.error("Cannot create a virtual instrument.");
  } else
    this->g_log.error("Instrument of the InputWorkspace is not parametrised.");

  if (!(m_virtualInstrument->isParametrized()))
    this->g_log.error("Cannot copy parameter map correctly from original "
                      "instrument. Virtual instrument is not parametrised.");
}

/**
 * @brief GravityCorrection::spectrumCheck
 * @param spectrumInfo :: reference spectrumInfo
 * @param i :: spectrum number
 * @return true if spectrum can be considered for gravity correction, false
 * otherwise
 */
bool GravityCorrection::spectrumCheck(SpectrumInfo &spectrumInfo, size_t i) {
  if (spectrumInfo.isMonitor(i))
    this->g_log.debug("Found monitor spectrum, will be ignored.");
  if (!spectrumInfo.hasDetectors(i))
    this->g_log.debug("No detector(s) found");
  return (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i));
}

/**
 * @brief GravityCorrection::spectrumNumber
 * @param angle :: a final angle for a specific detector
 * @param spectrumInfo :: a reference to spectrum information
 * @param i :: spectrum number
 * @return spectrum number closest to the given final angle
 */
size_t GravityCorrection::spectrumNumber(const double angle,
                                         SpectrumInfo &spectrumInfo, size_t i) {
  if (!(this->spectrumCheck(spectrumInfo, i))) {
    size_t n = 0;
    if (m_finalAngles.empty())
      this->g_log.error(
          "Map of initial final angles and its corresponding spectrum "
          "number does not exist.");
    double currentAngle = spectrumInfo.signedTwoTheta(i) / 2.;
    // a starting, lower bound iterator for an effective search that should
    // exist
    auto start_i = this->m_finalAngles.find(currentAngle);
    if (start_i == m_finalAngles.end())
      this->g_log.debug("Cannot find final angle for this spectrum.");
    // search first for the spectrum with closest smaller final angle
    while (start_i != m_finalAngles.end()) {
      ++start_i;
      if (start_i->first < angle) {
        n = start_i->second;
        continue;
      } else
        break;
    }
    // compare if the final angle is closer to the closest smaller final
    // angle or the next final angle
    if ((spectrumInfo.signedTwoTheta(n) / 2. - angle) >
        (spectrumInfo.signedTwoTheta(n + 1) / 2. - angle))
      n += 1;
    // what if corrected hit exactly between two detectors? -> not taken into
    // account, happens unlikely.

    // counts are dropping down due to gravitation, thus needed to move counts
    // up and n cannot be smaller than 0, only larger than
    // m_ws->getNumberHistograms()
    if (n > this->m_ws->indexInfo().globalSize())
      this->g_log.information("Move counts out of spectrum range!");
    return n;
  } else
    return i;
}

/**
 * @brief GravityCorrection::parabolaArcLength: integration range is 0 (sample
 * position) to detector position in beam direction (expressed in arg).
 * @param k ::
 * @param arg ::
 * @param constant ::
 * @return length of the parabola arc
 */
double GravityCorrection::parabolaArcLength(const double arg,
                                            double constant) const {
  return 1 / 2 * (arg * sqrt(constant + pow(arg, 2)) +
                  constant * log((arg / sqrt(constant)) +
                                 sqrt(constant + (pow(arg, 2)) / constant)));
}

void GravityCorrection::exec() {

  this->m_progress =
      make_unique<Progress>(this, 0.0, 1.0, 3 + this->m_ws->size());

  this->m_progress->report("Create virtual instrument ...");
  this->virtualInstrument();
  this->m_progress->report("Checking slits ...");
  this->slitCheck();

  auto spectrumInfo = this->m_ws->spectrumInfo();

  this->m_progress->report("Setup OutputWorkspace ...");
  MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");
  if (!outWS)
    outWS = this->m_ws->clone();
  outWS->setTitle(this->m_ws->getTitle() + " cancelled gravitation ");

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!(this->spectrumCheck(spectrumInfo, i)))
      continue;

    // delete data (x, y, e)
    HistogramX &xVals = outWS->mutableX(i);
    for (HistogramX::iterator xit = xVals.begin(); xit < xVals.end(); ++xit)
      *xit = 0.;
    HistogramY &yVals = outWS->mutableY(i);
    for (auto yit = yVals.begin(); yit < yVals.end(); ++yit)
      *yit = 0.;
    HistogramE &eVals = outWS->mutableE(i);
    for (auto eit = eVals.begin(); eit < eVals.end(); ++eit)
      *eit = 0.;

    // setup map of initial final angles (y axis, spectra)
    // this map is sorted internally by its finalAngleValue's
    double finalAngleValue = spectrumInfo.signedTwoTheta(i) / 2.;
    this->m_finalAngles[finalAngleValue] = i;

    // unmask bins of OutputWorkspace
    if (outWS->hasMaskedBins(i)) {
      this->g_log.debug("Unmask bins.");
      const MatrixWorkspace::MaskList &maskOut = outWS->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator maskit;
      for (maskit = maskOut.begin(); maskit != maskOut.end(); ++maskit)
        outWS->flagMasked(i, maskit->first, 0.0);
    }
    this->m_progress->report();
  }

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!(this->spectrumCheck(spectrumInfo, i)))
      continue;

    // take neutrons that hit the detector of spectrum i
    // get a reference to X values, which will be modified
    MatrixWorkspace_sptr clonedWS = this->m_ws->clone();
    HistogramX &tof = clonedWS->mutableX(i);
    // correct tof angles, velocity, characteristic length
    size_t i_tofit{0};
    for (HistogramX::iterator tofit = tof.begin(); tofit < tof.end(); ++tofit) {
      // this velocity should take the real flight path into account
      if (*tofit == 0.) {
        this->g_log.notice(
            "Zero tof detected. Cannot divide by it, skip this bin.");
        continue;
      }

      // detector position
      double detZ = this->coordinate(spectrumInfo, i, m_beamDirection);
      double v{(spectrumInfo.l1() + spectrumInfo.l2(i)) / *tofit};
      double k = g / (2. * pow(v, 2));
      double angle = this->finalAngle(k, i);
      if (cos(angle) == 0.) {
        this->g_log.error("Cannot divide by zero for calculating new tof "
                          "values. Skip this bin.");
        continue;
      }
      *tofit = detZ / (v * cos(angle));

      // get new spectrum number for new final angle
      auto j = this->spectrumNumber(angle, spectrumInfo, i);
      if (j > spectrumInfo.size())
        continue; // counts and corresponding errors will be lost
      // need to set the counts to spectrum according to finalAngle & *tofit
      outWS->mutableX(j)[i_tofit] = *tofit;
      outWS->mutableY(j)[i_tofit] += this->m_ws->y(i)[i_tofit];
      outWS->mutableE(j)[i_tofit] += this->m_ws->e(i)[i_tofit];
      ++i_tofit;
      this->m_progress->report();
    }

    if (this->m_ws->hasMaskedBins(i)) {
      // store mask of InputWorkspace
      HistogramX &tof2 = outWS->mutableX(i);
      const MatrixWorkspace::MaskList &maskIn = this->m_ws->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator maskit;
      for (maskit = maskIn.begin(); maskit != maskIn.end(); ++maskit) {
        // determine offset for new bin index
        HistogramX::iterator t =
            find_if(tof2.begin(), tof2.end(), [&](const auto &ii) {
              return ii > this->m_ws->x(i)[maskit->first];
            });
        if (t != tof2.end()) {
          // get left bin boundary (index) of t
          pair<size_t, double> ti = outWS->getXIndex(i, *t, true, 0);
          outWS->flagMasked(i, ti.first, maskit->second);
        }
        this->m_progress->report();
      }
    }
  }
  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
