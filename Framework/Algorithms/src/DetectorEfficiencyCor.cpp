// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace Mantid::Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(DetectorEfficiencyCor)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

namespace {

// E = KSquaredToE*K^2    KSquaredToE = (hbar^2)/(2*NeutronMass)
const double KSquaredToE = 2.07212466; // units of meV Angstrom^-2

const short NUMCOEFS = 25;
// series expansion coefficients copied from a fortran source code file
const double c_eff_f[] = {
    0.7648360390553052,     -0.3700950778935237,     0.1582704090813516,      -6.0170218669705407E-02,
    2.0465515957968953E-02, -6.2690181465706840E-03, 1.7408667184745830E-03,  -4.4101378999425122E-04,
    1.0252117967127217E-04, -2.1988904738111659E-05, 4.3729347905629990E-06,  -8.0998753944849788E-07,
    1.4031240949230472E-07, -2.2815971698619819E-08, 3.4943984983382137E-09,  -5.0562696807254781E-10,
    6.9315483353094009E-11, -9.0261598195695569E-12, 1.1192324844699897E-12,  -1.3204992654891612E-13,
    1.4100387524251801E-14, -8.6430862467068437E-16, -1.1129985821867194E-16, -4.5505266221823604E-16,
    3.8885561437496108E-16};

const double c_eff_g[] = {
    2.033429926215546,      -2.3123407369310212E-02, 7.0671915734894875E-03,  -7.5970017538257162E-04,
    7.4848652541832373E-05, 4.5642679186460588E-05,  -2.3097291253000307E-05, 1.9697221715275770E-06,
    2.4115259271262346E-06, -7.1302220919333692E-07, -2.5124427621592282E-07, 1.3246884875139919E-07,
    3.4364196805913849E-08, -2.2891359549026546E-08, -6.7281240212491156E-09, 3.8292458615085678E-09,
    1.6451021034313840E-09, -5.5868962123284405E-10, -4.2052310689211225E-10, 4.3217612266666094E-11,
    9.9547699528024225E-11, 1.2882834243832519E-11,  -1.9103066351000564E-11, -7.6805495297094239E-12,
    1.8568853399347773E-12};

// constants from the fortran code multiplied together sigref=143.23d0,
// wref=3.49416d0, atmref=10.0d0 const = 2.0*sigref*wref/atmref
const double g_helium_prefactor = 2.0 * 143.23 * 3.49416 / 10.0;

// this should be a big number but not so big that there are rounding errors
const double DIST_TO_UNIVERSE_EDGE = 1e3;

// Name of pressure parameter
const std::string PRESSURE_PARAM = "TubePressure";
// Name of wall thickness parameter
const std::string THICKNESS_PARAM = "TubeThickness";
} // namespace

// this default constructor calls default constructors and sets other member
// data to impossible (flag) values
DetectorEfficiencyCor::DetectorEfficiencyCor()
    : Algorithm(), m_inputWS(), m_outputWS(), m_paraMap(nullptr), m_Ei(-1.0), m_ki(-1.0), m_shapeCache(), m_samplePos(),
      m_spectraSkipped() {
  m_shapeCache.clear();
}

/**
 * Declare algorithm properties
 */
void DetectorEfficiencyCor::init() {
  auto val = std::make_shared<CompositeValidator>();
  val->add<WorkspaceUnitValidator>("DeltaE");
  val->add<HistogramValidator>();
  val->add<InstrumentValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, val),
                  "The workspace to correct for detector efficiency");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace in which to store the result. Each histogram "
                  "from the input workspace maps to a histogram in this workspace that has "
                  "just one value which indicates if there was a bad detector.");
  auto checkEi = std::make_shared<BoundedValidator<double>>();
  checkEi->setLower(0.0);
  declareProperty("IncidentEnergy", EMPTY_DBL(), checkEi,
                  "The energy of neutrons leaving the source as can be "
                  "calculated by :ref:`algm-GetEi`. If this value is provided, "
                  "uses property value, if it is not present, needs Ei log "
                  "value set on the workspace.");
}

/** Executes the algorithm
 *  @throw NullPointerException if a getDetector() returns NULL or pressure or
 * wall thickness is not set
 *  @throw invalid_argument if the shape of a detector is isn't a cylinder
 * aligned on axis or there is no baseInstrument
 */
