#include "MantidCrystal/PredictPeaks.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/EdgePixel.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

#include <fstream>
using Mantid::Kernel::EnabledWhenProperty;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PredictPeaks)

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/// Small helper function that return -1 if convention
/// is "Crystallography" and 1 otherwise.
double get_factor_for_q_convention(const std::string &convention) {
  if (convention == "Crystallography") {
    return -1.0;
  }

  return 1.0;
}
} // namespace

/** Constructor
 */
PredictPeaks::PredictPeaks()
    : m_runNumber(-1), m_inst(), m_pw(), m_sfCalculator(),
      m_qConventionFactor(get_factor_for_q_convention(
          ConfigService::Instance().getString("Q.convention"))) {
  m_refConds = getAllReflectionConditions();
}

/** Initialize the algorithm's properties.
 */
void PredictPeaks::init() {
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace (MatrixWorkspace, MDEventWorkspace, or "
                  "PeaksWorkspace) containing:\n"
                  "  - The relevant Instrument (calibrated as needed).\n"
                  "  - A sample with a UB matrix.\n"
                  "  - The goniometer rotation matrix.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("WavelengthMin", 0.1,
                                             Direction::Input),
      "Minimum wavelength limit at which to start looking for single-crystal "
      "peaks.");
  declareProperty(
      make_unique<PropertyWithValue<double>>("WavelengthMax", 100.0,
                                             Direction::Input),
      "Maximum wavelength limit at which to stop looking for single-crystal "
      "peaks.");

  declareProperty(make_unique<PropertyWithValue<double>>("MinDSpacing", 1.0,
                                                         Direction::Input),
                  "Minimum d-spacing of peaks to consider. Default = 1.0");
  declareProperty(make_unique<PropertyWithValue<double>>("MaxDSpacing", 100.0,
                                                         Direction::Input),
                  "Maximum d-spacing of peaks to consider.");

  declareProperty("CalculateGoniometerForCW", false,
                  "This will calculate the goniometer rotation (around y-axis "
                  "only) for a constant wavelength.");

  auto nonNegativeDbl = boost::make_shared<BoundedValidator<double>>();
  nonNegativeDbl->setLower(0);
  declareProperty("Wavelength", DBL_MAX, nonNegativeDbl,
                  "Wavelength to use when calculating goniometer angle");
  setPropertySettings(
      "Wavelength", make_unique<EnabledWhenProperty>("CalculateGoniometerForCW",
                                                     IS_NOT_DEFAULT));

  declareProperty(make_unique<PropertyWithValue<double>>("MinAngle", -180,
                                                         Direction::Input),
                  "Minimum goniometer rotation angle");
  setPropertySettings(
      "MinAngle", make_unique<EnabledWhenProperty>("CalculateGoniometerForCW",
                                                   IS_NOT_DEFAULT));

  declareProperty(
      make_unique<PropertyWithValue<double>>("MaxAngle", 180, Direction::Input),
      "Maximum goniometer rotation angle");
  setPropertySettings(
      "MaxAngle", make_unique<EnabledWhenProperty>("CalculateGoniometerForCW",
                                                   IS_NOT_DEFAULT));

  // Build up a list of reflection conditions to use
  std::vector<std::string> propOptions;
  for (auto &refCond : m_refConds)
    propOptions.push_back(refCond->getName());
  declareProperty("ReflectionCondition", "Primitive",
                  boost::make_shared<StringListValidator>(propOptions),
                  "Which reflection condition applies to this crystal, "
                  "reducing the number of expected HKL peaks?");

  declareProperty("CalculateStructureFactors", false,
                  "Calculate structure factors for the predicted peaks. This "
                  "option only works if the sample of the input workspace has "
                  "a crystal structure assigned.");

  declareProperty(
      make_unique<WorkspaceProperty<PeaksWorkspace>>(
          "HKLPeaksWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: An input PeaksWorkspace with the HKL of the peaks "
      "that we should predict. \n"
      "The WavelengthMin/Max and Min/MaxDSpacing parameters are "
      "unused if this is specified.");

  declareProperty("RoundHKL", true,
                  "When using HKLPeaksWorkspace, this will round the HKL "
                  "values in the HKLPeaksWorkspace to the nearest integers if "
                  "checked.\n"
                  "Keep unchecked to use the original values");

  setPropertySettings("RoundHKL", make_unique<EnabledWhenProperty>(
                                      "HKLPeaksWorkspace", IS_NOT_DEFAULT));

  // Disable some props when using HKLPeaksWorkspace
  auto makeSet = [] {
    std::unique_ptr<IPropertySettings> set =
        make_unique<EnabledWhenProperty>("HKLPeaksWorkspace", IS_DEFAULT);
    return set;
  };
  setPropertySettings("WavelengthMin", makeSet());
  setPropertySettings("WavelengthMax", makeSet());
  setPropertySettings("MinDSpacing", makeSet());
  setPropertySettings("MaxDSpacing", makeSet());
  setPropertySettings("ReflectionCondition", makeSet());

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output PeaksWorkspace.");

  declareProperty("PredictPeaksOutsideDetectors", false,
                  "Use an extended detector space (if defined for the"
                  " instrument) to predict peaks which do not fall onto any"
                  "detector. This may produce a very high number of results.");

  auto nonNegativeInt = boost::make_shared<BoundedValidator<int>>();
  nonNegativeInt->setLower(0);
  declareProperty("EdgePixels", 0, nonNegativeInt,
                  "Remove peaks that are at pixels this close to edge. ");
}

