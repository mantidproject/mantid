// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDNormSCD.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
// function to  compare two intersections (h,k,l,Momentum) by Momentum
bool compareMomentum(const std::array<double, 4> &v1,
                     const std::array<double, 4> &v2) {
  return (v1[3] < v2[3]);
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormSCD)

/**
 * Constructor
 */
MDNormSCD::MDNormSCD()
    : m_normWS(), m_inputWS(), m_hmin(0.0f), m_hmax(0.0f), m_kmin(0.0f),
      m_kmax(0.0f), m_lmin(0.0f), m_lmax(0.0f), m_hIntegrated(true),
      m_kIntegrated(true), m_lIntegrated(true), m_rubw(3, 3), m_kiMin(0.0),
      m_kiMax(EMPTY_DBL()), m_hIdx(-1), m_kIdx(-1), m_lIdx(-1), m_hX(), m_kX(),
      m_lX(), m_samplePos(), m_beamDir() {}

/// Algorithm's version for identification. @see Algorithm::version
int MDNormSCD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNormSCD::category() const {
  return "MDAlgorithms\\Normalisation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MDNormSCD::summary() const {
  return "Calculate normalization for an MDEvent workspace for single crystal "
         "diffraction.";
}

/// Algorithm's name for use in the GUI and help. @see Algorithm::name
const std::string MDNormSCD::name() const { return "MDNormSCD"; }

/**
 * Initialize the algorithm's properties.
 */
void MDNormSCD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDWorkspace.");

  std::string dimChars = getDimensionChars();
  // --------------- Axis-aligned properties
  // ---------------------------------------
  for (size_t i = 0; i < dimChars.size(); i++) {
    std::string dim(" ");
    dim[0] = dimChars[i];
    std::string propName = "AlignedDim" + dim;
    declareProperty(
        std::make_unique<PropertyWithValue<std::string>>(propName, "",
                                                            Direction::Input),
        "Binning parameters for the " + Strings::toString(i) +
            "th dimension.\n"
            "Enter it as a comma-separated list of values with the format: "
            "'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
  }

  auto fluxValidator = boost::make_shared<CompositeValidator>();
  fluxValidator->add<WorkspaceUnitValidator>("Momentum");
  fluxValidator->add<InstrumentValidator>();
  fluxValidator->add<CommonBinsValidator>();
  auto solidAngleValidator = fluxValidator->clone();

  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "FluxWorkspace", "", Direction::Input, fluxValidator),
                  "An input workspace containing momentum dependent flux.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "",
                                                   Direction::Input,
                                                   solidAngleValidator),
                  "An input workspace containing momentum integrated vanadium "
                  "(a measure of the solid angle).");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("SkipSafetyCheck", false,
                                                       Direction::Input),
                  "If set to true, the algorithm does "
                  "not check history if the workspace was modified since the"
                  "ConvertToMD algorithm was run, and assume that the elastic "
                  "mode is used.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "TemporaryNormalizationWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate normalization "
                  "from multiple MDEventWorkspaces. "
                  "If unspecified a blank MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "TemporaryDataWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate data from "
                  "multiple MDEventWorkspaces. If "
                  "unspecified a blank MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
}

/**
 * Execute the algorithm.
 */
void MDNormSCD::exec() {
  cacheInputs();
  auto outputWS = binInputWS();
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  outputWS->setDisplayNormalization(Mantid::API::NoNormalization);
  setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
  createNormalizationWS(*outputWS);
  m_normWS->setDisplayNormalization(Mantid::API::NoNormalization);
  setProperty("OutputNormalizationWorkspace", m_normWS);

  m_numExptInfos = outputWS->getNumExperimentInfo();
  // loop over all experiment infos
  for (uint16_t expInfoIndex = 0; expInfoIndex < m_numExptInfos;
       expInfoIndex++) {
    // Check for other dimensions if we could measure anything in the original
    // data
    bool skipNormalization = false;
    const std::vector<coord_t> otherValues =
        getValuesFromOtherDimensions(skipNormalization, expInfoIndex);
    const auto affineTrans =
        findIntergratedDimensions(otherValues, skipNormalization);
    cacheDimensionXValues();

    if (!skipNormalization) {
      calculateNormalization(otherValues, affineTrans, expInfoIndex);
    } else {
      g_log.warning("Binning limits are outside the limits of the MDWorkspace. "
                    "Not applying normalization.");
    }
    m_accumulate = true;
  }
}

