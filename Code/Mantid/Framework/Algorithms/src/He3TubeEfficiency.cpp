#include "MantidAlgorithms/He3TubeEfficiency.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/cow_ptr.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// this should be a big number but not so big that there are rounding errors
const double DIST_TO_UNIVERSE_EDGE = 1e3;

// Scalar constant for exponential in units of K / (m Angstroms atm)
const double EXP_SCALAR_CONST = 2175.486863864;

// Tolerance for diameter/thickness comparison
const double TOL = 1.0e-8;

namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(He3TubeEfficiency)

/// Default constructor
He3TubeEfficiency::He3TubeEfficiency()
    : Algorithm(), inputWS(), outputWS(), paraMap(NULL), shapeCache(),
      samplePos(), spectraSkipped(), progress(NULL) {
  this->shapeCache.clear();
}

/// Destructor
He3TubeEfficiency::~He3TubeEfficiency() {
  if (this->progress) {
    delete this->progress;
  }
}

/**
 * Declare algorithm properties
 */
void He3TubeEfficiency::init() {
  using namespace Mantid::Kernel;

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<API::WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<API::HistogramValidator>();
  wsValidator->add<API::InstrumentValidator>();
  this->declareProperty(
      new API::WorkspaceProperty<API::MatrixWorkspace>(
          "InputWorkspace", "", Kernel::Direction::Input, wsValidator),
      "Name of the input workspace");
  this->declareProperty(
      new API::WorkspaceProperty<API::MatrixWorkspace>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "Name of the output workspace, can be the same as the input");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  this->declareProperty(
      new Kernel::PropertyWithValue<double>("ScaleFactor", 1.0, mustBePositive),
      "Constant factor with which to scale the calculated"
      "detector efficiency. Same factor applies to all efficiencies.");

  auto mustBePosArr =
      boost::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  this->declareProperty(
      new Kernel::ArrayProperty<double>("TubePressure", mustBePosArr),
      "Provide overriding the default tube pressure. The pressure must "
      "be specified in atm.");
  this->declareProperty(
      new Kernel::ArrayProperty<double>("TubeThickness", mustBePosArr),
      "Provide overriding the default tube thickness. The thickness must "
      "be specified in metres.");
  this->declareProperty(
      new Kernel::ArrayProperty<double>("TubeTemperature", mustBePosArr),
      "Provide overriding the default tube temperature. The temperature must "
      "be specified in Kelvin.");
}

/**
 * Executes the algorithm
 */
void He3TubeEfficiency::exec() {
  // Get the workspaces
  this->inputWS = this->getProperty("InputWorkspace");
  this->outputWS = this->getProperty("OutputWorkspace");

  if (this->outputWS != this->inputWS) {
    this->outputWS = API::WorkspaceFactory::Instance().create(this->inputWS);
  }

  // Get the detector parameters
  this->paraMap = &(this->inputWS->instrumentParameters());

  // Store some information about the instrument setup that will not change
  this->samplePos = this->inputWS->getInstrument()->getSample()->getPos();

  // Check if it is an event workspace
  DataObjects::EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWS);
  if (eventW != NULL) {
    this->execEvent();
    return;
  }

  std::size_t numHists = this->inputWS->getNumberHistograms();
  this->progress = new API::Progress(this, 0.0, 1.0, numHists);

  PARALLEL_FOR2(inputWS, outputWS)
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION

    this->outputWS->setX(i, this->inputWS->refX(i));
    try {
      this->correctForEfficiency(i);
    } catch (std::out_of_range &) {
      // if we don't have all the data there will be spectra we can't correct,
      // avoid leaving the workspace part corrected
      Mantid::MantidVec &dud = this->outputWS->dataY(i);
      std::transform(dud.begin(), dud.end(), dud.begin(),
                     std::bind2nd(std::multiplies<double>(), 0));
      PARALLEL_CRITICAL(deteff_invalid) {
        this->spectraSkipped.push_back(this->inputWS->getAxis(1)->spectraNo(i));
      }
    }

    // make regular progress reports
    this->progress->report();
    // check for canceling the algorithm
    if (i % 1000 == 0) {
      interruption_point();
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->logErrors();
  this->setProperty("OutputWorkspace", this->outputWS);
}

/**
 * Corrects a spectra for the detector efficiency calculated from detector
 * information. Gets the detector information and uses this to calculate its
 * efficiency
 *  @param spectraIndex :: index of the spectrum to get the efficiency for
 *  @throw invalid_argument if the shape of a detector is isn't a cylinder
 *  aligned along one axis
 *  @throw NotFoundError if the detector or its gas pressure or wall thickness
 *  were not found
 */