/** Execute the algorithm.
 */
void PredictPeaks::exec() {
  // Get the input properties
  Workspace_sptr rawInputWorkspace = getProperty("InputWorkspace");
  m_edge = this->getProperty("EdgePixels");

  ExperimentInfo_sptr inputExperimentInfo =
      boost::dynamic_pointer_cast<ExperimentInfo>(rawInputWorkspace);

  MatrixWorkspace_sptr matrixWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(rawInputWorkspace);
  PeaksWorkspace_sptr peaksWS =
      boost::dynamic_pointer_cast<PeaksWorkspace>(rawInputWorkspace);
  MultipleExperimentInfos_sptr mdWS =
      boost::dynamic_pointer_cast<MultipleExperimentInfos>(rawInputWorkspace);

  std::vector<DblMatrix> gonioVec;
  if (matrixWS) {
    // Retrieve the goniometer rotation matrix
    try {
      DblMatrix goniometerMatrix = matrixWS->run().getGoniometerMatrix();
      gonioVec.push_back(goniometerMatrix);
    } catch (std::runtime_error &e) {
      // If there is no goniometer matrix, use identity matrix instead.
      g_log.error() << "Error getting the goniometer rotation matrix from the "
                       "InputWorkspace.\n"
                    << e.what() << '\n';
      g_log.warning() << "Using identity goniometer rotation matrix instead.\n";
    }
  } else if (peaksWS) {
    // Sort peaks by run number so that peaks with equal goniometer matrices are
    // adjacent
    std::vector<std::pair<std::string, bool>> criteria;
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));

    peaksWS->sort(criteria);

    // Get all goniometer matrices
    DblMatrix lastGoniometerMatrix = Matrix<double>(3, 3, false);
    for (int i = 0; i < static_cast<int>(peaksWS->getNumberPeaks()); ++i) {
      IPeak &p = peaksWS->getPeak(i);
      DblMatrix currentGoniometerMatrix = p.getGoniometerMatrix();
      if (!(currentGoniometerMatrix == lastGoniometerMatrix)) {
        gonioVec.push_back(currentGoniometerMatrix);
        lastGoniometerMatrix = currentGoniometerMatrix;
      }
    }

  } else if (mdWS) {
    if (mdWS->getNumExperimentInfo() <= 0)
      throw std::invalid_argument(
          "Specified a MDEventWorkspace as InputWorkspace but it does not have "
          "any ExperimentInfo associated. Please choose a workspace with a "
          "full instrument and sample.");

    inputExperimentInfo = mdWS->getExperimentInfo(0);

    // Retrieve the goniometer rotation matrices for each experiment info
    for (uint16_t i = 0; i < mdWS->getNumExperimentInfo(); ++i) {
      try {
        DblMatrix goniometerMatrix =
            mdWS->getExperimentInfo(i)->mutableRun().getGoniometerMatrix();
        gonioVec.push_back(goniometerMatrix);
      } catch (std::runtime_error &e) {
        // If there is no goniometer matrix, use identity matrix instead.
        gonioVec.push_back(DblMatrix(3, 3, true));

        g_log.error()
            << "Error getting the goniometer rotation matrix from the "
               "InputWorkspace.\n"
            << e.what() << '\n';
        g_log.warning()
            << "Using identity goniometer rotation matrix instead.\n";
      }
    }
  }

  // If there's no goniometer matrix at this point, push back an identity
  // matrix.
  if (gonioVec.empty()) {
    gonioVec.push_back(DblMatrix(3, 3, true));
  }

  setInstrumentFromInputWorkspace(inputExperimentInfo);
  setRunNumberFromInputWorkspace(inputExperimentInfo);
  setReferenceFrameAndBeamDirection();
  checkBeamDirection();

  // Create the output
  m_pw = boost::make_shared<PeaksWorkspace>();

  // Copy instrument, sample, etc.
  m_pw->copyExperimentInfoFrom(inputExperimentInfo.get());

  const Sample &sample = inputExperimentInfo->sample();

  // Retrieve the OrientedLattice (UnitCell) from the workspace
  const OrientedLattice &orientedLattice = sample.getOrientedLattice();

  // Get the UB matrix from it
  Matrix<double> ub(3, 3, true);
  ub = orientedLattice.getUB();

  std::vector<V3D> possibleHKLs;
  PeaksWorkspace_sptr possibleHKLWorkspace = getProperty("HKLPeaksWorkspace");

  if (!possibleHKLWorkspace) {
    fillPossibleHKLsUsingGenerator(orientedLattice, possibleHKLs);
  } else {
    fillPossibleHKLsUsingPeaksWorkspace(possibleHKLWorkspace, possibleHKLs);
  }

  setStructureFactorCalculatorFromSample(sample);

  /* The wavelength filtering can not be done before because it depends
   * on q being correctly oriented, so an additional filtering step is required.
   */
  double lambdaMin = getProperty("WavelengthMin");
  double lambdaMax = getProperty("WavelengthMax");

  Progress prog(this, 0.0, 1.0, possibleHKLs.size() * gonioVec.size());
  prog.setNotifyStep(0.01);

  m_detectorCacheSearch =
      Kernel::make_unique<DetectorSearcher>(m_inst, m_pw->detectorInfo());

  if (getProperty("CalculateGoniometerForCW")) {
    size_t allowedPeakCount = 0;
    int seqNum = 1;

    double wavelength = getProperty("Wavelength");
    if (wavelength == DBL_MAX) {
      if (m_inst->hasParameter("wavelength")) {
        wavelength = m_inst->getNumberParameter("wavelength").at(0);
      } else {
        throw std::runtime_error(
            "Could not get wavelength, neither Wavelength algorithm property "
            "set nor instrument wavelength parameter");
      }
    }
    double angleMin = getProperty("MinAngle");
    double angleMax = getProperty("MaxAngle");
    for (auto &possibleHKL : possibleHKLs) {
      Geometry::Goniometer goniometer;
      V3D q_sample = ub * possibleHKL * (2.0 * M_PI * m_qConventionFactor);
      goniometer.calcFromQSampleAndWavelength(q_sample, wavelength);
      double angle = goniometer.getEulerAngles()[0];
      if (!std::isfinite(angle) || angle < angleMin || angle > angleMax)
        continue;
      DblMatrix orientedUB = goniometer.getR() * ub;
      V3D q_lab = orientedUB * possibleHKL;
      double lambda = (2.0 * q_lab.Z()) / (q_lab.norm2());
      if (std::abs(wavelength - lambda) < 0.01) {
        g_log.information() << "Found goniometer rotation to be "
                            << goniometer.getEulerAngles()[0]
                            << " degrees for HKL = " << possibleHKL << "\n";
        calculateQAndAddToOutput(possibleHKL, orientedUB, goniometer.getR(),
                                 seqNum);
        ++allowedPeakCount;
      }
      prog.report();
    }

    logNumberOfPeaksFound(allowedPeakCount);

  } else {
    for (auto &goniometerMatrix : gonioVec) {
      // Final transformation matrix (HKL to Q in lab frame)
      DblMatrix orientedUB = goniometerMatrix * ub;

      /* Because of the additional filtering step it's better to keep track of
       * the allowed peaks with a counter. */
      HKLFilterWavelength lambdaFilter(orientedUB, lambdaMin, lambdaMax);

      size_t allowedPeakCount = 0;
      int seqNum = 1;

      bool useExtendedDetectorSpace =
          getProperty("PredictPeaksOutsideDetectors");
      if (useExtendedDetectorSpace &&
          !m_inst->getComponentByName("extended-detector-space")) {
        g_log.warning() << "Attempting to find peaks outside of detectors but "
                           "no extended detector space has been defined\n";
      }

      for (auto &possibleHKL : possibleHKLs) {
        if (lambdaFilter.isAllowed(possibleHKL)) {
          calculateQAndAddToOutput(possibleHKL, orientedUB, goniometerMatrix,
                                   seqNum);
          ++allowedPeakCount;
        }
        prog.report();
      }

      logNumberOfPeaksFound(allowedPeakCount);
    }
  }

  setProperty<PeaksWorkspace_sptr>("OutputWorkspace", m_pw);
}

