// Includes
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/NDPseudoRandomNumberGenerator.h"
#include "MantidKernel/SobolSequence.h"
#include "MantidKernel/Timer.h"

#include <Poco/Timestamp.h>

namespace Mantid {
namespace MDAlgorithms {
using Geometry::Instrument;
using Geometry::Instrument_const_sptr;
using Geometry::IObjComponent_const_sptr;
using Geometry::IDetector_const_sptr;
using API::Run;
using API::IFunction;

DECLARE_MDRESOLUTIONCONVOLUTION(TobyFitResolutionModel,
                                "TobyFitResolutionModel")

namespace // anonymous
    {
/// Map strings to attributes names
const char *CRYSTAL_MOSAIC = "CrystalMosaic";
const char *MC_MIN_NAME = "MCLoopMin";
const char *MC_MAX_NAME = "MCLoopMax";
const char *MC_LOOP_TOL = "MCTolerance";
const char *MC_TYPE = "MCType";
const char *FOREGROUNDONLY_NAME = "ForegroundOnly";

/// static logger
Kernel::Logger g_log("TobyFitResolutionModel");
}

/*
 * Default constructor
 */
TobyFitResolutionModel::TobyFitResolutionModel()
    : MDResolutionConvolution(), m_randomNumbers(1, NULL), m_mcLoopMin(100),
      m_mcLoopMax(1000), m_mcType(4), m_mcRelErrorTol(1e-5),
      m_foregroundOnly(false), m_mosaicActive(true), m_bmatrix(1), m_yvector(1),
      m_etaInPlane(1, 0.0), m_etaOutPlane(1, 0.0),
      m_deltaQE(1, std::vector<double>(4, 0.0)), m_exptCache() {
  setupRandomNumberGenerator();
}

/**
 *  Construct with a model pointer
 *  @param fittedFunction :: A pointer to the function being fitted
 *  @param fgModel :: A pointer to a foreground model
 */
TobyFitResolutionModel::TobyFitResolutionModel(
    const API::IFunctionMD &fittedFunction, const std::string &fgModel)
    : MDResolutionConvolution(fittedFunction, fgModel),
      m_randomNumbers(1, NULL), m_mcLoopMin(100), m_mcLoopMax(1000),
      m_mcType(4), m_mcRelErrorTol(1e-5), m_foregroundOnly(false),
      m_mosaicActive(true), m_bmatrix(1), m_yvector(1), m_etaInPlane(1, 0.0),
      m_etaOutPlane(1, 0.0), m_deltaQE(1, std::vector<double>(4, 0.0)),
      m_exptCache() {
  setupRandomNumberGenerator();
}

/**
 * Destroy the object deleting the observation cache
 */
TobyFitResolutionModel::~TobyFitResolutionModel() {
  deleteRandomNumberGenerator();

  // Remove experiment cache
  auto iter = m_exptCache.begin();
  while (iter != m_exptCache.end()) {
    delete iter->second;       // Delete the observation itself
    m_exptCache.erase(iter++); // Post-increment to return old iterator to
                               // remove item from map
  }
}

/**
 * Returns the value of the cross-section convoluted with the resolution an
 * event. This assumes that
 * the box forms a 4D point with axes: Qx, Qy, Qz, \f$\Delta E\f$ in the
 * cartesian crystal frame
 * @param box :: An iterator pointing at the current box under examination
 * @param eventIndex :: An index of the current event in the box
 * @param innerRunIndex :: An index of the current run within the workspace.
 * This is NOT the run number. The experiment
 * can be retrieved using box->getExperimentInfo(innerRunIndex)
 * @returns the cross-section convoluted with the resolution
 */
double TobyFitResolutionModel::signal(const API::IMDIterator &box,
                                      const uint16_t innerRunIndex,
                                      const size_t eventIndex) const {
  auto iter = m_exptCache.find(std::make_pair(
      innerRunIndex,
      box.getInnerDetectorID(eventIndex))); // Guaranteed to exist
  const CachedExperimentInfo &detCachedExperimentInfo = *(iter->second);
  QOmegaPoint qCrystal(box, eventIndex);

  // Transform to spectrometer coordinates for resolution calculation
  // Done by hand to avoid expensive memory allocations when using Matrix
  // classes
  const Geometry::OrientedLattice &lattice =
      detCachedExperimentInfo.experimentInfo().sample().getOrientedLattice();
  const Kernel::DblMatrix &gr =
      detCachedExperimentInfo.experimentInfo().run().getGoniometerMatrix();
  const Kernel::DblMatrix &umat = lattice.getU();

  QOmegaPoint qLab(0.0, 0.0, 0.0, qCrystal.deltaE);
  for (unsigned int i = 0; i < 3; ++i) {
    qLab.qx += (gr[0][i] * umat[i][0] * qCrystal.qx +
                gr[0][i] * umat[i][1] * qCrystal.qy +
                gr[0][i] * umat[i][2] * qCrystal.qz);

    qLab.qy += (gr[1][i] * umat[i][0] * qCrystal.qx +
                gr[1][i] * umat[i][1] * qCrystal.qy +
                gr[1][i] * umat[i][2] * qCrystal.qz);

    qLab.qz += (gr[2][i] * umat[i][0] * qCrystal.qx +
                gr[2][i] * umat[i][1] * qCrystal.qy +
                gr[2][i] * umat[i][2] * qCrystal.qz);
  }

  if (m_foregroundOnly) {
    std::vector<double> &nominalQ = m_deltaQE[PARALLEL_THREAD_NUMBER];
    nominalQ[0] = qCrystal.qx;
    nominalQ[1] = qCrystal.qy;
    nominalQ[2] = qCrystal.qz;
    nominalQ[3] = qCrystal.deltaE;
    return foregroundModel().scatteringIntensity(
        detCachedExperimentInfo.experimentInfo(), nominalQ);
  }

  // -- Add in perturbations to nominal Q from instrument resolution --

  // Calculate the matrix of coefficients that contribute to the resolution
  // function (the B matrix in TobyFit).
  calculateResolutionCoefficients(detCachedExperimentInfo, qLab);

  // Pre calculate the transform inverse (RU) matrix elements
  double rb00(0.0), rb01(0.0), rb02(0.0), rb10(0.0), rb11(0.0), rb12(0.0),
      rb20(0.0), rb21(0.0), rb22(0.0);
  for (unsigned int i = 0; i < 3; ++i) {
    rb00 += gr[0][i] * umat[i][0];
    rb01 += gr[0][i] * umat[i][1];
    rb02 += gr[0][i] * umat[i][2];

    rb10 += gr[1][i] * umat[i][0];
    rb11 += gr[1][i] * umat[i][1];
    rb12 += gr[1][i] * umat[i][2];

    rb20 += gr[2][i] * umat[i][0];
    rb21 += gr[2][i] * umat[i][1];
    rb22 += gr[2][i] * umat[i][2];
  }
  const double determinant =
      (rb00 * (rb11 * rb22 - rb12 * rb21) - rb01 * (rb10 * rb22 - rb12 * rb20) +
       rb02 * (rb10 * rb21 - rb11 * rb20));

  // Start MC loop and check the relative error every min steps
  monteCarloLoopStarting();
  double sumSigma(0.0), sumSigmaSqr(0.0), avgSigma(0.0);
  for (int step = 1; step <= m_mcLoopMax; ++step) {
    generateIntegrationVariables(detCachedExperimentInfo, qLab);
    calculatePerturbedQE(detCachedExperimentInfo, qLab);

    std::vector<double> &q0 =
        m_deltaQE[PARALLEL_THREAD_NUMBER]; // Currently ordered beam,perp,up
                                           // (z,x,y)
    // Reorder to X,Y,Z
    std::swap(q0[0], q0[1]);
    std::swap(q0[1], q0[2]);

    // Transform to crystal frame for model
    // Need to tidy this up when we confirm it is correct
    const double qcx = ((rb11 * rb22 - rb12 * rb21) * q0[0] +
                        (rb02 * rb21 - rb01 * rb22) * q0[1] +
                        (rb01 * rb12 - rb02 * rb11) * q0[2]) /
                       determinant;
    const double qcy = ((rb12 * rb20 - rb10 * rb22) * q0[0] +
                        (rb00 * rb22 - rb02 * rb20) * q0[1] +
                        (rb02 * rb10 - rb00 * rb12) * q0[2]) /
                       determinant;
    const double qcz = ((rb10 * rb21 - rb11 * rb20) * q0[0] +
                        (rb01 * rb20 - rb00 * rb21) * q0[1] +
                        (rb00 * rb11 - rb01 * rb10) * q0[2]) /
                       determinant;
    q0[0] = qcx;
    q0[1] = qcy;
    q0[2] = qcz;

    // Compute weight from the foreground at this point
    const double weight = foregroundModel().scatteringIntensity(
        detCachedExperimentInfo.experimentInfo(), q0);
    // Add on this contribution to the average
    sumSigma += weight;
    sumSigmaSqr += weight * weight;

    avgSigma = sumSigma / step;
    if (checkForConvergence(step) &&
        hasConverged(step, sumSigma, sumSigmaSqr, avgSigma)) {
      break;
    }
  }

  return avgSigma;
}

//---------------------------------------------------------------------------------
// Private members
//---------------------------------------------------------------------------------

/**
 * Declare function attributes
 */
void TobyFitResolutionModel::declareAttributes() {
  // Resolution attributes, all on by default
  TobyFitYVector resolutionVector;
  resolutionVector.addAttributes(*this);
  // Crystal mosaic
  declareAttribute(CRYSTAL_MOSAIC,
                   IFunction::Attribute(m_mosaicActive ? 1 : 0));

  declareAttribute(MC_MIN_NAME, API::IFunction::Attribute(m_mcLoopMin));
  declareAttribute(MC_MAX_NAME, API::IFunction::Attribute(m_mcLoopMax));
  declareAttribute(MC_TYPE, API::IFunction::Attribute(m_mcType));
  declareAttribute(MC_LOOP_TOL, API::IFunction::Attribute(m_mcRelErrorTol));
  declareAttribute(FOREGROUNDONLY_NAME,
                   API::IFunction::Attribute(m_foregroundOnly ? 1 : 0));
}

/**
 *  Declare possible fitting parameters that will vary as fit progresses
 */
void TobyFitResolutionModel::declareParameters() {}

/**
 * Cache some frequently used attributes
 * @param name :: The name of the attribute
 * @param value :: It's value
 */
void
TobyFitResolutionModel::setAttribute(const std::string &name,
                                     const API::IFunction::Attribute &value) {
  MDResolutionConvolution::setAttribute(name, value);
  if (name == MC_MIN_NAME)
    m_mcLoopMin = value.asInt();
  else if (name == MC_MAX_NAME)
    m_mcLoopMax = value.asInt();
  else if (name == MC_LOOP_TOL)
    m_mcRelErrorTol = value.asDouble();
  else if (name == MC_TYPE) {
    m_mcType = value.asInt();
    if (m_mcType > 4 || m_mcType < 0) {
      throw std::invalid_argument("TobyFitResolutionModel: Invalid MCType "
                                  "argument, valid values are 0-4. Current "
                                  "value=" +
                                  boost::lexical_cast<std::string>(m_mcType));
    }
  } else if (name == CRYSTAL_MOSAIC) {
    m_mosaicActive = (value.asInt() != 0);
  } else if (name == FOREGROUNDONLY_NAME) {
    m_foregroundOnly = (value.asInt() != 0);
  } else {
    for (auto iter = m_yvector.begin(); iter != m_yvector.end(); ++iter) {
      iter->setAttribute(name, value);
    }
  }
}

/**
 * Calculate the values of the resolution coefficients from pg 112 of
 * T.G.Perring's thesis. It maps the
 * vector of randomly generated integration points to a vector of resolution
 * coefficients
 * @param observation :: The current observation defining the point experimental
 * setup
 * @param eventPoint :: The point in QE space that this refers to
 */
void TobyFitResolutionModel::calculateResolutionCoefficients(
    const CachedExperimentInfo &observation,
    const QOmegaPoint &eventPoint) const {
  m_bmatrix[PARALLEL_THREAD_NUMBER].recalculate(observation, eventPoint);
}

/**
 * Generates the Y vector of random deviates
 * @param observation :: The current observation defining the point experimental
 * setup
 * @param eventPoint :: The point in QE space that this refers to
 */
void TobyFitResolutionModel::generateIntegrationVariables(
    const CachedExperimentInfo &observation,
    const QOmegaPoint &eventPoint) const {
  const std::vector<double> &randomNums = generateRandomNumbers();
  const size_t nRandUsed = m_yvector[PARALLEL_THREAD_NUMBER].recalculate(
      randomNums, observation, eventPoint);

  // Calculate crystal mosaic contribution
  if (m_mosaicActive) {
    const double r1 = randomNums[nRandUsed];
    const double r2 = randomNums[nRandUsed + 1];
    const double small(1e-20);
    const double fwhhToStdDev =
        M_PI / 180. /
        std::sqrt(log(256.0)); // degrees FWHH -> st.dev. in radians

    const double prefactor = std::sqrt(-2.0 * std::log(std::max(r1, small)));
    const double etaSig =
        observation.experimentInfo().run().getLogAsSingleValue("eta_sigma") *
        fwhhToStdDev;

    m_etaInPlane[PARALLEL_THREAD_NUMBER] =
        etaSig * prefactor * std::cos(2.0 * M_PI * r2);
    m_etaOutPlane[PARALLEL_THREAD_NUMBER] =
        etaSig * prefactor * std::sin(2.0 * M_PI * r2);
  } else {
    m_etaInPlane[PARALLEL_THREAD_NUMBER] =
        m_etaOutPlane[PARALLEL_THREAD_NUMBER] = 0.0;
  }
}

/**
 * Returns the next set of random numbers
 */
const std::vector<double> &
TobyFitResolutionModel::generateRandomNumbers() const {
  return m_randomNumbers[PARALLEL_THREAD_NUMBER]->nextPoint();
}

/**
 * Calculates the point in Q-E space where the foreground model will be
 * evaluated.
 * @param observation :: The current observation defining the point experimental
 * setup
 * @param qOmega :: The point in QE space that this refers to
 */
void TobyFitResolutionModel::calculatePerturbedQE(
    const CachedExperimentInfo &observation, const QOmegaPoint &qOmega) const {
  // -- Calculate components of dKi & dKf, essentially X vector in TobyFit --

  /**
   * This function is called for each iteration of the Monte Carlo loop
   * and using the standard matrix algebra will produce a lot of repeated memory
   * allocations for creating new vectors/matrices.

   * The manual computation of the matrix multiplication is to avoid this
   overhead.
   */

  double xVec0(0.0), xVec1(0.0), xVec2(0.0), xVec3(0.0), xVec4(0.0), xVec5(0.0);
  const std::vector<double> &yvalues =
      m_yvector[PARALLEL_THREAD_NUMBER].values();
  const TobyFitBMatrix &bmatrix = m_bmatrix[PARALLEL_THREAD_NUMBER];
  for (unsigned int i = 0; i < TobyFitYVector::length(); ++i) {
    const double &yi = yvalues[i];
    xVec0 += bmatrix[0][i] * yi;
    xVec1 += bmatrix[1][i] * yi;
    xVec2 += bmatrix[2][i] * yi;
    xVec3 += bmatrix[3][i] * yi;
    xVec4 += bmatrix[4][i] * yi;
    xVec5 += bmatrix[5][i] * yi;
  }

  // Convert to dQ in lab frame
  // dQ = L*Xf, L = ((labToDet)^1)^T, Xf = outgoing components of X vector
  const Kernel::DblMatrix &D = observation.labToDetectorTransform();
  const double D00(D[0][0]), D01(D[0][1]), D02(D[0][2]), D10(D[1][0]),
      D11(D[1][1]), D12(D[1][2]), D20(D[2][0]), D21(D[2][1]), D22(D[2][2]);
  const double determinant = D00 * (D11 * D22 - D12 * D21) -
                             D01 * (D10 * D22 - D12 * D20) +
                             D02 * (D10 * D21 - D11 * D20);
  const double L00(D11 * D22 - D12 * D21), L01(D12 * D20 - D10 * D22),
      L02(D10 * D21 - D11 * D20), L10(D02 * D21 - D01 * D22),
      L11(D00 * D22 - D02 * D20), L12(D20 * D01 - D00 * D21),
      L20(D01 * D12 - D02 * D11), L21(D02 * D10 - D00 * D12),
      L22(D00 * D11 - D01 * D10);

  const double dqlab0 = (L22 * xVec3 + L02 * xVec4 + L12 * xVec5) / determinant;
  const double dqlab1 = (L20 * xVec3 + L00 * xVec4 + L10 * xVec5) / determinant;
  const double dqlab2 = (L21 * xVec3 + L01 * xVec4 + L11 * xVec5) / determinant;

  std::vector<double> &deltaQE = m_deltaQE[PARALLEL_THREAD_NUMBER];
  deltaQE[0] = (xVec0 - dqlab0);
  deltaQE[1] = (xVec1 - dqlab1);
  deltaQE[2] = (xVec2 - dqlab2);

  const double efixed = observation.getEFixed();
  const double wi =
      std::sqrt(efixed / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  const double wf = std::sqrt((efixed - qOmega.deltaE) /
                              PhysicalConstants::E_mev_toNeutronWavenumberSq);
  deltaQE[3] = 4.1442836 * (wi * xVec0 - wf * xVec3);

  if (m_mosaicActive) {
    const double &etaInPlane = m_etaInPlane[PARALLEL_THREAD_NUMBER];
    const double &etaOutPlane = m_etaOutPlane[PARALLEL_THREAD_NUMBER];

    const double qx(qOmega.qx + deltaQE[1]), qy(qOmega.qy + deltaQE[2]),
        qz(qOmega.qz + deltaQE[0]);
    const double qipmodSq = qy * qy + qz * qz;
    const double qmod = std::sqrt(qx * qx + qipmodSq);
    const double small(1e-10);
    if (qmod > small) {
      const double qipmod = std::sqrt(qipmodSq);
      if (qipmod > small) {
        deltaQE[0] -= qipmod * etaInPlane;
        deltaQE[1] +=
            ((qx * qy) * etaInPlane - (qz * qmod) * etaOutPlane) / qipmod;
        deltaQE[2] +=
            ((qx * qz) * etaInPlane + (qy * qmod) * etaOutPlane) / qipmod;
      } else {
        deltaQE[1] += qmod * etaInPlane;
        deltaQE[2] += qmod * etaOutPlane;
      }
    }
  }

  // Add on the nominal Q
  deltaQE[0] += qOmega.qz; // beam
  deltaQE[1] += qOmega.qx; // perp
  deltaQE[2] += qOmega.qy; // up

  deltaQE[3] += qOmega.deltaE;
}

/**
 * Return true if it is time to check for convergence of the
 * current sigma value
 * @param step :: The current step value
 * @return True if it is time to check for convergence
 */
bool TobyFitResolutionModel::checkForConvergence(const int step) const {
  return (step % m_mcLoopMin) == 0 || step == m_mcLoopMax;
}

/**
 * Returns true if the Monte Carlo loop should be broken. This occurs when
 *either the relative
 * error satisfies the tolerance or simulated value is zero after the minimum
 *number of steps
 *
 * @param step
 * @param sumSigma :: The current value of the sum of \f$\sigma\f$
 * @param sumSigmaSqr :: The current value of the sum of \f$\sigma^2\f$
 * @param avgSigma :: The current value of the monte carlo estimate of the true
 *\f$\sigma\f$ value,
 *                    i.e. \f$\sigma/step\f$
 * @return True if the sum has converged
 */
bool TobyFitResolutionModel::hasConverged(const int step, const double sumSigma,
                                          const double sumSigmaSqr,
                                          const double avgSigma) const {
  const double smallValue(1e-10);
  const double error = std::sqrt(
      std::fabs((sumSigmaSqr / step) - std::pow(sumSigma / step, 2)) / step);
  if (std::fabs(avgSigma) > smallValue) {
    const double relativeError = error / avgSigma;
    if (relativeError < m_mcRelErrorTol) {
      return true;
    }
  } else {
    // Value is considered zero after min number of steps
    // Probably as converged as we will get
    return true;
  }
  return false;
}

/// Called before a function evaluation begins
void TobyFitResolutionModel::functionEvalStarting() {
  // Ensure the random number generators are in the correct state
  // for future calls. This depends on the requested MCType.
  // See comments in setupRandomNumberGenerator for the reason why
  // there are two sets of generators
}

/// Called after a function evaluation is finished
void TobyFitResolutionModel::functionEvalFinished() {
  // See comments in TobyFitResolutionModel::functionEvalStarting()
}

/**
 * Called just before the Monte Carlo evaluation starts
 */
void TobyFitResolutionModel::monteCarloLoopStarting() const {
  if (m_mcType == 1) {
    m_randomNumbers[PARALLEL_THREAD_NUMBER]->restart();
  }
}

/**
 * Called before any fit/simulation is started to allow caching of
 * frequently used parameters
 * @param workspace :: The MD that will be used for the fit
 */
void TobyFitResolutionModel::preprocess(
    const API::IMDEventWorkspace_const_sptr &workspace) {
  Kernel::Timer timer;
  // Fill the observation cache
  auto iterator = workspace->createIterator();
  g_log.debug() << "Starting preprocessing loop\n";
  do {
    const size_t nevents = iterator->getNumEvents();
    for (size_t i = 0; i < nevents; ++i) {
      uint16_t innerRunIndex = iterator->getInnerRunIndex(i);
      detid_t detID = iterator->getInnerDetectorID(i);
      const auto key = std::make_pair(innerRunIndex, detID);
      if (m_exptCache.find(key) == m_exptCache.end()) {
        API::ExperimentInfo_const_sptr expt =
            workspace->getExperimentInfo(innerRunIndex);
        m_exptCache.insert(
            std::make_pair(key, new CachedExperimentInfo(*expt, detID)));
      }
    }
  } while (iterator->next());
  g_log.debug() << "Done preprocessing loop:" << timer.elapsed()
                << " seconds\n";
  delete iterator;
}

/**
 *  Called just before the fitting job starts. Sets up the
 *  random number generator
 */
void TobyFitResolutionModel::setUpForFit() {
  setNThreads(API::FrameworkManager::Instance().getNumOMPThreads());
  setupRandomNumberGenerator();
}

/**
 * Sets up the function to cope with the given number of threads processing it
 * at once
 * @param nthreads :: The maximum number of threads that will be used to
 * evaluate the
 * function
 */
void TobyFitResolutionModel::setNThreads(int nthreads) {
  if (nthreads <= 0)
    nthreads = 1; // Ensure we have a sensible number
  if (nthreads == 1)
    return; // done on construction

  m_randomNumbers =
      std::vector<Kernel::NDRandomNumberGenerator *>(nthreads, NULL);
  m_bmatrix = std::vector<TobyFitBMatrix>(
      nthreads, m_bmatrix[0]); // Initialize with copy of current
  m_yvector = std::vector<TobyFitYVector>(nthreads, m_yvector[0]);
  m_etaInPlane = std::vector<double>(nthreads, 0.0);
  m_etaOutPlane = std::vector<double>(nthreads, 0.0);
  m_deltaQE =
      std::vector<std::vector<double>>(nthreads, std::vector<double>(4, 0.0));
}

/**
 * Setup the random number generator based on the selected type
 */
void TobyFitResolutionModel::setupRandomNumberGenerator() {
  using namespace Mantid::Kernel;
  /**
   * As in TobyFit:
   *   mcType=0,2,4 -> pseudo-random number generator (we use MersenneTwister)
   *   mcType=1,3 -> quasi-random number generator (we use SobolSequence)
   *
   * Pseudo-Random:
   *   0 - Generator is restarted for each iteration
   *   2, 4 - Generator is NOT restarted at all throughout job
   *       but the partial derivatives need to use the same set
   *       of random numbers as the function evaluation so two
   *       sets of generators are used. At the end of the function
   *       evaluation the derivative one is swapped, which is precisely
   *       ncalls behind the first so will use the same set of numbers
   * Quasi-Random:
   *  1 - Generator is restarted for each pixel (handled in
   *monteCarloLoopStarting() callback)
   *  3 - As above for 2,4. Generator is NOT restarted but two sets must
   *      be used to ensure partial derivatives use the same set of deviates
   */
  // Clear out any old ones
  deleteRandomNumberGenerator();

  unsigned int nrand = m_yvector[0].requiredRandomNums();
  if (m_mosaicActive) {
    nrand += 2; // Extra 2 for mosaic
  }
  const size_t ngenerators(m_yvector.size());
  if (m_mcType % 2 == 0) // Pseudo-random
  {
    size_t seed(0);
    if (m_mcType == 0 || m_mcType == 2)
      seed = 1;
    else if (m_mcType == 4)
      seed = static_cast<size_t>(Poco::Timestamp().epochMicroseconds());

    typedef NDPseudoRandomNumberGenerator<MersenneTwister> NDMersenneTwister;
    for (size_t i = 0; i < ngenerators; ++i) {
      m_randomNumbers[i] = new NDMersenneTwister(nrand, seed, 0.0, 1.0);
    }
  } else // Quasi-random
  {
    for (size_t i = 0; i < ngenerators; ++i) {
      m_randomNumbers[i] = new Kernel::SobolSequence(nrand);
    }
  }
}

/**
 */
void TobyFitResolutionModel::deleteRandomNumberGenerator() {
  // Delete random number generator(s)
  auto iend = m_randomNumbers.end();
  for (auto it = m_randomNumbers.begin(); it != iend; ++it) {
    delete *it; // Delete pointer at given iterator location. vector stays same
                // size
  }
  // Leave the vector at the size it was
}
}
}