/**
 * Set up starting values for cached variables
 */
void MDNormSCD::cacheInputs() {
  m_inputWS = getProperty("InputWorkspace");
  bool skipCheck = getProperty("SkipSafetyCheck");
  if (!skipCheck && inputEnergyMode() != "Elastic") {
    throw std::invalid_argument("Invalid energy transfer mode. Algorithm "
                                "currently only supports elastic data.");
  }
  // Min/max dimension values
  const auto hdim(m_inputWS->getDimension(0)), kdim(m_inputWS->getDimension(1)),
      ldim(m_inputWS->getDimension(2));
  m_hmin = hdim->getMinimum();
  m_kmin = kdim->getMinimum();
  m_lmin = ldim->getMinimum();
  m_hmax = hdim->getMaximum();
  m_kmax = kdim->getMaximum();
  m_lmax = ldim->getMaximum();

  const auto &exptInfoZero = *(m_inputWS->getExperimentInfo(0));
  auto source = exptInfoZero.getInstrument()->getSource();
  auto sample = exptInfoZero.getInstrument()->getSample();
  if (source == nullptr || sample == nullptr) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }
  m_samplePos = sample->getPos();
  m_beamDir = normalize(m_samplePos - source->getPos());
}

/**
 * Currently looks for the ConvertToMD algorithm in the history
 * @return A string donating the energy transfer mode of the input workspace
 */
std::string MDNormSCD::inputEnergyMode() const {
  const auto &hist = m_inputWS->getHistory();
  const size_t nalgs = hist.size();
  const auto &lastAlgorithm = hist.lastAlgorithm();

  std::string emode;
  if (lastAlgorithm->name() == "ConvertToMD") {
    emode = lastAlgorithm->getPropertyValue("dEAnalysisMode");
  } else if ((lastAlgorithm->name() == "Load" ||
              hist.lastAlgorithm()->name() == "LoadMD") &&
             hist.getAlgorithmHistory(nalgs - 2)->name() == "ConvertToMD") {
    // get dEAnalysisMode
    PropertyHistories histvec =
        hist.getAlgorithmHistory(nalgs - 2)->getProperties();
    for (auto &hist : histvec) {
      if (hist->name() == "dEAnalysisMode") {
        emode = hist->value();
        break;
      }
    }
  } else {
    throw std::invalid_argument("The last algorithm in the history of the "
                                "input workspace is not ConvertToMD");
  }
  return emode;
}

/**
 * Runs the BinMD algorithm on the input to provide the output workspace
 * All slicing algorithm properties are passed along
 * @return MDHistoWorkspace as a result of the binning
 */
MDHistoWorkspace_sptr MDNormSCD::binInputWS() {
  const auto &props = getProperties();
  IAlgorithm_sptr binMD = createChildAlgorithm("BinMD", 0.0, 0.3);
  binMD->setPropertyValue("AxisAligned", "1");
  for (auto prop : props) {
    const auto &propName = prop->name();
    if (propName != "FluxWorkspace" && propName != "SolidAngleWorkspace" &&
        propName != "TemporaryNormalizationWorkspace" &&
        propName != "OutputNormalizationWorkspace" &&
        propName != "SkipSafetyCheck") {
      binMD->setPropertyValue(propName, prop->value());
    }
  }
  binMD->executeAsChildAlg();
  Workspace_sptr outputWS = binMD->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
}

/**
 * Create & cached the normalization workspace
 * @param dataWS The binned workspace that will be used for the data
 */
void MDNormSCD::createNormalizationWS(const MDHistoWorkspace &dataWS) {
  // Copy the MDHisto workspace, and change signals and errors to 0.
  boost::shared_ptr<IMDHistoWorkspace> tmp =
      this->getProperty("TemporaryNormalizationWorkspace");
  m_normWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
  if (!m_normWS) {
    m_normWS = dataWS.clone();
    m_normWS->setTo(0., 0., 0.);
  } else {
    m_accumulate = true;
  }
}

/**
 * Retrieve logged values from non-HKL dimensions
 * @param skipNormalization [InOut] Updated to false if any values are outside
 * range measured by input workspace
 * @param expInfoIndex current experiment info index
 * @return A vector of values from other dimensions to be include in normalized
 * MD position calculation
 */
