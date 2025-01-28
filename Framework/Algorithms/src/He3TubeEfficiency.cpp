// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/He3TubeEfficiency.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/FloatingPointComparison.h"

#include <cmath>
#include <stdexcept>

// this should be a big number but not so big that there are rounding errors
const double DIST_TO_UNIVERSE_EDGE = 1e3;

// Scalar constant for exponential in units of K / (m Angstroms atm)
const double EXP_SCALAR_CONST = 2175.486863864;

// Tolerance for diameter/thickness comparison
const double TOL = 1.0e-8;

namespace Mantid::Algorithms {

using namespace DataObjects;
using namespace HistogramData;
// Register the class into the algorithm factory
DECLARE_ALGORITHM(He3TubeEfficiency)

/// Default constructor
He3TubeEfficiency::He3TubeEfficiency()
    : Algorithm(), m_inputWS(), m_outputWS(), m_paraMap(nullptr), m_shapeCache(), m_samplePos(), m_spectraSkipped(),
      m_progress(nullptr) {
  m_shapeCache.clear();
}

/**
 * Declare algorithm properties
 */
void He3TubeEfficiency::init() {
  using namespace Mantid::Kernel;

  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<API::WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<API::HistogramValidator>();
  wsValidator->add<API::InstrumentValidator>();
  this->declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                            "InputWorkspace", "", Kernel::Direction::Input, wsValidator),
                        "Name of the input workspace");
  this->declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "Name of the output workspace, can be the same as the input");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  this->declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>("ScaleFactor", 1.0, mustBePositive),
                        "Constant factor with which to scale the calculated"
                        "detector efficiency. Same factor applies to all efficiencies.");

  auto mustBePosArr = std::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  this->declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("TubePressure", mustBePosArr),
                        "Provide overriding the default tube pressure. The pressure must "
                        "be specified in atm.");
  this->declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("TubeThickness", mustBePosArr),
                        "Provide overriding the default tube thickness. The thickness must "
                        "be specified in metres.");
  this->declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("TubeTemperature", mustBePosArr),
                        "Provide overriding the default tube temperature. The temperature must "
                        "be specified in Kelvin.");
}

/**
 * Executes the algorithm
 */
void He3TubeEfficiency::exec() {
  // Get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");
  m_outputWS = this->getProperty("OutputWorkspace");

  if (m_outputWS != m_inputWS) {
    m_outputWS = create<API::MatrixWorkspace>(*m_inputWS);
  }

  // Get the detector parameters
  m_paraMap = &(m_inputWS->constInstrumentParameters());

  // Store some information about the instrument setup that will not change
  m_samplePos = m_inputWS->getInstrument()->getSample()->getPos();

  // Check if it is an event workspace
  DataObjects::EventWorkspace_const_sptr eventW =
      std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(m_inputWS);
  if (eventW != nullptr) {
    this->execEvent();
    return;
  }

  std::size_t numHists = m_inputWS->getNumberHistograms();
  m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, numHists);
  const auto &spectrumInfo = m_inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    m_outputWS->setSharedX(i, m_inputWS->sharedX(i));
    try {
      this->correctForEfficiency(i, spectrumInfo);
    } catch (std::out_of_range &) {
      // if we don't have all the data there will be spectra we can't correct,
      // avoid leaving the workspace part corrected
      m_outputWS->mutableY(i) = 0;
      PARALLEL_CRITICAL(deteff_invalid) { m_spectraSkipped.emplace_back(m_inputWS->getAxis(1)->spectraNo(i)); }
    }

    // make regular progress reports
    m_progress->report();
    // check for canceling the algorithm
    if (i % 1000 == 0) {
      interruption_point();
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  this->logErrors();
  this->setProperty("OutputWorkspace", m_outputWS);
}

/**
 * Corrects a spectra for the detector efficiency calculated from detector
 * information. Gets the detector information and uses this to calculate its
 * efficiency
 *  @param spectraIndex :: index of the spectrum to get the efficiency for
 *  @param spectrumInfo :: the SpectrumInfo object for the input workspace
 *  @throw invalid_argument if the shape of a detector is isn't a cylinder
 *  aligned along one axis
 *  @throw NotFoundError if the detector or its gas pressure or wall thickness
 *  were not found
 */