void DetectorEfficiencyCor::exec() {
  // gets and checks the values passed to the algorithm
  retrieveProperties();

  // wave number that the neutrons originally had
  m_ki = std::sqrt(m_Ei / KSquaredToE);

  // Store some information about the instrument setup that will not change
  m_samplePos = m_inputWS->getInstrument()->getSample()->getPos();

  int64_t numHists = m_inputWS->getNumberHistograms();
  auto numHists_d = static_cast<double>(numHists);
  const auto progStep = static_cast<int64_t>(ceil(numHists_d / 100.0));
  auto const &spectrumInfo = m_inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int64_t i = 0; i < numHists; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    m_outputWS->setSharedX(i, m_inputWS->sharedX(i));
    try {
      correctForEfficiency(i, spectrumInfo);
    } catch (Exception::NotFoundError &) {
      // zero the Y data that can't be corrected
      m_outputWS->mutableY(i) *= 0.0;
      PARALLEL_CRITICAL(deteff_invalid) {
        m_spectraSkipped.insert(m_spectraSkipped.end(), m_inputWS->getAxis(1)->spectraNo(i));
      }
    }
    // make regular progress reports and check for canceling the algorithm
    if (i % progStep == 0) {
      progress(static_cast<double>(i) / numHists_d);
      interruption_point();
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  logErrors(numHists);
  setProperty("OutputWorkspace", m_outputWS);
}
/** Loads and checks the values passed to the algorithm
 *
 *  @throw invalid_argument if there is an incompatible property value so the
 *algorithm can't continue
 */
void DetectorEfficiencyCor::retrieveProperties() {
  // these first three properties are fully checked by validators
  m_inputWS = getProperty("InputWorkspace");
  m_paraMap = &(m_inputWS->constInstrumentParameters());

  m_Ei = getProperty("IncidentEnergy");
  // If we're not given an Ei, see if one has been set.
  if (m_Ei == EMPTY_DBL()) {
    if (m_inputWS->run().hasProperty("Ei")) {
      m_Ei = m_inputWS->run().getPropertyValueAsType<double>("Ei");
      g_log.debug() << "Using stored Ei value " << m_Ei << "\n";
    } else {
      throw std::invalid_argument("No Ei value has been set or stored within the run information.");
    }
  }

  m_outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != m_inputWS) {
    m_outputWS = create<MatrixWorkspace>(*m_inputWS);
  }
}

/**
 * Corrects a spectra for the detector efficiency calculated from detector
 * information
 * Gets the detector information and uses this to calculate its efficiency
 *  @param spectraIn :: index of the spectrum to get the efficiency for
 *  @param spectrumInfo :: The SpectrumInfo object for the input workspace
 *  @throw invalid_argument if the shape of a detector is not a cylinder aligned
 * along one axis
 *  @throw NotFoundError if the detector or its gas pressure or wall thickness
 * were not found
 */
void DetectorEfficiencyCor::correctForEfficiency(int64_t spectraIn, const SpectrumInfo &spectrumInfo) {
  if (!spectrumInfo.hasDetectors(spectraIn))
    throw Exception::NotFoundError("No detectors found", spectraIn);

  if (spectrumInfo.isMonitor(spectraIn) || spectrumInfo.isMasked(spectraIn)) {
    return;
  }

  auto &yout = m_outputWS->mutableY(spectraIn);
  auto &eout = m_outputWS->mutableE(spectraIn);
  // Need the original values so this is not a reference
  const auto yValues = m_inputWS->y(spectraIn);
  const auto eValues = m_inputWS->e(spectraIn);

  // Storage for the reciprocal wave vectors that are calculated as the
  // correction proceeds
  std::vector<double> oneOverWaveVectors(yValues.size());
  const auto &detectorInfo = m_inputWS->detectorInfo();
  const auto &spectrumDefinition = spectrumInfo.spectrumDefinition(spectraIn);

  for (const auto &index : spectrumDefinition) {
    const auto detIndex = index.first;
    const auto &det_member = detectorInfo.detector(detIndex);
    Parameter_sptr par = m_paraMap->getRecursive(det_member.getComponentID(), PRESSURE_PARAM);
    if (!par) {
      throw Exception::NotFoundError(PRESSURE_PARAM, spectraIn);
    }
    const double atms = par->value<double>();
    par = m_paraMap->getRecursive(det_member.getComponentID(), THICKNESS_PARAM);
    if (!par) {
      throw Exception::NotFoundError(THICKNESS_PARAM, spectraIn);
    }
    const double wallThickness = par->value<double>();
    double detRadius(0.0);
    V3D detAxis;
    getDetectorGeometry(det_member, detRadius, detAxis);

    // now get the sin of the angle, it's the magnitude of the cross product of
    // unit vector along the detector tube axis and a unit vector directed from
    // the sample to the detector centre
    const V3D vectorFromSample = normalize(det_member.getPos() - m_samplePos);
    Quat rot = det_member.getRotation();
    // rotate the original cylinder object axis to get the detector axis in the
    // actual instrument
    rot.rotate(detAxis);
    detAxis.normalize();
    // Scalar product is quicker than cross product
    double cosTheta = detAxis.scalar_prod(vectorFromSample);
    double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
    // Detector constant
    const double det_const = g_helium_prefactor * (detRadius - wallThickness) * atms / sinTheta;

    auto yinItr = yValues.cbegin();
    auto einItr = eValues.cbegin();
    auto youtItr = yout.begin();
    auto eoutItr = eout.begin();
    auto xItr = m_inputWS->x(spectraIn).cbegin();
    auto wavItr = oneOverWaveVectors.begin();

    for (; youtItr != yout.end(); ++youtItr, ++eoutItr) {
      if (index == spectrumDefinition[0]) {
        *youtItr = 0.0;
        *eoutItr = 0.0;
        *wavItr = calculateOneOverK(*xItr, *(xItr + 1));
      }
      const double oneOverWave = *wavItr;
      const auto nDets(static_cast<double>(spectrumDefinition.size()));
      const double factor = 1.0 / nDets / detectorEfficiency(det_const * oneOverWave);
      *youtItr += (*yinItr) * factor;
      *eoutItr += (*einItr) * factor;
      ++yinItr;
      ++einItr;
      ++xItr;
      ++wavItr;
    }
  }
}