void He3TubeEfficiency::correctForEfficiency(std::size_t spectraIndex) {
  Geometry::IDetector_const_sptr det = this->inputWS->getDetector(spectraIndex);
  if (det->isMonitor() || det->isMasked()) {
    return;
  }

  const double exp_constant = this->calculateExponential(spectraIndex, det);
  const double scale = this->getProperty("ScaleFactor");

  Mantid::MantidVec &yout = this->outputWS->dataY(spectraIndex);
  Mantid::MantidVec &eout = this->outputWS->dataE(spectraIndex);
  // Need the original values so this is not a reference
  const Mantid::MantidVec yValues = this->inputWS->readY(spectraIndex);
  const Mantid::MantidVec eValues = this->inputWS->readE(spectraIndex);

  std::vector<double>::const_iterator yinItr = yValues.begin();
  std::vector<double>::const_iterator einItr = eValues.begin();
  Mantid::MantidVec::const_iterator xItr =
      this->inputWS->readX(spectraIndex).begin();
  Mantid::MantidVec::iterator youtItr = yout.begin();
  Mantid::MantidVec::iterator eoutItr = eout.begin();

  for (; youtItr != yout.end(); ++youtItr, ++eoutItr) {
    const double wavelength = (*xItr + *(xItr + 1)) / 2.0;
    const double effcorr =
        this->detectorEfficiency(exp_constant * wavelength, scale);
    *youtItr = (*yinItr) * effcorr;
    *eoutItr = (*einItr) * effcorr;
    ++yinItr;
    ++einItr;
    ++xItr;
  }

  return;
}

/**
 * This function calculates the exponential contribution to the He3 tube
 * efficiency.
 * @param spectraIndex :: the current index to calculate
 * @param idet :: the current detector pointer
 * @throw out_of_range if twice tube thickness is greater than tube diameter
 * @return the exponential contribution for the given detector
 */