void He3TubeEfficiency::correctForEfficiency(std::size_t spectraIndex, const API::SpectrumInfo &spectrumInfo) {
  if (spectrumInfo.isMonitor(spectraIndex) || spectrumInfo.isMasked(spectraIndex)) {
    return;
  }

  const auto &det = spectrumInfo.detector(spectraIndex);
  const double exp_constant = this->calculateExponential(spectraIndex, det);
  const double scale = this->getProperty("ScaleFactor");

  const auto &yValues = m_inputWS->y(spectraIndex);
  auto &yOut = m_outputWS->mutableY(spectraIndex);

  const auto wavelength = m_inputWS->points(spectraIndex);

  // todo is it going to die? returning local var by reference
  std::vector<double> effCorrection(wavelength.size());

  computeEfficiencyCorrection(effCorrection, wavelength, exp_constant, scale);

  std::transform(yValues.cbegin(), yValues.cend(), effCorrection.cbegin(), yOut.begin(),
                 [&](const double y, const double effcorr) { return y * effcorr; });

  const auto &eValues = m_inputWS->e(spectraIndex);
  auto &eOut = m_outputWS->mutableE(spectraIndex);

  // reset wavelength iterator
  std::transform(eValues.cbegin(), eValues.cend(), effCorrection.cbegin(), eOut.begin(),
                 [&](const double e, const double effcorr) { return e * effcorr; });
}

/**
 * This function calculates the exponential contribution to the He3 tube
 * efficiency.
 * @param spectraIndex :: the current index to calculate
 * @param idet :: the current detector pointer
 * @throw out_of_range if twice tube thickness is greater than tube diameter
 * @return the exponential contribution for the given detector
 */
double He3TubeEfficiency::calculateExponential(std::size_t spectraIndex, const Geometry::IDetector &idet) {
  // Get the parameters for the current associated tube
  double pressure = this->getParameter("TubePressure", spectraIndex, "tube_pressure", idet);
  double tubethickness = this->getParameter("TubeThickness", spectraIndex, "tube_thickness", idet);
  double temperature = this->getParameter("TubeTemperature", spectraIndex, "tube_temperature", idet);

  double detRadius(0.0);
  Kernel::V3D detAxis;
  this->getDetectorGeometry(idet, detRadius, detAxis);
  double detDiameter = 2.0 * detRadius;
  double twiceTubeThickness = 2.0 * tubethickness;

  // now get the sin of the angle, it's the magnitude of the cross product of
  // unit vector along the detector tube axis and a unit vector directed from
  // the sample to the detector center
  const Kernel::V3D vectorFromSample = normalize(idet.getPos() - m_samplePos);
  Kernel::Quat rot = idet.getRotation();
  // rotate the original cylinder object axis to get the detector axis in the
  // actual instrument
  rot.rotate(detAxis);
  detAxis.normalize();
  // Scalar product is quicker than cross product
  double cosTheta = detAxis.scalar_prod(vectorFromSample);
  double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);

  const double straight_path = detDiameter - twiceTubeThickness;
  if (Kernel::withinAbsoluteDifference(straight_path, 0.0, TOL)) {
    throw std::out_of_range("Twice tube thickness cannot be greater than "
                            "or equal to the tube diameter");
  }

  const double pathlength = straight_path / sinTheta;
  return EXP_SCALAR_CONST * (pressure / temperature) * pathlength;
}

/**
 * Update the shape cache if necessary
 * @param det :: a pointer to the detector to query
 * @param detRadius :: An output parameter that contains the detector radius
 * @param detAxis :: An output parameter that contains the detector axis vector
 */