/**
 * Log the number of peaks found to fall on and off detectors
 *
 * @param allowedPeakCount :: number of candidate peaks found
 */
void PredictPeaks::logNumberOfPeaksFound(size_t allowedPeakCount) const {
  const bool usingExtendedDetectorSpace =
      getProperty("PredictPeaksOutsideDetectors");
  const auto &peaks = m_pw->getPeaks();
  size_t offDetectorPeakCount = 0;
  size_t onDetectorPeakCount = 0;

  for (const auto &peak : peaks) {
    if (peak.getDetectorID() == -1) {
      offDetectorPeakCount++;
    } else {
      onDetectorPeakCount++;
    }
  }

  g_log.notice() << "Out of " << allowedPeakCount
                 << " allowed peaks within parameters, " << onDetectorPeakCount
                 << " were found to hit a detector";

  if (usingExtendedDetectorSpace) {
    g_log.notice() << " and " << offDetectorPeakCount << " were found in "
                   << "extended detector space.";
  }

  g_log.notice() << "\n";
}

/// Tries to set the internally stored instrument from an ExperimentInfo-object.
void PredictPeaks::setInstrumentFromInputWorkspace(
    const ExperimentInfo_sptr &inWS) {
  // Check that there is an input workspace that has a sample.
  if (!inWS || !inWS->getInstrument())
    throw std::invalid_argument("Did not specify a valid InputWorkspace with a "
                                "full instrument.");

  m_inst = inWS->getInstrument();
}