std::vector<coord_t>
MDNormSCD::getValuesFromOtherDimensions(bool &skipNormalization,
                                        uint16_t expInfoIndex) const {
  const auto &currentRun = m_inputWS->getExperimentInfo(expInfoIndex)->run();

  std::vector<coord_t> otherDimValues;
  for (size_t i = 3; i < m_inputWS->getNumDims(); i++) {
    const auto dimension = m_inputWS->getDimension(i);
    float dimMin = static_cast<float>(dimension->getMinimum());
    float dimMax = static_cast<float>(dimension->getMaximum());
    auto *dimProp = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        currentRun.getProperty(dimension->getName()));
    if (dimProp) {
      coord_t value = static_cast<coord_t>(dimProp->firstValue());
      otherDimValues.push_back(value);
      // in the original MD data no time was spent measuring between dimMin and
      // dimMax
      if (value < dimMin || value > dimMax) {
        skipNormalization = true;
      }
    }
  }
  return otherDimValues;
}

/**
 * Checks the normalization workspace against the indices of the original
 * dimensions.
 * If not found, the corresponding dimension is integrated
 * @param otherDimValues Values from non-HKL dimensions
 * @param skipNormalization [InOut] Sets the flag true if normalization values
 * are outside of original inputs
 * @return Affine trasform matrix
 */
Kernel::Matrix<coord_t>
MDNormSCD::findIntergratedDimensions(const std::vector<coord_t> &otherDimValues,
                                     bool &skipNormalization) {
  // Get indices of the original dimensions in the output workspace,
  // and if not found, the corresponding dimension is integrated
  Kernel::Matrix<coord_t> affineMat =
      m_normWS->getTransformFromOriginal(0)->makeAffineMatrix();

  const size_t nrm1 = affineMat.numRows() - 1;
  const size_t ncm1 = affineMat.numCols() - 1;
  for (size_t row = 0; row < nrm1; row++) // affine matrix, ignore last row
  {
    const auto dimen = m_normWS->getDimension(row);
    const auto dimMin(dimen->getMinimum()), dimMax(dimen->getMaximum());
    if (affineMat[row][0] == 1.) {
      m_hIntegrated = false;
      m_hIdx = row;
      m_hmin = std::max(m_hmin, dimMin);
      m_hmax = std::min(m_hmax, dimMax);
      if (m_hmin > dimMax || m_hmax < dimMin) {
        skipNormalization = true;
      }
    }
    if (affineMat[row][1] == 1.) {
      m_kIntegrated = false;
      m_kIdx = row;
      m_kmin = std::max(m_kmin, dimMin);
      m_kmax = std::min(m_kmax, dimMax);
      if (m_kmin > dimMax || m_kmax < dimMin) {
        skipNormalization = true;
      }
    }
    if (affineMat[row][2] == 1.) {
      m_lIntegrated = false;
      m_lIdx = row;
      m_lmin = std::max(m_lmin, dimMin);
      m_lmax = std::min(m_lmax, dimMax);
      if (m_lmin > dimMax || m_lmax < dimMin) {
        skipNormalization = true;
      }
    }

    for (size_t col = 3; col < ncm1; col++) // affine matrix, ignore last column
    {
      if (affineMat[row][col] == 1.) {
        double val = otherDimValues.at(col - 3);
        if (val > dimMax || val < dimMin) {
          skipNormalization = true;
        }
      }
    }
  }

  return affineMat;
}

/**
 * Stores the X values from each H,K,L dimension as member variables
 */
void MDNormSCD::cacheDimensionXValues() {
  if (!m_hIntegrated) {
    auto &hDim = *m_normWS->getDimension(m_hIdx);
    m_hX.resize(hDim.getNBoundaries());
    for (size_t i = 0; i < m_hX.size(); ++i) {
      m_hX[i] = hDim.getX(i);
    }
  }
  if (!m_kIntegrated) {
    auto &kDim = *m_normWS->getDimension(m_kIdx);
    m_kX.resize(kDim.getNBoundaries());
    for (size_t i = 0; i < m_kX.size(); ++i) {
      m_kX[i] = kDim.getX(i);
    }
  }
  if (!m_lIntegrated) {
    auto &lDim = *m_normWS->getDimension(m_lIdx);
    m_lX.resize(lDim.getNBoundaries());
    for (size_t i = 0; i < m_lX.size(); ++i) {
      m_lX[i] = lDim.getX(i);
    }
  }
}