double He3TubeEfficiency::calculateExponential(
    std::size_t spectraIndex,
    boost::shared_ptr<const Geometry::IDetector> idet) {
  // Get the parameters for the current associated tube
  double pressure =
      this->getParameter("TubePressure", spectraIndex, "tube_pressure", idet);
  double tubethickness =
      this->getParameter("TubeThickness", spectraIndex, "tube_thickness", idet);
  double temperature = this->getParameter("TubeTemperature", spectraIndex,
                                          "tube_temperature", idet);

  double detRadius(0.0);
  Kernel::V3D detAxis;
  this->getDetectorGeometry(idet, detRadius, detAxis);
  double detDiameter = 2.0 * detRadius;
  double twiceTubeThickness = 2.0 * tubethickness;

  // now get the sin of the angle, it's the magnitude of the cross product of
  // unit vector along the detector tube axis and a unit vector directed from
  // the sample to the detector center
  Kernel::V3D vectorFromSample = idet->getPos() - this->samplePos;
  vectorFromSample.normalize();
  Kernel::Quat rot = idet->getRotation();
  // rotate the original cylinder object axis to get the detector axis in the
  // actual instrument
  rot.rotate(detAxis);
  detAxis.normalize();
  // Scalar product is quicker than cross product
  double cosTheta = detAxis.scalar_prod(vectorFromSample);
  double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);

  const double straight_path = detDiameter - twiceTubeThickness;
  if (std::fabs(straight_path - 0.0) < TOL) {
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
void He3TubeEfficiency::getDetectorGeometry(
    boost::shared_ptr<const Geometry::IDetector> det, double &detRadius,
    Kernel::V3D &detAxis) {
  boost::shared_ptr<const Geometry::Object> shape_sptr = det->shape();
  std::map<const Geometry::Object *,
           std::pair<double, Kernel::V3D>>::const_iterator it =
      this->shapeCache.find(shape_sptr.get());
  if (it == this->shapeCache.end()) {
    double xDist = distToSurface(Kernel::V3D(DIST_TO_UNIVERSE_EDGE, 0, 0),
                                 shape_sptr.get());
    double zDist = distToSurface(Kernel::V3D(0, 0, DIST_TO_UNIVERSE_EDGE),
                                 shape_sptr.get());
    if (std::abs(zDist - xDist) < 1e-8) {
      detRadius = zDist / 2.0;
      detAxis = Kernel::V3D(0, 1, 0);
      // assume radii in z and x and the axis is in the y
      PARALLEL_CRITICAL(deteff_shapecachea) {
        this->shapeCache.insert(
            std::pair<const Geometry::Object *, std::pair<double, Kernel::V3D>>(
                shape_sptr.get(),
                std::pair<double, Kernel::V3D>(detRadius, detAxis)));
      }
      return;
    }
    double yDist = distToSurface(Kernel::V3D(0, DIST_TO_UNIVERSE_EDGE, 0),
                                 shape_sptr.get());
    if (std::abs(yDist - zDist) < 1e-8) {
      detRadius = yDist / 2.0;
      detAxis = Kernel::V3D(1, 0, 0);
      // assume that y and z are radii of the cylinder's circular cross-section
      // and the axis is perpendicular, in the x direction
      PARALLEL_CRITICAL(deteff_shapecacheb) {
        this->shapeCache.insert(
            std::pair<const Geometry::Object *, std::pair<double, Kernel::V3D>>(
                shape_sptr.get(),
                std::pair<double, Kernel::V3D>(detRadius, detAxis)));
      }
      return;
    }

    if (std::abs(xDist - yDist) < 1e-8) {
      detRadius = xDist / 2.0;
      detAxis = Kernel::V3D(0, 0, 1);
      PARALLEL_CRITICAL(deteff_shapecachec) {
        this->shapeCache.insert(
            std::pair<const Geometry::Object *, std::pair<double, Kernel::V3D>>(
                shape_sptr.get(),
                std::pair<double, Kernel::V3D>(detRadius, detAxis)));
      }
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
double He3TubeEfficiency::distToSurface(const Kernel::V3D start,
                                        const Geometry::Object *shape) const {
  // get a vector from the point that was passed to the origin
  Kernel::V3D direction = Kernel::V3D(0.0, 0.0, 0.0) - start;
  // it needs to be a unit vector
  direction.normalize();
  // put the point and the vector (direction) together to get a line,
  // here called a track
  Geometry::Track track(start, direction);
  // split the track (line) up into the part that is inside the shape and the
  // part that is outside
  shape->interceptSurface(track);

  if (track.count() != 1) {
    // the track missed the shape, probably the shape is not centered on
    // the origin
    throw std::invalid_argument(
        "Fatal error interpreting the shape of a detector");
  }
  // the first part of the track will be the part inside the shape,
  // return its length
  return track.begin()->distInsideObject;
}

/**
 * Calculate the detector efficiency from the detector parameters and the
 * spectrum's x-axis.
 * @param alpha :: the value to feed to the exponential
 * @param scale_factor :: an overall value for scaling the efficiency
 * @return the calculated efficiency
 */
double He3TubeEfficiency::detectorEfficiency(const double alpha,
                                             const double scale_factor) const {
  return (scale_factor / (1.0 - std::exp(-alpha)));
}

/**
 * Logs if there were any problems locating spectra.
 */
void He3TubeEfficiency::logErrors() const {
  std::vector<int>::size_type nspecs = this->spectraSkipped.size();
  if (nspecs > 0) {
    this->g_log.warning() << "There were " << nspecs
                          << " spectra that could not be corrected. ";
    this->g_log.debug() << "Unaffected spectra numbers: ";
    for (size_t i = 0; i < nspecs; ++i) {
      this->g_log.debug() << this->spectraSkipped[i] << " ";
    }
    this->g_log.debug() << std::endl;
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
double He3TubeEfficiency::getParameter(
    std::string wsPropName, std::size_t currentIndex, std::string detPropName,
    boost::shared_ptr<const Geometry::IDetector> idet) {
  std::vector<double> wsProp = this->getProperty(wsPropName);

  if (wsProp.empty()) {
    return idet->getNumberParameter(detPropName).at(0);
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

  const API::MatrixWorkspace_const_sptr matrixInputWS =
      this->getProperty("InputWorkspace");
  DataObjects::EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(
          matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS =
      this->getProperty("OutputWorkspace");
  DataObjects::EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS) {
    outputWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
        matrixOutputWS);
  } else {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS,
                                                           false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));

    // Cast to the matrixOutputWS and save it
    matrixOutputWS =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  std::size_t numHistograms = inputWS->getNumberHistograms();
  this->progress = new API::Progress(this, 0.0, 1.0, numHistograms);
  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < static_cast<int>(numHistograms); ++i) {
    PARALLEL_START_INTERUPT_REGION

    Geometry::IDetector_const_sptr det = inputWS->getDetector(i);
    if (det->isMonitor() || det->isMasked()) {
      continue;
    }

    double exp_constant = 0.0;
    try {
      exp_constant = this->calculateExponential(i, det);
    } catch (std::out_of_range &) {
      // Parameters are bad so skip correction
      PARALLEL_CRITICAL(deteff_invalid) {
        this->spectraSkipped.push_back(inputWS->getAxis(1)->spectraNo(i));
        outputWS->maskWorkspaceIndex(i);
      }
    }

    // Do the correction
    DataObjects::EventList *evlist = outputWS->getEventListPtr(i);
    switch (evlist->getEventType()) {
    case API::TOF:
      // Switch to weights if needed.
      evlist->switchTo(API::WEIGHTED);
    // Fall through
    case API::WEIGHTED:
      eventHelper(evlist->getWeightedEvents(), exp_constant);
      break;
    case API::WEIGHTED_NOTIME:
      eventHelper(evlist->getWeightedEventsNoTime(), exp_constant);
      break;
    }

    this->progress->report();

    // check for canceling the algorithm
    if (i % 1000 == 0) {
      interruption_point();
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->clearMRU();

  this->logErrors();
}

/**
 * Private function for doing the event correction.
 * @param events :: the list of events to correct
 * @param expval :: the value of the exponent for the detector efficiency
 */
template <class T>
void He3TubeEfficiency::eventHelper(std::vector<T> &events, double expval) {
  const double scale = this->getProperty("ScaleFactor");
  typename std::vector<T>::iterator it;
  for (it = events.begin(); it != events.end(); ++it) {
    float de =
        static_cast<float>(this->detectorEfficiency(expval * it->tof(), scale));
    it->m_weight *= de;
    it->m_errorSquared *= de * de;
  }
}

} // namespace Algorithms
} // namespace Mantid