/// Sets the run number from the supplied ExperimentInfo or throws an exception.
void PredictPeaks::setRunNumberFromInputWorkspace(
    const ExperimentInfo_sptr &inWS) {
  if (!inWS) {
    throw std::runtime_error("Failed to get run number");
  }

  m_runNumber = inWS->getRunNumber();
}

/// Checks that the beam direction is +Z, throws exception otherwise.
void PredictPeaks::checkBeamDirection() const {
  V3D samplePos = m_inst->getSample()->getPos();

  // L1 path and direction
  V3D beamDir = m_inst->getSource()->getPos() - samplePos;

  if ((fabs(beamDir.X()) > 1e-2) ||
      (fabs(beamDir.Y()) > 1e-2)) // || (beamDir.Z() < 0))
    throw std::invalid_argument("Instrument must have a beam direction that "
                                "is only in the +Z direction for this "
                                "algorithm to be valid..");
}

/// Fills possibleHKLs with all HKLs that are allowed within d- and
/// lambda-limits.
void PredictPeaks::fillPossibleHKLsUsingGenerator(
    const OrientedLattice &orientedLattice,
    std::vector<V3D> &possibleHKLs) const {
  double dMin = getProperty("MinDSpacing");
  double dMax = getProperty("MaxDSpacing");

  // --- Reflection condition ----
  // Use the primitive by default
  ReflectionCondition_sptr refCond =
      boost::make_shared<ReflectionConditionPrimitive>();
  // Get it from the property
  std::string refCondName = getPropertyValue("ReflectionCondition");
  for (const auto &m_refCond : m_refConds)
    if (m_refCond->getName() == refCondName)
      refCond = m_refCond;

  HKLGenerator gen(orientedLattice, dMin);
  auto filter =
      boost::make_shared<HKLFilterCentering>(refCond) &
      boost::make_shared<HKLFilterDRange>(orientedLattice, dMin, dMax);

  V3D hklMin = *(gen.begin());

  g_log.information() << "HKL range for d_min of " << dMin << " to d_max of "
                      << dMax << " is from " << hklMin << " to "
                      << hklMin * -1.0 << ", a total of " << gen.size()
                      << " possible HKL's\n";

  if (gen.size() > 10000000000)
    throw std::invalid_argument("More than 10 billion HKLs to search. Is "
                                "your d_min value too small?");

  possibleHKLs.clear();
  possibleHKLs.reserve(gen.size());
  std::remove_copy_if(gen.begin(), gen.end(), std::back_inserter(possibleHKLs),
                      (~filter)->fn());
}