void He3TubeEfficiency::getDetectorGeometry(const Geometry::IDetector &det, double &detRadius, Kernel::V3D &detAxis) {
  std::shared_ptr<const Geometry::IObject> shape_sptr = det.shape();
  if (!shape_sptr) {
    throw std::runtime_error("Detector geometry error: detector with id: " + std::to_string(det.getID()) +
                             " does not have shape. Is this a detectors group?\n"
                             "The algorithm works for instruments with one-to-one "
                             "spectra-to-detector maps only!");
  }
  // std::map<const Geometry::Object *, std::pair<double,
  // Kernel::V3D>>::const_iterator
  auto it = m_shapeCache.find(shape_sptr.get());
  if (it == m_shapeCache.end()) {
    double xDist = distToSurface(Kernel::V3D(DIST_TO_UNIVERSE_EDGE, 0, 0), shape_sptr.get());
    double zDist = distToSurface(Kernel::V3D(0, 0, DIST_TO_UNIVERSE_EDGE), shape_sptr.get());
    if (Kernel::withinAbsoluteDifference(zDist, xDist, 1e-8)) {
      detRadius = zDist / 2.0;
      detAxis = Kernel::V3D(0, 1, 0);
      // assume radii in z and x and the axis is in the y
      PARALLEL_CRITICAL(deteff_shapecachea) { m_shapeCache.insert({shape_sptr.get(), {detRadius, detAxis}}); }
      return;
    }
    double yDist = distToSurface(Kernel::V3D(0, DIST_TO_UNIVERSE_EDGE, 0), shape_sptr.get());
    if (Kernel::withinAbsoluteDifference(yDist, zDist, 1e-8)) {
      detRadius = yDist / 2.0;
      detAxis = Kernel::V3D(1, 0, 0);
      // assume that y and z are radii of the cylinder's circular cross-section
      // and the axis is perpendicular, in the x direction
      PARALLEL_CRITICAL(deteff_shapecacheb) { m_shapeCache.insert({shape_sptr.get(), {detRadius, detAxis}}); }
      return;
    }

    if (Kernel::withinAbsoluteDifference(xDist, yDist, 1e-8)) {
      detRadius = xDist / 2.0;
      detAxis = Kernel::V3D(0, 0, 1);
      PARALLEL_CRITICAL(deteff_shapecachec) { m_shapeCache.insert({shape_sptr.get(), {detRadius, detAxis}}); }
      return;
    }
  } else {
    std::pair<double, Kernel::V3D> geometry = it->second;
    detRadius = geometry.first;
    detAxis = geometry.second;
  }
}

/**
 * For basic shapes centered on the origin (0,0,0) this returns the distance to
 * the surface in the direction of the point given
 *  @param start :: the distance calculated from origin to the surface in a line
 *  towards this point. It should be outside the shape
 *  @param shape :: the object to calculate for, should be centered on the
 * origin
 *  @return the distance to the surface in the direction of the point given
 *  @throw invalid_argument if there is any error finding the distance
 * @returns The distance to the surface in metres
 */
double He3TubeEfficiency::distToSurface(const Kernel::V3D &start, const Geometry::IObject *shape) const {
  // get a vector from the point that was passed to the origin
  const Kernel::V3D direction = normalize(-start);
  // put the point and the vector (direction) together to get a line,
  // here called a track
  Geometry::Track track(start, direction);
  // split the track (line) up into the part that is inside the shape and the
  // part that is outside
  shape->interceptSurface(track);

  if (track.count() != 1) {
    // the track missed the shape, probably the shape is not centered on
    // the origin
    throw std::invalid_argument("Fatal error interpreting the shape of a detector");
  }
  // the first part of the track will be the part inside the shape,
  // return its length
  return track.cbegin()->distInsideObject;
}

/**
 * Calculate the detector efficiency from the detector parameters and the
 * spectrum's x-axis.
 * @param alpha :: the value to feed to the exponential
 * @param scale_factor :: an overall value for scaling the efficiency
 * @return the calculated efficiency
 */
double He3TubeEfficiency::detectorEfficiency(const double alpha, const double scale_factor) const {
  return (scale_factor / (1.0 - std::exp(-alpha)));
}

/**
 * Logs if there were any problems locating spectra.
 */
void He3TubeEfficiency::logErrors() const {
  std::vector<int>::size_type nspecs = m_spectraSkipped.size();
  if (nspecs > 0) {
    this->g_log.warning() << "There were " << nspecs << " spectra that could not be corrected. ";
    this->g_log.debug() << "Unaffected spectra numbers: ";
    for (size_t i = 0; i < nspecs; ++i) {
      this->g_log.debug() << m_spectraSkipped[i] << " ";
    }
    this->g_log.debug() << '\n';
  }
}