/**
 * Calculates one over the wave number of a neutron based on a lower and upper
 * bin boundary
 * @param loBinBound :: A value interpreted as the lower bin bound of a
 * histogram
 * @param uppBinBound :: A value interpreted as the upper bin bound of a
 * histogram
 * @return The value of 1/K for this energy bin
 */
double DetectorEfficiencyCor::calculateOneOverK(double loBinBound, double uppBinBound) const {
  double energy = m_Ei - 0.5 * (uppBinBound + loBinBound);
  double oneOverKSquared = KSquaredToE / energy;
  return std::sqrt(oneOverKSquared);
}

/** Update the shape cache if necessary
 * @param det :: a pointer to the detector to query
 * @param detRadius :: An output parameter that contains the detector radius
 * @param detAxis :: An output parameter that contains the detector axis vector
 */
void DetectorEfficiencyCor::getDetectorGeometry(const Geometry::IDetector &det, double &detRadius, V3D &detAxis) {
  std::shared_ptr<const IObject> shape_sptr = det.shape();
  if (!shape_sptr->hasValidShape()) {
    throw Exception::NotFoundError("Shape", "Detector has no shape");
  }

  std::map<const Geometry::IObject *, std::pair<double, Kernel::V3D>>::const_iterator it =
      m_shapeCache.find(shape_sptr.get());
  if (it == m_shapeCache.end()) {
    double xDist = distToSurface(V3D(DIST_TO_UNIVERSE_EDGE, 0, 0), shape_sptr.get());
    double zDist = distToSurface(V3D(0, 0, DIST_TO_UNIVERSE_EDGE), shape_sptr.get());
    if (std::abs(zDist - xDist) < 1e-8) {
      detRadius = zDist / 2.0;
      detAxis = V3D(0, 1, 0);
      // assume radi in z and x and the axis is in the y
      PARALLEL_CRITICAL(deteff_shapecachea) {
        m_shapeCache.emplace(shape_sptr.get(), std::make_pair(detRadius, detAxis));
      }
      return;
    }
    double yDist = distToSurface(V3D(0, DIST_TO_UNIVERSE_EDGE, 0), shape_sptr.get());
    if (std::abs(yDist - zDist) < 1e-8) {
      detRadius = yDist / 2.0;
      detAxis = V3D(1, 0, 0);
      // assume that y and z are radi of the cylinder's circular cross-section
      // and the axis is perpendicular, in the x direction
      PARALLEL_CRITICAL(deteff_shapecacheb) {
        m_shapeCache.emplace(shape_sptr.get(), std::make_pair(detRadius, detAxis));
      }
      return;
    }

    if (std::abs(xDist - yDist) < 1e-8) {
      detRadius = xDist / 2.0;
      detAxis = V3D(0, 0, 1);
      PARALLEL_CRITICAL(deteff_shapecachec) {
        m_shapeCache.emplace(shape_sptr.get(), std::make_pair(detRadius, detAxis));
      }
      return;
    }
  } else {
    std::pair<double, V3D> geometry = it->second;
    detRadius = geometry.first;
    detAxis = geometry.second;
  }
}