/// Fills possibleHKLs with all HKLs from the supplied PeaksWorkspace.
void PredictPeaks::fillPossibleHKLsUsingPeaksWorkspace(
    const PeaksWorkspace_sptr &peaksWorkspace,
    std::vector<V3D> &possibleHKLs) const {
  possibleHKLs.clear();
  possibleHKLs.reserve(peaksWorkspace->getNumberPeaks());

  bool roundHKL = getProperty("RoundHKL");

  /* Q is at the end multiplied with the factor determined in the
   * constructor (-1 for crystallography, 1 otherwise). So to avoid
   * "flippling HKLs" when it's not required, the HKLs of the input
   * workspace are also multiplied by the factor that is appropriate
   * for the convention stored in the workspace.
   */
  double peaks_q_convention_factor =
      get_factor_for_q_convention(peaksWorkspace->getConvention());

  for (int i = 0; i < static_cast<int>(peaksWorkspace->getNumberPeaks()); ++i) {
    IPeak &p = peaksWorkspace->getPeak(i);
    // Get HKL from that peak
    V3D hkl = p.getHKL() * peaks_q_convention_factor;

    if (roundHKL)
      hkl.round();

    possibleHKLs.push_back(hkl);
  } // for each hkl in the workspace
}

/**
 * @brief Assigns a StructureFactorCalculator if available in sample.
 *
 * This method constructs a StructureFactorCalculator using the CrystalStructure
 * stored in sample if available. For consistency it sets the OrientedLattice
 * in the sample as the unit cell of the crystal structure.
 *
 * Additionally, the property CalculateStructureFactors is taken into account.
 * If it's disabled, the calculator will not be assigned, disabling structure
 * factor calculation.
 *
 * @param sample :: Sample, potentially with crystal structure
 */
void PredictPeaks::setStructureFactorCalculatorFromSample(
    const Sample &sample) {
  bool calculateStructureFactors = getProperty("CalculateStructureFactors");

  if (calculateStructureFactors && sample.hasCrystalStructure()) {
    CrystalStructure crystalStructure = sample.getCrystalStructure();
    crystalStructure.setCell(sample.getOrientedLattice());

    m_sfCalculator = StructureFactorCalculatorFactory::create<
        StructureFactorCalculatorSummation>(crystalStructure);
  }
}

/**
 * @brief Calculates Q from HKL and adds a peak to the output workspace
 *
 * This method takes HKL and uses the oriented UB matrix (UB multiplied by the
 * goniometer matrix) to calculate Q. It then creates a Peak-object using
 * that Q-vector and the internally stored instrument. If the corresponding
 * diffracted beam intersects with a detector, the peak is added to the output-
 * workspace.
 *
 * @param hkl
 * @param orientedUB
 * @param goniometerMatrix
 * @param seqNum
 */