/**
 * Retrieve the detector parameter either from the workspace property or from
 * the associated detector property.
 * @param wsPropName :: the workspace property name for the detector parameter
 * @param currentIndex :: the currently requested spectra index
 * @param detPropName :: the detector property name for the detector parameter
 * @param idet :: the current detector
 * @return the value of the detector property
 */
double He3TubeEfficiency::getParameter(const std::string &wsPropName, std::size_t currentIndex,
                                       const std::string &detPropName, const Geometry::IDetector &idet) {
  std::vector<double> wsProp = this->getProperty(wsPropName);

  if (wsProp.empty()) {
    return idet.getNumberParameter(detPropName).at(0);
  } else {
    if (wsProp.size() == 1) {
      return wsProp.at(0);
    } else {
      return wsProp.at(currentIndex);
    }
  }
}

/**
 * Execute for events
 */
void He3TubeEfficiency::execEvent() {
  this->g_log.information("Processing event workspace");

  const API::MatrixWorkspace_const_sptr matrixInputWS = getProperty("InputWorkspace");

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    setProperty("OutputWorkspace", matrixOutputWS);
  }
  auto loc_outputWS = std::dynamic_pointer_cast<DataObjects::EventWorkspace>(matrixOutputWS);

  std::size_t numHistograms = loc_outputWS->getNumberHistograms();
  auto &spectrumInfo = loc_outputWS->mutableSpectrumInfo();
  m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, numHistograms);

  PARALLEL_FOR_IF(Kernel::threadSafe(*loc_outputWS))
  for (int i = 0; i < static_cast<int>(numHistograms); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    const auto &det = spectrumInfo.detector(i);
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i)) {
      continue;
    }

    double exp_constant = 0.0;
    try {
      exp_constant = this->calculateExponential(i, det);
    } catch (std::out_of_range &) {
      // Parameters are bad so skip correction
      PARALLEL_CRITICAL(deteff_invalid) {
        m_spectraSkipped.emplace_back(loc_outputWS->getAxis(1)->spectraNo(i));
        loc_outputWS->getSpectrum(i).clearData();
        spectrumInfo.setMasked(i, true);
      }
    }

    // Do the correction
    auto &evlist = loc_outputWS->getSpectrum(i);
    switch (evlist.getEventType()) {
    case API::TOF:
      // Switch to weights if needed.
      evlist.switchTo(API::WEIGHTED);
    // Fall through
    case API::WEIGHTED:
      eventHelper(evlist.getWeightedEvents(), exp_constant);
      break;
    case API::WEIGHTED_NOTIME:
      eventHelper(evlist.getWeightedEventsNoTime(), exp_constant);
      break;
    }

    m_progress->report();

    // check for canceling the algorithm
    if (i % 1000 == 0) {
      interruption_point();
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  loc_outputWS->clearMRU();

  this->logErrors();
}

/** Private function to calculate the effeciency correction from
 * points(wavelength)
 * @param effCorrection :: Vector that will hold the values for each wavelength
 * @param wavelength :: The points calculated from the histogram
 * @param expConstant :: The exponential constant
 * @param scale :: The scale factor
 */
void He3TubeEfficiency::computeEfficiencyCorrection(std::vector<double> &effCorrection, const Points &xes,
                                                    const double expConstant, const double scale) const {

  std::transform(xes.cbegin(), xes.cend(), effCorrection.begin(),
                 [&](double wavelen) { return detectorEfficiency(expConstant * wavelen, scale); });
}

/**
 * Private function for doing the event correction.
 * @param events :: the list of events to correct
 * @param expval :: the value of the exponent for the detector efficiency
 */
template <class T> void He3TubeEfficiency::eventHelper(std::vector<T> &events, double expval) {
  const double scale = this->getProperty("ScaleFactor");

  for (auto &event : events) {
    auto de = static_cast<float>(this->detectorEfficiency(expval * event.tof(), scale));
    event.m_weight *= de;
    event.m_errorSquared *= de * de;
  }
}

} // namespace Mantid::Algorithms
