#include "MantidAlgorithms/GravityCorrection.h"

//#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>
#include <forward_list>
#include <iterator>
#include <utility>

using Mantid::API::AlgorithmHistories; // a std::set
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
using Mantid::Geometry::IComponent_const_sptr;
using Mantid::Geometry::IComponent_sptr;
using Mantid::Geometry::ParameterMap_sptr;
using Mantid::Geometry::PointingAlong;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::HistogramE;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::Exception::InstrumentDefinitionError;
using Mantid::Kernel::Exception::NotFoundError;
using Mantid::Kernel::make_unique;
using Mantid::Kernel::V3D;
using Mantid::PhysicalConstants::g;

using boost::make_shared;

using std::find_if;
using std::forward_list;
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
  if (!this->m_ws)
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
  // Slits
  const string slit1Name = this->getProperty("FirstSlitName");
  const string slit2Name = this->getProperty("SecondSlitName");
  // Check slit component existance
  // get a pointer to the instrument of the input workspace
  this->m_originalInstrument = this->m_ws->getInstrument();
  IComponent_const_sptr slit1 =
      this->m_originalInstrument->getComponentByName(slit1Name);
  if (!slit1)
    result["FirstSlitName"] =
        "Instrument component with name " + slit1Name + " does not exist.";
  IComponent_const_sptr slit2 =
      this->m_originalInstrument->getComponentByName(slit2Name);
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
    g_log.error("Axis is not X/Y/Z");
    throw runtime_error("Axis is not X/Y/Z");
    break;
  }
  return position;
}

/**
 * @brief GravityCorrection::slitCheck
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

/**
 * @brief GravityCorrection::finalAngle computes the update on the final angle
 * taking gravity into account
 * @param k
 * @return a new, corrected final angle
 */
double GravityCorrection::finalAngle(const double k) {
  double beamShift, upShift;
  tie(beamShift, upShift) = this->GravityCorrection::parabola(k);
  return atan(2. * k * sqrt(upShift / k));
}

/**
 * @brief GravityCorrection::virtualInstrument defines a virtual instrument with
 * the sample at its origin x = y = z = 0 m. The original instrument and its
 * parameter map will be copied.
 * @param spectrumInfo ::
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
      // parameter map holds parameters of all modified
      // IComponent_const_sptr's:
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
      if (this->m_originalInstrument->getSample()->getPos() ==
          V3D(0.0, 0.0, 0.0))
        this->g_log.debug("Accidentially moved the original instrument.");
    }
    if (this->m_virtualInstrument->isEmptyInstrument())
      this->g_log.debug("Cannot create a virtual instrument.");
  } else
    this->g_log.debug("Instrument of the InputWorkspace is not parametrised.");
  if (this->m_virtualInstrument->getSample()->getPos() != V3D(0.0, 0.0, 0.0))
    this->g_log.debug("Could not move the virtual instrument.");
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
    this->g_log.debug("Is monitor spectrum, will be ignored.");
  if (!spectrumInfo.hasDetectors(i)) {
    g_log.debug("No detector(s) found");
  }
  return (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i));
}

/**
 * @brief GravityCorrection::spectrumNumber
 * @param angle :: a final angle for a specific detector
 * @param spectrumInfo ::
 * @param i ::
 * @return spectrum number closest to the given final angle
 */
size_t GravityCorrection::spectrumNumber(const double angle,
                                         SpectrumInfo &spectrumInfo, size_t i) {
  if (!(this->spectrumCheck(spectrumInfo, i))) {
    size_t n = 0;
    if (m_finalAngles.empty())
      g_log.error("Map of initial final angles and its corresponding spectrum "
                  "number does not exist.");
    double currentAngle = spectrumInfo.signedTwoTheta(i) / 2.;
    // a starting, lower bound iterator for an effective search that should
    // exist
    auto start_i = this->m_finalAngles.find(currentAngle);
    if (start_i == m_finalAngles.end())
      g_log.debug("Cannot find final angle for this spectrum.");
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
    if (n > this->m_ws->getNumberHistograms())
      g_log.information("Move counts out of spectrum range.");
    return n;
  } else
    return i;
}