/**
 * Computed the normalization for the input workspace. Results are stored in
 * m_normWS
 * @param otherValues
 * @param affineTrans
 * @param expInfoIndex current experiment info index
 */
void MDNormSCD::calculateNormalization(
    const std::vector<coord_t> &otherValues,
    const Kernel::Matrix<coord_t> &affineTrans, uint16_t expInfoIndex) {
  API::MatrixWorkspace_const_sptr integrFlux = getProperty("FluxWorkspace");
  integrFlux->getXMinMax(m_kiMin, m_kiMax);
  API::MatrixWorkspace_const_sptr solidAngleWS =
      getProperty("SolidAngleWorkspace");

  const auto &currentExptInfo = *(m_inputWS->getExperimentInfo(expInfoIndex));
  using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;
  auto *rubwLog = dynamic_cast<VectorDoubleProperty *>(
      currentExptInfo.getLog("RUBW_MATRIX"));
  if (!rubwLog) {
    throw std::runtime_error(
        "Wokspace does not contain a log entry for the RUBW matrix."
        "Cannot continue.");
  } else {
    Kernel::DblMatrix rubwValue(
        (*rubwLog)()); // includes the 2*pi factor but not goniometer for now :)
    m_rubw = currentExptInfo.run().getGoniometerMatrix() * rubwValue;
    m_rubw.Invert();
  }
  const double protonCharge = currentExptInfo.run().getProtonCharge();

  const auto &spectrumInfo = currentExptInfo.spectrumInfo();

  // Mappings
  const int64_t ndets = static_cast<int64_t>(spectrumInfo.size());
  const detid2index_map fluxDetToIdx =
      integrFlux->getDetectorIDToWorkspaceIndexMap();
  const detid2index_map solidAngDetToIdx =
      solidAngleWS->getDetectorIDToWorkspaceIndexMap();

  const size_t vmdDims = 4;
  std::vector<std::atomic<signal_t>> signalArray(m_normWS->getNPoints());
  std::vector<std::array<double, 4>> intersections;
  std::vector<double> xValues, yValues;
  std::vector<coord_t> pos, posNew;
  double progStep = 0.7 / m_numExptInfos;
  auto prog =
      std::make_unique<API::Progress>(this, 0.3 + progStep * expInfoIndex,
                                 0.3 + progStep * (expInfoIndex + 1.), ndets);
  // cppcheck-suppress syntaxError
PRAGMA_OMP(parallel for private(intersections, xValues, yValues, pos, posNew) if (Kernel::threadSafe(*integrFlux)))
for (int64_t i = 0; i < ndets; i++) {
  PARALLEL_START_INTERUPT_REGION

  if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) ||
      spectrumInfo.isMasked(i)) {
    continue;
  }
  const auto &detector = spectrumInfo.detector(i);
  double theta = detector.getTwoTheta(m_samplePos, m_beamDir);
  double phi = detector.getPhi();
  // If the dtefctor is a group, this should be the ID of the first detector
  const auto detID = detector.getID();

  // Intersections
  this->calculateIntersections(intersections, theta, phi);
  if (intersections.empty())
    continue;

  // get the flux spetrum number
  size_t wsIdx = fluxDetToIdx.find(detID)->second;
  // Get solid angle for this contribution
  double solid =
      solidAngleWS->y(solidAngDetToIdx.find(detID)->second)[0] * protonCharge;

  // -- calculate integrals for the intersection --
  // momentum values at intersections
  auto intersectionsBegin = intersections.begin();
  // copy momenta to xValues
  xValues.resize(intersections.size());
  yValues.resize(intersections.size());
  auto x = xValues.begin();
  for (auto it = intersectionsBegin; it != intersections.end(); ++it, ++x) {
    *x = (*it)[3];
  }
  // calculate integrals at momenta from xValues by interpolating between
  // points in spectrum sp
  // of workspace integrFlux. The result is stored in yValues
  calcIntegralsForIntersections(xValues, *integrFlux, wsIdx, yValues);

  // Compute final position in HKL
  // pre-allocate for efficiency and copy non-hkl dim values into place
  pos.resize(vmdDims + otherValues.size());
  std::copy(otherValues.begin(), otherValues.end(), pos.begin() + vmdDims - 1);
  pos.push_back(1.);

  for (auto it = intersectionsBegin + 1; it != intersections.end(); ++it) {
    const auto &curIntSec = *it;
    const auto &prevIntSec = *(it - 1);
    // the full vector isn't used so compute only what is necessary
    double delta = curIntSec[3] - prevIntSec[3];
    if (delta < 1e-07)
      continue; // Assume zero contribution if difference is small

    // Average between two intersections for final position
    std::transform(curIntSec.begin(), curIntSec.begin() + vmdDims - 1,
                   prevIntSec.begin(), pos.begin(),
                   [](const double lhs, const double rhs) {
                     return static_cast<coord_t>(0.5 * (lhs + rhs));
                   });
    affineTrans.multiplyPoint(pos, posNew);
    size_t linIndex = m_normWS->getLinearIndexAtCoord(posNew.data());
    if (linIndex == size_t(-1))
      continue;

    // index of the current intersection
    size_t k = static_cast<size_t>(std::distance(intersectionsBegin, it));
    // signal = integral between two consecutive intersections
    signal_t signal = (yValues[k] - yValues[k - 1]) * solid;
    Mantid::Kernel::AtomicOp(signalArray[linIndex], signal,
                             std::plus<signal_t>());
  }
  prog->report();

  PARALLEL_END_INTERUPT_REGION
}
PARALLEL_CHECK_INTERUPT_REGION
if (m_accumulate) {
  std::transform(
      signalArray.cbegin(), signalArray.cend(), m_normWS->getSignalArray(),
      m_normWS->getSignalArray(),
      [](const std::atomic<signal_t> &a, const signal_t &b) { return a + b; });
} else {
  std::copy(signalArray.cbegin(), signalArray.cend(),
            m_normWS->getSignalArray());
}
}