void PredictPeaks::calculateQAndAddToOutput(const V3D &hkl,
                                            const DblMatrix &orientedUB,
                                            const DblMatrix &goniometerMatrix,
                                            int &seqNum) {
  // The q-vector direction of the peak is = goniometer * ub * hkl_vector
  // This is in inelastic convention: momentum transfer of the LATTICE!
  // Also, q does have a 2pi factor = it is equal to 2pi/wavelength.
  const auto q = orientedUB * hkl * (2.0 * M_PI * m_qConventionFactor);
  const auto params = getPeakParametersFromQ(q);
  const auto detectorDir = std::get<0>(params);
  const auto wl = std::get<1>(params);

  const bool useExtendedDetectorSpace =
      getProperty("PredictPeaksOutsideDetectors");
  const auto result = m_detectorCacheSearch->findDetectorIndex(q);
  const auto hitDetector = std::get<0>(result);
  const auto index = std::get<1>(result);

  if (!hitDetector && !useExtendedDetectorSpace) {
    return;
  }

  const auto &detInfo = m_pw->detectorInfo();
  const auto &det = detInfo.detector(index);
  std::unique_ptr<Peak> peak;

  if (hitDetector) {
    // peak hit a detector to add it to the list
    peak = Kernel::make_unique<Peak>(m_inst, det.getID(), wl);
    if (!peak->getDetector())
      return;

  } else if (useExtendedDetectorSpace) {
    // use extended detector space to try and guess peak position
    const auto returnedComponent =
        m_inst->getComponentByName("extended-detector-space");
    // Check that the component is valid
    const auto component =
        boost::dynamic_pointer_cast<const ObjComponent>(returnedComponent);
    if (!component)
      throw std::runtime_error("PredictPeaks: user requested use of a extended "
                               "detector space to predict peaks but there is no"
                               "definition in the IDF");

    // find where this Q vector should intersect with "extended" space
    Geometry::Track track(detInfo.samplePosition(), detectorDir);
    if (!component->interceptSurface(track))
      return;

    // The exit point is the vector to the place that we hit a detector
    const auto magnitude = track.back().exitPoint.norm();
    peak = Kernel::make_unique<Peak>(m_inst, q,
                                     boost::optional<double>(magnitude));
  }

  if (m_edge > 0 && edgePixel(m_inst, peak->getBankName(), peak->getCol(),
                              peak->getRow(), m_edge))
    return;

  // Only add peaks that hit the detector
  peak->setGoniometerMatrix(goniometerMatrix);
  // Save the run number found before.
  peak->setRunNumber(m_runNumber);
  peak->setHKL(hkl * m_qConventionFactor);
  peak->setPeakNumber(seqNum);
  seqNum++;

  if (m_sfCalculator) {
    peak->setIntensity(m_sfCalculator->getFSquared(hkl));
  }

  // Add it to the workspace
  m_pw->addPeak(*peak);
}

/** Get the detector direction and wavelength of a peak from it's QLab vector
 *
 * @param q :: the q lab vector for this peak
 * @return a tuple containing the detector direction and the wavelength
 */
std::tuple<V3D, double>
PredictPeaks::getPeakParametersFromQ(const V3D &q) const {
  double norm_q = q.norm();
  // Default for ki-kf has -q
  const double qBeam = q.scalar_prod(m_refBeamDir) * m_qConventionFactor;
  double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);
  double wl = (2.0 * M_PI) / one_over_wl;
  // Default for ki-kf has -q
  V3D detectorDir = q * -m_qConventionFactor;
  detectorDir[m_refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
  detectorDir.normalize();
  return std::make_tuple(detectorDir, wl);
}

/** Cache the reference frame and beam direction using the instrument
 */
void PredictPeaks::setReferenceFrameAndBeamDirection() {
  m_refFrame = m_inst->getReferenceFrame();
  m_refBeamDir = m_refFrame->vecPointingAlongBeam();
}

} // namespace Crystal
} // namespace Mantid