void GravityCorrection::exec() {
  this->m_progress = make_unique<Progress>(this, 0.0, 1.0, 3); // or size() ?

  const auto refFrame = m_originalInstrument->getReferenceFrame();
  this->m_upDirection = refFrame->pointingUp();
  this->m_beamDirection = refFrame->pointingAlongBeam();
  this->m_horizontalDirection = refFrame->pointingHorizontal();
  this->m_progress->report("Check slits ...");
  this->slitCheck();

  auto spectrumInfo = this->m_ws->spectrumInfo();

  this->m_progress->report("Create virtual instrument ...");
  this->virtualInstrument(spectrumInfo);

  double finalAngleValue;

  this->m_progress->report("Setup OutputWorkspace");
  MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");
  outWS = this->m_ws->clone();
  outWS->setTitle(this->m_ws->getTitle());

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    // delete data (x, y, e)
    HistogramX &xVals = outWS->mutableX(i);
    for (HistogramX::iterator xit = xVals.begin(); xit < xVals.end(); ++xit)
      *xit = 0.;
    HistogramY &Y = outWS->mutableY(i);
    for (auto yit = Y.begin(); yit < Y.end(); ++yit)
      *yit = 0.;
    HistogramE &E = outWS->mutableE(i);
    for (auto eit = E.begin(); eit < E.end(); ++eit)
      *eit = 0.;

    // setup map of initial final angles (y axis, spectra)
    // this map is sorted internally by its finalAngleValue's
    if (!(this->spectrumCheck(spectrumInfo, i)))
      continue;
    finalAngleValue = spectrumInfo.signedTwoTheta(i) / 2.;
    this->m_finalAngles[finalAngleValue] = i;

    // unmask bins of OutputWorkspace
    if (outWS->hasMaskedBins(i)) {
      const MatrixWorkspace::MaskList &maskOut = outWS->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator maskit;
      for (maskit = maskOut.begin(); maskit != maskOut.end(); ++maskit)
        outWS->flagMasked(i, maskit->first, 0.0);
    }
  }

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    stringstream istr;
    istr << static_cast<int>(i);
    this->g_log.debug("Spectrum " + i);
    if (!(this->spectrumCheck(spectrumInfo, i)))
      continue;

    // take neutrons that hit the detector of spectrum i
    V3D pos = spectrumInfo.position(i);
    double l1 = spectrumInfo.l1();
    // get a reference to X (tof values), Y and E of the input WS
    HistogramX &tof = this->m_ws->mutableX(i);
    // correct tof angles, velocity, characteristic length
    size_t i_tofit{0};
    for (HistogramX::iterator tofit = tof.begin(); tofit < tof.end(); ++tofit) {
      double v{l1 / (*tofit)};
      double k = g / (2. * pow(v, 2));
      double angle = this->finalAngle(k);
      // no sorting of tof values needed
      *tofit = pos.X() / (v * cos(angle));

      // get new spectrum number for new final angle
      auto j = this->spectrumNumber(angle, spectrumInfo, i);
      if (j > spectrumInfo.size())
        continue; // counts and corresponding errors will be lost

      // need to set the counts to spectrum according to finalAngle & *tofit
      outWS->mutableX(j)[i_tofit] = *tofit;
      outWS->mutableY(j)[i_tofit] = this->m_ws->y(i)[i_tofit];
      outWS->mutableE(j)[i_tofit] = this->m_ws->e(i)[i_tofit];
      ++i_tofit;
    }

    if (this->m_ws->hasMaskedBins(i)) {
      // store mask of InputWorkspace
      const HistogramX &tof2 = outWS->mutableX(i);
      const MatrixWorkspace::MaskList &maskIn = this->m_ws->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator maskit;
      for (maskit = maskIn.begin(); maskit != maskIn.end(); ++maskit) {
        // determine offset for new bin index
        auto t = find_if(tof2.begin(), tof2.end(), [&](const auto &ii) {
          return ii > this->m_ws->x(i)[ii];
        });
        if (t != tof2.end()) {
          // get left bin boundary (index) of t
          pair<size_t, double> ti = outWS->getXIndex(i, *t, true, 0);
          outWS->flagMasked(i, ti.first, maskit->second);
        }
      }
    }
  }
  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