/**
 * Linearly interpolate between the points in integrFlux at xValues and save the
 * results in yValues.
 * @param xValues :: X-values at which to interpolate
 * @param integrFlux :: A workspace with the spectra to interpolate
 * @param sp :: A workspace index for a spectrum in integrFlux to interpolate.
 * @param yValues :: A vector to save the results.
 */
void MDNormSCD::calcIntegralsForIntersections(
    const std::vector<double> &xValues, const API::MatrixWorkspace &integrFlux,
    size_t sp, std::vector<double> &yValues) const {
  assert(xValues.size() == yValues.size());

  // the x-data from the workspace
  const auto &xData = integrFlux.x(sp);
  const double xStart = xData.front();
  const double xEnd = xData.back();

  // the values in integrFlux are expected to be integrals of a non-negative
  // function
  // ie they must make a non-decreasing function
  const auto &yData = integrFlux.y(sp);
  size_t spSize = yData.size();

  const double yMin = 0.0;
  const double yMax = yData.back();

  size_t nData = xValues.size();
  // all integrals below xStart must be 0
  if (xValues[nData - 1] < xStart) {
    std::fill(yValues.begin(), yValues.end(), yMin);
    return;
  }

  // all integrals above xEnd must be equal tp yMax
  if (xValues[0] > xEnd) {
    std::fill(yValues.begin(), yValues.end(), yMax);
    return;
  }

  size_t i = 0;
  // integrals below xStart must be 0
  while (i < nData - 1 && xValues[i] < xStart) {
    yValues[i] = yMin;
    i++;
  }
  size_t j = 0;
  for (; i < nData; i++) {
    // integrals above xEnd must be equal tp yMax
    if (j >= spSize - 1) {
      yValues[i] = yMax;
    } else {
      double xi = xValues[i];
      while (j < spSize - 1 && xi > xData[j])
        j++;
      // if x falls onto an interpolation point return the corresponding y
      if (xi == xData[j]) {
        yValues[i] = yData[j];
      } else if (j == spSize - 1) {
        // if we get above xEnd it's yMax
        yValues[i] = yMax;
      } else if (j > 0) {
        // interpolate between the consecutive points
        double x0 = xData[j - 1];
        double x1 = xData[j];
        double y0 = yData[j - 1];
        double y1 = yData[j];
        yValues[i] = y0 + (y1 - y0) * (xi - x0) / (x1 - x0);
      } else // j == 0
      {
        yValues[i] = yMin;
      }
    }
  }
}