/** For basic shapes centered on the origin (0,0,0) this returns the distance to
 * the surface in
 *  the direction of the point given
 *  @param start :: the distance calculated from origin to the surface in a line
 * towards this point. It should be outside the shape
 *  @param shape :: the object to calculate for, should be centered on the
 * origin
 *  @return the distance to the surface in the direction of the point given
 *  @throw invalid_argument if there is any error finding the distance
 * @returns The distance to the surface in meters
 */
double DetectorEfficiencyCor::distToSurface(const V3D &start, const IObject *shape) const {
  // get a vector from the point that was passed to the origin
  const V3D direction = normalize(-start);
  // put the point and the vector (direction) together to get a line, here
  // called a track
  Track track(start, direction);
  // split the track (line) up into the part that is inside the shape and the
  // part that is outside
  shape->interceptSurface(track);

  if (track.count() != 1) { // the track missed the shape, probably the shape is
                            // not centered on the origin
    throw std::invalid_argument("Fatal error interpreting the shape of a detector");
  }
  // the first part of the track will be the part inside the shape, return its
  // length
  return track.cbegin()->distInsideObject;
}

/** Calculates detector efficiency, copied from the fortran code in
 * effic_3he_cylinder.for
 *  @param alpha :: From T.G.Perring's effic_3he_cylinder.for: alpha =
 * const*rad*(1.0d0-t2rad)*atms/wvec
 *  @return detector efficiency
 */
double DetectorEfficiencyCor::detectorEfficiency(const double alpha) const {
  if (alpha < 9.0) {
    return 0.25 * M_PI * alpha * chebevApprox(0.0, 10.0, c_eff_f, alpha);
  }
  if (alpha > 10.0) {
    double y = 1.0 - 18.0 / alpha;
    return 1.0 - chebevApprox(-1.0, 1.0, c_eff_g, y) / (alpha * alpha);
  }
  double eff_f = 0.25 * M_PI * alpha * chebevApprox(0.0, 10.0, c_eff_f, alpha);
  double y = 1.0 - 18.0 / alpha;
  double eff_g = 1.0 - chebevApprox(-1.0, 1.0, c_eff_g, y) / (alpha * alpha);
  return (10.0 - alpha) * eff_f + (alpha - 9.0) * eff_g;
}

/** Calculates an expansion similar to that in CHEBEV of "Numerical Recipes"
 *  copied from the fortran code in effic_3he_cylinder.for
 * @param a :: a fit parameter, only the difference between a and b enters this
 * equation
 * @param b :: a fit parameter, only the difference between a and b enters this
 * equation
 * @param exspansionCoefs :: one of the 25 element constant arrays declared in
 * this file
 * @param x :: a fit parameter
 * @return a numerical approximation provided by the expansion
 */
double DetectorEfficiencyCor::chebevApprox(double a, double b, const double exspansionCoefs[], double x) const {
  double d = 0.0;
  double dd = 0.0;
  double y = (2.0 * x - a - b) / (b - a);
  double y2 = 2.0 * y;
  for (int j = NUMCOEFS - 1; j > 0; j -= 1) {
    double sv = d;
    d = y2 * d - dd + exspansionCoefs[j];
    dd = sv;
  }
  return y * d - dd + 0.5 * exspansionCoefs[0];
}

/**
 * Logs if there were any problems locating spectra.
 * @param totalNDetectors -- number of all detectors in the workspace
 *
 */
void DetectorEfficiencyCor::logErrors(size_t totalNDetectors) const {
  std::vector<int>::size_type nspecs = m_spectraSkipped.size();
  if (!m_spectraSkipped.empty()) {
    g_log.warning() << "There were " << nspecs
                    << " spectra that could not be corrected out of total: " << totalNDetectors << '\n';
    g_log.warning() << "Their spectra were nullified\n";
    g_log.debug() << " Nullified spectra numbers: ";
    auto itend = m_spectraSkipped.end();
    for (auto it = m_spectraSkipped.begin(); it != itend; ++it) {
      g_log.debug() << *it << " ";
    }
    g_log.debug() << "\n";
  }
}

} // namespace Mantid::Algorithms