/**
 * Calculate the points of intersection for the given detector with cuboid
 * surrounding the
 * detector position in HKL
 * @param intersections A list of intersections in HKL space
 * @param theta Polar angle withd detector
 * @param phi Azimuthal angle with detector
 */
void MDNormSCD::calculateIntersections(
    std::vector<std::array<double, 4>> &intersections, const double theta,
    const double phi) {
  V3D q(-sin(theta) * cos(phi), -sin(theta) * sin(phi), 1. - cos(theta));
  q = m_rubw * q;
  if (convention == "Crystallography") {
    q *= -1;
  }

  double hStart = q.X() * m_kiMin, hEnd = q.X() * m_kiMax;
  double kStart = q.Y() * m_kiMin, kEnd = q.Y() * m_kiMax;
  double lStart = q.Z() * m_kiMin, lEnd = q.Z() * m_kiMax;

  double eps = 1e-7;

  auto hNBins = m_hX.size();
  auto kNBins = m_kX.size();
  auto lNBins = m_lX.size();
  intersections.clear();
  intersections.reserve(hNBins + kNBins + lNBins + 8);

  // calculate intersections with planes perpendicular to h
  if (fabs(hStart - hEnd) > eps) {
    double fmom = (m_kiMax - m_kiMin) / (hEnd - hStart);
    double fk = (kEnd - kStart) / (hEnd - hStart);
    double fl = (lEnd - lStart) / (hEnd - hStart);
    if (!m_hIntegrated) {
      for (size_t i = 0; i < hNBins; i++) {
        double hi = m_hX[i];
        if ((hi >= m_hmin) && (hi <= m_hmax) &&
            ((hStart - hi) * (hEnd - hi) < 0)) {
          // if hi is between hStart and hEnd, then ki and li will be between
          // kStart, kEnd and lStart, lEnd and momi will be between m_kiMin and
          // KnincidemtmMax
          double ki = fk * (hi - hStart) + kStart;
          double li = fl * (hi - hStart) + lStart;
          if ((ki >= m_kmin) && (ki <= m_kmax) && (li >= m_lmin) &&
              (li <= m_lmax)) {
            double momi = fmom * (hi - hStart) + m_kiMin;
            intersections.push_back({{hi, ki, li, momi}});
          }
        }
      }
    }

    double momhMin = fmom * (m_hmin - hStart) + m_kiMin;
    if ((momhMin > m_kiMin) && (momhMin < m_kiMax)) {
      // khmin and lhmin
      double khmin = fk * (m_hmin - hStart) + kStart;
      double lhmin = fl * (m_hmin - hStart) + lStart;
      if ((khmin >= m_kmin) && (khmin <= m_kmax) && (lhmin >= m_lmin) &&
          (lhmin <= m_lmax)) {
        intersections.push_back({{m_hmin, khmin, lhmin, momhMin}});
      }
    }
    double momhMax = fmom * (m_hmax - hStart) + m_kiMin;
    if ((momhMax > m_kiMin) && (momhMax < m_kiMax)) {
      // khmax and lhmax
      double khmax = fk * (m_hmax - hStart) + kStart;
      double lhmax = fl * (m_hmax - hStart) + lStart;
      if ((khmax >= m_kmin) && (khmax <= m_kmax) && (lhmax >= m_lmin) &&
          (lhmax <= m_lmax)) {
        intersections.push_back({{m_hmax, khmax, lhmax, momhMax}});
      }
    }
  }

  // calculate intersections with planes perpendicular to k
  if (fabs(kStart - kEnd) > eps) {
    double fmom = (m_kiMax - m_kiMin) / (kEnd - kStart);
    double fh = (hEnd - hStart) / (kEnd - kStart);
    double fl = (lEnd - lStart) / (kEnd - kStart);
    if (!m_kIntegrated) {
      for (size_t i = 0; i < kNBins; i++) {
        double ki = m_kX[i];
        if ((ki >= m_kmin) && (ki <= m_kmax) &&
            ((kStart - ki) * (kEnd - ki) < 0)) {
          // if ki is between kStart and kEnd, then hi and li will be between
          // hStart, hEnd and lStart, lEnd
          double hi = fh * (ki - kStart) + hStart;
          double li = fl * (ki - kStart) + lStart;
          if ((hi >= m_hmin) && (hi <= m_hmax) && (li >= m_lmin) &&
              (li <= m_lmax)) {
            double momi = fmom * (ki - kStart) + m_kiMin;
            intersections.push_back({{hi, ki, li, momi}});
          }
        }
      }
    }

    double momkMin = fmom * (m_kmin - kStart) + m_kiMin;
    if ((momkMin > m_kiMin) && (momkMin < m_kiMax)) {
      // hkmin and lkmin
      double hkmin = fh * (m_kmin - kStart) + hStart;
      double lkmin = fl * (m_kmin - kStart) + lStart;
      if ((hkmin >= m_hmin) && (hkmin <= m_hmax) && (lkmin >= m_lmin) &&
          (lkmin <= m_lmax)) {
        intersections.push_back({{hkmin, m_kmin, lkmin, momkMin}});
      }
    }
    double momkMax = fmom * (m_kmax - kStart) + m_kiMin;
    if ((momkMax > m_kiMin) && (momkMax < m_kiMax)) {
      // hkmax and lkmax
      double hkmax = fh * (m_kmax - kStart) + hStart;
      double lkmax = fl * (m_kmax - kStart) + lStart;
      if ((hkmax >= m_hmin) && (hkmax <= m_hmax) && (lkmax >= m_lmin) &&
          (lkmax <= m_lmax)) {
        intersections.push_back({{hkmax, m_kmax, lkmax, momkMax}});
      }
    }
  }

  // calculate intersections with planes perpendicular to l
  if (fabs(lStart - lEnd) > eps) {
    double fmom = (m_kiMax - m_kiMin) / (lEnd - lStart);
    double fh = (hEnd - hStart) / (lEnd - lStart);
    double fk = (kEnd - kStart) / (lEnd - lStart);
    if (!m_lIntegrated) {
      for (size_t i = 0; i < lNBins; i++) {
        double li = m_lX[i];
        if ((li >= m_lmin) && (li <= m_lmax) &&
            ((lStart - li) * (lEnd - li) < 0)) {
          // if li is between lStart and lEnd, then hi and ki will be between
          // hStart, hEnd and kStart, kEnd
          double hi = fh * (li - lStart) + hStart;
          double ki = fk * (li - lStart) + kStart;
          if ((hi >= m_hmin) && (hi <= m_hmax) && (ki >= m_kmin) &&
              (ki <= m_kmax)) {
            double momi = fmom * (li - lStart) + m_kiMin;
            intersections.push_back({{hi, ki, li, momi}});
          }
        }
      }
    }

    double momlMin = fmom * (m_lmin - lStart) + m_kiMin;
    if ((momlMin > m_kiMin) && (momlMin < m_kiMax)) {
      // hlmin and klmin
      double hlmin = fh * (m_lmin - lStart) + hStart;
      double klmin = fk * (m_lmin - lStart) + kStart;
      if ((hlmin >= m_hmin) && (hlmin <= m_hmax) && (klmin >= m_kmin) &&
          (klmin <= m_kmax)) {
        intersections.push_back({{hlmin, klmin, m_lmin, momlMin}});
      }
    }
    double momlMax = fmom * (m_lmax - lStart) + m_kiMin;
    if ((momlMax > m_kiMin) && (momlMax < m_kiMax)) {
      // khmax and lhmax
      double hlmax = fh * (m_lmax - lStart) + hStart;
      double klmax = fk * (m_lmax - lStart) + kStart;
      if ((hlmax >= m_hmin) && (hlmax <= m_hmax) && (klmax >= m_kmin) &&
          (klmax <= m_kmax)) {
        intersections.push_back({{hlmax, klmax, m_lmax, momlMax}});
      }
    }
  }

  // add endpoints
  if ((hStart >= m_hmin) && (hStart <= m_hmax) && (kStart >= m_kmin) &&
      (kStart <= m_kmax) && (lStart >= m_lmin) && (lStart <= m_lmax)) {
    intersections.push_back({{hStart, kStart, lStart, m_kiMin}});
  }
  if ((hEnd >= m_hmin) && (hEnd <= m_hmax) && (kEnd >= m_kmin) &&
      (kEnd <= m_kmax) && (lEnd >= m_lmin) && (lEnd <= m_lmax)) {
    intersections.push_back({{hEnd, kEnd, lEnd, m_kiMax}});
  }

  // sort intersections by momentum
  std::stable_sort(intersections.begin(), intersections.end(), compareMomentum);
}

} // namespace MDAlgorithms
} // namespace Mantid
