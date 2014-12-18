#include "MantidMDAlgorithms/MDNormDirectSC.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
// function to  compare two intersections (h,k,l,Momentum) by Momentum
bool compareMomentum(const Mantid::Kernel::VMD &v1,
                     const Mantid::Kernel::VMD &v2) {
  return (v1[3] < v2[3]);
}
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormDirectSC)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
MDNormDirectSC::MDNormDirectSC()
    : m_normWS(), m_inputWS(), m_hmin(0.0f), m_hmax(0.0f), m_kmin(0.0f),
      m_kmax(0.0f), m_lmin(0.0f), m_lmax(0.0f), m_dEmin(0.f), m_dEmax(0.f),
      m_Ei(0.), m_ki(0.), m_kfmin(0.), m_kfmax(0.), m_hIntegrated(true),
      m_kIntegrated(true), m_lIntegrated(true), m_dEIntegrated(false),
      m_rubw(3, 3), m_hIdx(-1), m_kIdx(-1), m_lIdx(-1), m_eIdx(-1), m_hX(),
      m_kX(), m_lX(), m_eX(), m_samplePos(), m_beamDir() {}

/// Algorithm's version for identification. @see Algorithm::version
int MDNormDirectSC::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNormDirectSC::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MDNormDirectSC::summary() const {
  return "Calculate normalization for an MDEvent workspace for single crystal "
         "direct geometry inelastic measurement.";
}

/// Algorithm's name for use in the GUI and help. @see Algorithm::name
const std::string MDNormDirectSC::name() const { return "MDNormDirectSC"; }

//----------------------------------------------------------------------------------------------
/**
  * Initialize the algorithm's properties.
  */
void MDNormDirectSC::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input MDWorkspace.");

  std::string dimChars = getDimensionChars();
  // --------------- Axis-aligned properties
  // ---------------------------------------
  for (size_t i = 0; i < dimChars.size(); i++) {
    std::string dim(" ");
    dim[0] = dimChars[i];
    std::string propName = "AlignedDim" + dim;
    declareProperty(
        new PropertyWithValue<std::string>(propName, "", Direction::Input),
        "Binning parameters for the " + Strings::toString(i) +
            "th dimension.\n"
            "Enter it as a comma-separated list of values with the format: "
            "'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
  }

  auto solidAngleValidator = boost::make_shared<CompositeValidator>();
  solidAngleValidator->add<InstrumentValidator>();
  solidAngleValidator->add<CommonBinsValidator>();

  declareProperty(
      new WorkspaceProperty<>("SolidAngleWorkspace", "", Direction::Input,
                              PropertyMode::Optional, solidAngleValidator),
      "An input workspace containing integrated vanadium (a measure of the "
      "solid angle).");

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
  declareProperty(new WorkspaceProperty<Workspace>(
                      "OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void MDNormDirectSC::exec() {
  cacheInputs();
  auto outputWS = binInputWS();
  setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
  createNormalizationWS(*outputWS);
  setProperty("OutputNormalizationWorkspace", m_normWS);

  // Check for other dimensions if we could measure anything in the original
  // data
  bool skipNormalization = false;
  const std::vector<coord_t> otherValues =
      getValuesFromOtherDimensions(skipNormalization);
  const auto affineTrans =
      findIntergratedDimensions(otherValues, skipNormalization);
  cacheDimensionXValues();

  if (!skipNormalization) {
    calculateNormalization(otherValues, affineTrans);
  } else {
    g_log.warning("Binning limits are outside the limits of the MDWorkspace. "
                  "Not applying normalization.");
  }
}

/**
 * Set up starting values for cached variables
 */
void MDNormDirectSC::cacheInputs() {
  m_inputWS = getProperty("InputWorkspace");
  if (inputEnergyMode() != "Direct") {
    throw std::invalid_argument("Invalid energy transfer mode. Algorithm only "
                                "supports direct geometry spectrometers.");
  }
  // Min/max dimension values
  const auto hdim(m_inputWS->getDimension(0)), kdim(m_inputWS->getDimension(1)),
      ldim(m_inputWS->getDimension(2)), edim(m_inputWS->getDimension(3));
  m_hmin = hdim->getMinimum();
  m_kmin = kdim->getMinimum();
  m_lmin = ldim->getMinimum();
  m_dEmin = edim->getMinimum();
  m_hmax = hdim->getMaximum();
  m_kmax = kdim->getMaximum();
  m_lmax = ldim->getMaximum();
  m_dEmax = edim->getMaximum();

  const auto &exptInfoZero = *(m_inputWS->getExperimentInfo(0));
  auto source = exptInfoZero.getInstrument()->getSource();
  auto sample = exptInfoZero.getInstrument()->getSample();
  if (source == NULL || sample == NULL) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }
  m_samplePos = sample->getPos();
  m_beamDir = m_samplePos - source->getPos();
  m_beamDir.normalize();

  double originaldEmin = exptInfoZero.run().getBinBoundaries().front();
  double originaldEmax = exptInfoZero.run().getBinBoundaries().back();
  if (exptInfoZero.run().hasProperty("Ei")) {
    Kernel::Property *eiprop = exptInfoZero.run().getProperty("Ei");
    m_Ei = boost::lexical_cast<double>(eiprop->value());
    if (m_Ei <= 0) {
      throw std::invalid_argument("Ei stored in the workspace is not positive");
    }
  } else {
    throw std::invalid_argument("Could not find Ei value in the workspace.");
  }
  double eps = 1e-7;
  if (m_Ei - originaldEmin < eps) {
    originaldEmin = m_Ei - eps;
  }
  if (m_Ei - originaldEmax < eps) {
    originaldEmax = m_Ei - 1e-7;
  }
  if (originaldEmin == originaldEmax) {
    throw std::runtime_error("The limits of the original workspace used in "
                             "ConvertToMD are incorrect");
  }
  const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass *
                           PhysicalConstants::meV * 1e-20 /
                           (PhysicalConstants::h * PhysicalConstants::h);
  m_ki = std::sqrt(energyToK * m_Ei);
  m_kfmin = std::sqrt(energyToK * (m_Ei - originaldEmin));
  m_kfmax = std::sqrt(energyToK * (m_Ei - originaldEmax));
}

/**
 * Currently looks for the ConvertToMD algorithm in the history
 * @return A string donating the energy transfer mode of the input workspace
 */
std::string MDNormDirectSC::inputEnergyMode() const {
  const auto &hist = m_inputWS->getHistory();
  const size_t nalgs = hist.size();
  const auto &lastAlgorithm = hist.lastAlgorithm();

  std::string emode("");
  if (lastAlgorithm->name() == "ConvertToMD") {
    emode = lastAlgorithm->getPropertyValue("dEAnalysisMode");
  } else if ((lastAlgorithm->name() == "Load" ||
              hist.lastAlgorithm()->name() == "LoadMD") &&
             hist.getAlgorithmHistory(nalgs - 2)->name() == "ConvertToMD") {
    // get dEAnalysisMode
    PropertyHistories histvec =
        hist.getAlgorithmHistory(nalgs - 2)->getProperties();
    for (auto it = histvec.begin(); it != histvec.end(); ++it) {
      if ((*it)->name() == "dEAnalysisMode") {
        emode = (*it)->value();
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
MDHistoWorkspace_sptr MDNormDirectSC::binInputWS() {
  const auto &props = getProperties();
  IAlgorithm_sptr binMD = createChildAlgorithm("BinMD", 0.0, 0.3);
  binMD->setPropertyValue("AxisAligned", "1");
  for (auto it = props.begin(); it != props.end(); ++it) {
    const auto &propName = (*it)->name();
    if (propName != "SolidAngleWorkspace" &&
        propName != "OutputNormalizationWorkspace") {
      binMD->setPropertyValue(propName, (*it)->value());
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
void MDNormDirectSC::createNormalizationWS(const MDHistoWorkspace &dataWS) {
  // Copy the MDHisto workspace, and change signals and errors to 0.
  m_normWS = boost::make_shared<MDHistoWorkspace>(dataWS);
  m_normWS->setTo(0., 0., 0.);
}

/**
 * Retrieve logged values from non-HKL dimensions
 * @param skipNormalization [InOut] Updated to false if any values are outside
 * range measured by input workspace
 * @return A vector of values from other dimensions to be include in normalized
 * MD position calculation
 */
std::vector<coord_t>
MDNormDirectSC::getValuesFromOtherDimensions(bool &skipNormalization) const {
  const auto &runZero = m_inputWS->getExperimentInfo(0)->run();

  std::vector<coord_t> otherDimValues;
  for (size_t i = 4; i < m_inputWS->getNumDims(); i++) {
    const auto dimension = m_inputWS->getDimension(i);
    float dimMin = static_cast<float>(dimension->getMinimum());
    float dimMax = static_cast<float>(dimension->getMaximum());
    auto *dimProp = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        runZero.getProperty(dimension->getName()));
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
Kernel::Matrix<coord_t> MDNormDirectSC::findIntergratedDimensions(
    const std::vector<coord_t> &otherDimValues, bool &skipNormalization) {
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
      if (m_hmin < dimMin)
        m_hmin = dimMin;
      if (m_hmax > dimMax)
        m_hmax = dimMax;
      if (m_hmin > dimMax || m_hmax < dimMin) {
        skipNormalization = true;
      }
    }
    if (affineMat[row][1] == 1.) {
      m_kIntegrated = false;
      m_kIdx = row;
      if (m_kmin < dimMin)
        m_kmin = dimMin;
      if (m_kmax > dimMax)
        m_kmax = dimMax;
      if (m_kmin > dimMax || m_kmax < dimMin) {
        skipNormalization = true;
      }
    }
    if (affineMat[row][2] == 1.) {
      m_lIntegrated = false;
      m_lIdx = row;
      if (m_lmin < dimMin)
        m_lmin = dimMin;
      if (m_lmax > dimMax)
        m_lmax = dimMax;
      if (m_lmin > dimMax || m_lmax < dimMin) {
        skipNormalization = true;
      }
    }

    if (affineMat[row][3] == 1.) {
      m_dEIntegrated = false;
      m_eIdx = row;
      if (m_dEmin < dimMin)
        m_dEmin = dimMin;
      if (m_dEmax > dimMax)
        m_dEmax = dimMax;
      if (m_dEmin > dimMax || m_dEmax < dimMin) {
        skipNormalization = true;
      }
    }
    for (size_t col = 4; col < ncm1; col++) // affine matrix, ignore last column
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
void MDNormDirectSC::cacheDimensionXValues() {
  const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass *
                           PhysicalConstants::meV * 1e-20 /
                           (PhysicalConstants::h * PhysicalConstants::h);
  if (!m_hIntegrated) {
    auto &hDim = *m_normWS->getDimension(m_hIdx);
    m_hX.resize(hDim.getNBins());
    for (size_t i = 0; i < m_hX.size(); ++i) {
      m_hX[i] = hDim.getX(i);
    }
  }
  if (!m_kIntegrated) {
    auto &kDim = *m_normWS->getDimension(m_kIdx);
    m_kX.resize(kDim.getNBins());
    for (size_t i = 0; i < m_kX.size(); ++i) {
      m_kX[i] = kDim.getX(i);
    }
  }
  if (!m_lIntegrated) {
    auto &lDim = *m_normWS->getDimension(m_lIdx);
    m_lX.resize(lDim.getNBins());
    for (size_t i = 0; i < m_lX.size(); ++i) {
      m_lX[i] = lDim.getX(i);
    }
  }
  if (!m_dEIntegrated) {
    // NOTE: store k final instead
    auto &eDim = *m_normWS->getDimension(m_eIdx);
    m_eX.resize(eDim.getNBins());
    for (size_t i = 0; i < m_eX.size(); ++i) {
      double temp = m_Ei - eDim.getX(i);
      if (temp <= 0)
        temp = 0.;
      m_eX[i] = std::sqrt(energyToK * (temp));
    }
  }
}

/**
 * Computed the normalization for the input workspace. Results are stored in
 * m_normWS
 * @param otherValues
 * @param affineTrans
 */
void MDNormDirectSC::calculateNormalization(
    const std::vector<coord_t> &otherValues,
    const Kernel::Matrix<coord_t> &affineTrans) {
  const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass *
                           PhysicalConstants::meV * 1e-20 /
                           (PhysicalConstants::h * PhysicalConstants::h);
  const auto &exptInfoZero = *(m_inputWS->getExperimentInfo(0));
  typedef Kernel::PropertyWithValue<std::vector<double> > VectorDoubleProperty;
  auto *rubwLog =
      dynamic_cast<VectorDoubleProperty *>(exptInfoZero.getLog("RUBW_MATRIX"));
  if (!rubwLog) {
    throw std::runtime_error(
        "Wokspace does not contain a log entry for the RUBW matrix."
        "Cannot continue.");
  } else {
    Kernel::DblMatrix rubwValue(
        (*rubwLog)()); // includes the 2*pi factor but not goniometer for now :)
    m_rubw = exptInfoZero.run().getGoniometerMatrix() * rubwValue;
    m_rubw.Invert();
  }
  const double protonCharge = exptInfoZero.run().getProtonCharge();

  auto instrument = exptInfoZero.getInstrument();
  std::vector<detid_t> detIDs = instrument->getDetectorIDs(true);
  // Prune out those that are part of a group and simply leave the head of the
  // group
  detIDs = removeGroupedIDs(exptInfoZero, detIDs);

  // Mapping
  const int64_t ndets = static_cast<int64_t>(detIDs.size());
  bool haveSA = false;
  API::MatrixWorkspace_const_sptr solidAngleWS =
      getProperty("SolidAngleWorkspace");
  detid2index_map solidAngDetToIdx;
  if (solidAngleWS != NULL) {
    haveSA = true;
    solidAngDetToIdx = solidAngleWS->getDetectorIDToWorkspaceIndexMap();
  }

  auto *prog = new API::Progress(this, 0.3, 1.0, ndets);
  // PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < ndets; i++) {
    // PARALLEL_START_INTERUPT_REGION

    const auto detID = detIDs[i];
    double theta(0.0), phi(0.0);
    bool skip(false);
    try {
      auto spectrum = getThetaPhi(detID, exptInfoZero, theta, phi);
      if (spectrum->isMonitor() || spectrum->isMasked())
        continue;
    }
    catch (std::exception &) // detector might not exist or has no been included
                             // in grouping
    {
      skip = true; // Intel compiler has a problem with continue inside a catch
                   // inside openmp...
    }
    if (skip)
      continue;

    // Intersections
    auto intersections = calculateIntersections(theta, phi);
    if (intersections.empty())
      continue;

    // Get solid angle for this contribution
    double solid = protonCharge;
    if (haveSA) {
      solid = solidAngleWS->readY(solidAngDetToIdx.find(detID)->second)[0] *
              protonCharge;
    }
    // Compute final position in HKL
    const size_t vmdDims = intersections.front().size();
    // pre-allocate for efficiency and copy non-hkl dim values into place
    std::vector<coord_t> pos(vmdDims + otherValues.size() + 1);
    std::copy(otherValues.begin(), otherValues.end(), pos.begin() + vmdDims);
    pos.push_back(1.);
    auto intersectionsBegin = intersections.begin();
    for (auto it = intersectionsBegin + 1; it != intersections.end(); ++it) {
      const auto &curIntSec = *it;
      const auto &prevIntSec = *(it - 1);
      // the full vector isn't used so compute only what is necessary
      double delta =
          (curIntSec[3] * curIntSec[3] - prevIntSec[3] * prevIntSec[3]) /
          energyToK;
      if (delta < 1e-10)
        continue; // Assume zero contribution if difference is small

      // Average between two intersections for final position
      std::transform(curIntSec.getBareArray(),
                     curIntSec.getBareArray() + vmdDims,
                     prevIntSec.getBareArray(), pos.begin(),
                     VectorHelper::SimpleAverage<coord_t>());

      // transform kf to energy transfer
      pos[3] = static_cast<coord_t>(m_Ei - pos[3] * pos[3] / energyToK);
      std::vector<coord_t> posNew = affineTrans * pos;
      size_t linIndex = m_normWS->getLinearIndexAtCoord(posNew.data());
      if (linIndex == size_t(-1))
        continue;

      // signal = integral between two consecutive intersections *solid angle
      // *PC
      double signal = solid * delta;

      // PARALLEL_CRITICAL(updateMD)
      {
        signal += m_normWS->getSignalAt(linIndex);
        m_normWS->setSignalAt(linIndex, signal);
      }
    }
    prog->report();

    // PARALLEL_END_INTERUPT_REGION
  }
  // PARALLEL_CHECK_INTERUPT_REGION

  delete prog;
}

/**
 * Checks for IDs that are actually part of the same group and just keeps one
 * from the group.
 * For a 1:1 map, none will be removed.
 * @param exptInfo An ExperimentInfo object that defines the grouping
 * @param detIDs A list of existing IDs
 * @return A new list of IDs
 */
std::vector<detid_t>
MDNormDirectSC::removeGroupedIDs(const ExperimentInfo &exptInfo,
                                 const std::vector<detid_t> &detIDs) {
  const size_t ntotal = detIDs.size();
  std::vector<detid_t> singleIDs;
  singleIDs.reserve(ntotal / 2); // reserve half. In the case of 1:1 it will
                                 // double to the correct size once
  std::set<detid_t> groupedIDs;

  for (auto iter = detIDs.begin(); iter != detIDs.end(); ++iter) {
    detid_t curID = *iter;
    if (groupedIDs.count(curID) == 1)
      continue; // Already been processed

    try {
      const auto &members = exptInfo.getGroupMembers(curID);
      singleIDs.push_back(members.front());
      std::copy(members.begin() + 1, members.end(),
                std::inserter(groupedIDs, groupedIDs.begin()));
    }
    catch (std::runtime_error &) {
      singleIDs.push_back(curID);
    }
  }

  g_log.debug() << "Found " << singleIDs.size() << " spectra from  "
                << detIDs.size() << " IDs\n";
  return singleIDs;
}

/**
 * Get the theta and phi angles for the given ID. If the detector was part of a
 * group,
 * as defined in the ExperimentInfo object, then the theta/phi are for the whole
 * set.
 * @param detID A reference to a single ID
 * @param exptInfo A reference to the ExperimentInfo that defines that
 * spectrum->detector mapping
 * @param theta [Output] Set to the theta angle for the detector (set)
 * @param phi [Output] Set to the phi angle for the detector (set)
 * @return A poiner to the Detector object for this spectrum as a whole
 *         (may be a single pixel or group)
 */
Geometry::IDetector_const_sptr
MDNormDirectSC::getThetaPhi(const detid_t detID, const ExperimentInfo &exptInfo,
                            double &theta, double &phi) {
  const auto spectrum = exptInfo.getDetectorByID(detID);
  theta = spectrum->getTwoTheta(m_samplePos, m_beamDir);
  phi = spectrum->getPhi();
  return spectrum;
}

/**
 * Calculate the points of intersection for the given detector with cuboid
 * surrounding the
 * detector position in HKL
 * @param theta Polar angle withd detector
 * @param phi Azimuthal angle with detector
 * @return A list of intersections in HKL space
 */
std::vector<Kernel::VMD>
MDNormDirectSC::calculateIntersections(const double theta, const double phi) {
  V3D qout(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)),
      qin(0., 0., m_ki);
  qout = m_rubw * qout;
  qin = m_rubw * qin;
  double hStart = qin.X() - qout.X() * m_kfmin,
         hEnd = qin.X() - qout.X() * m_kfmax;
  double kStart = qin.Y() - qout.Y() * m_kfmin,
         kEnd = qin.Y() - qout.Y() * m_kfmax;
  double lStart = qin.Z() - qout.Z() * m_kfmin,
         lEnd = qin.Z() - qout.Z() * m_kfmax;
  double eps = 1e-10;
  auto hNBins = m_hX.size();
  auto kNBins = m_kX.size();
  auto lNBins = m_lX.size();
  auto eNBins = m_eX.size();
  std::vector<Kernel::VMD> intersections;
  intersections.reserve(hNBins + kNBins + lNBins + eNBins +
                        8); // 8 is 3*(min,max for each Q component)+kfmin+kfmax

  // calculate intersections with planes perpendicular to h
  if (fabs(hStart - hEnd) > eps) {
    double fmom = (m_kfmax - m_kfmin) / (hEnd - hStart);
    double fk = (kEnd - kStart) / (hEnd - hStart);
    double fl = (lEnd - lStart) / (hEnd - hStart);
    if (!m_hIntegrated) {
      for (size_t i = 0; i < hNBins; i++) {
        double hi = m_hX[i];
        if ((hi >= m_hmin) && (hi <= m_hmax) &&
            ((hStart - hi) * (hEnd - hi) < 0)) {
          // if hi is between hStart and hEnd, then ki and li will be between
          // kStart, kEnd and lStart, lEnd and momi will be between m_kfmin and
          // m_kfmax
          double ki = fk * (hi - hStart) + kStart;
          double li = fl * (hi - hStart) + lStart;
          if ((ki >= m_kmin) && (ki <= m_kmax) && (li >= m_lmin) &&
              (li <= m_lmax)) {
            double momi = fmom * (hi - hStart) + m_kfmin;
            Mantid::Kernel::VMD v(hi, ki, li, momi);
            intersections.push_back(v);
          }
        }
      }
    }
    double momhMin = fmom * (m_hmin - hStart) + m_kfmin;
    if ((momhMin - m_kfmin) * (momhMin - m_kfmax) < 0) // m_kfmin>m_kfmax
    {
      // khmin and lhmin
      double khmin = fk * (m_hmin - hStart) + kStart;
      double lhmin = fl * (m_hmin - hStart) + lStart;
      if ((khmin >= m_kmin) && (khmin <= m_kmax) && (lhmin >= m_lmin) &&
          (lhmin <= m_lmax)) {
        Mantid::Kernel::VMD v(m_hmin, khmin, lhmin, momhMin);
        intersections.push_back(v);
      }
    }
    double momhMax = fmom * (m_hmax - hStart) + m_kfmin;
    if ((momhMax - m_kfmin) * (momhMax - m_kfmax) <= 0) {
      // khmax and lhmax
      double khmax = fk * (m_hmax - hStart) + kStart;
      double lhmax = fl * (m_hmax - hStart) + lStart;
      if ((khmax >= m_kmin) && (khmax <= m_kmax) && (lhmax >= m_lmin) &&
          (lhmax <= m_lmax)) {
        Mantid::Kernel::VMD v(m_hmax, khmax, lhmax, momhMax);
        intersections.push_back(v);
      }
    }
  }

  // calculate intersections with planes perpendicular to k
  if (fabs(kStart - kEnd) > eps) {
    double fmom = (m_kfmax - m_kfmin) / (kEnd - kStart);
    double fh = (hEnd - hStart) / (kEnd - kStart);
    double fl = (lEnd - lStart) / (kEnd - kStart);
    if (!m_kIntegrated) {
      for (size_t i = 0; i < kNBins; i++) {
        double ki = m_kX[i];
        if ((ki >= m_kmin) && (ki <= m_kmax) &&
            ((kStart - ki) * (kEnd - ki) < 0)) {
          // if ki is between kStart and kEnd, then hi and li will be between
          // hStart, hEnd and lStart, lEnd and momi will be between m_kfmin and
          // m_kfmax
          double hi = fh * (ki - kStart) + hStart;
          double li = fl * (ki - kStart) + lStart;
          if ((hi >= m_hmin) && (hi <= m_hmax) && (li >= m_lmin) &&
              (li <= m_lmax)) {
            double momi = fmom * (ki - kStart) + m_kfmin;
            Mantid::Kernel::VMD v(hi, ki, li, momi);
            intersections.push_back(v);
          }
        }
      }
    }
    double momkMin = fmom * (m_kmin - kStart) + m_kfmin;
    if ((momkMin - m_kfmin) * (momkMin - m_kfmax) < 0) {
      // hkmin and lkmin
      double hkmin = fh * (m_kmin - kStart) + hStart;
      double lkmin = fl * (m_kmin - kStart) + lStart;
      if ((hkmin >= m_hmin) && (hkmin <= m_hmax) && (lkmin >= m_lmin) &&
          (lkmin <= m_lmax)) {
        Mantid::Kernel::VMD v(hkmin, m_kmin, lkmin, momkMin);
        intersections.push_back(v);
      }
    }
    double momkMax = fmom * (m_kmax - kStart) + m_kfmin;
    if ((momkMax - m_kfmin) * (momkMax - m_kfmax) <= 0) {
      // hkmax and lkmax
      double hkmax = fh * (m_kmax - kStart) + hStart;
      double lkmax = fl * (m_kmax - kStart) + lStart;
      if ((hkmax >= m_hmin) && (hkmax <= m_hmax) && (lkmax >= m_lmin) &&
          (lkmax <= m_lmax)) {
        Mantid::Kernel::VMD v(hkmax, m_kmax, lkmax, momkMax);
        intersections.push_back(v);
      }
    }
  }

  // calculate intersections with planes perpendicular to l
  if (fabs(lStart - lEnd) > eps) {
    double fmom = (m_kfmax - m_kfmin) / (lEnd - lStart);
    double fh = (hEnd - hStart) / (lEnd - lStart);
    double fk = (kEnd - kStart) / (lEnd - lStart);
    if (!m_lIntegrated) {
      for (size_t i = 0; i < lNBins; i++) {
        double li = m_lX[i];
        if ((li >= m_lmin) && (li <= m_lmax) &&
            ((lStart - li) * (lEnd - li) < 0)) {
          double hi = fh * (li - lStart) + hStart;
          double ki = fk * (li - lStart) + kStart;
          if ((hi >= m_hmin) && (hi <= m_hmax) && (ki >= m_kmin) &&
              (ki <= m_kmax)) {
            double momi = fmom * (li - lStart) + m_kfmin;
            Mantid::Kernel::VMD v(hi, ki, li, momi);
            intersections.push_back(v);
          }
        }
      }
    }
    double momlMin = fmom * (m_lmin - lStart) + m_kfmin;
    if ((momlMin - m_kfmin) * (momlMin - m_kfmax) <= 0) {
      // hlmin and klmin
      double hlmin = fh * (m_lmin - lStart) + hStart;
      double klmin = fk * (m_lmin - lStart) + kStart;
      if ((hlmin >= m_hmin) && (hlmin <= m_hmax) && (klmin >= m_kmin) &&
          (klmin <= m_kmax)) {
        Mantid::Kernel::VMD v(hlmin, klmin, m_lmin, momlMin);
        intersections.push_back(v);
      }
    }
    double momlMax = fmom * (m_lmax - lStart) + m_kfmin;
    if ((momlMax - m_kfmin) * (momlMax - m_kfmax) < 0) {
      // hlmax and klmax
      double hlmax = fh * (m_lmax - lStart) + hStart;
      double klmax = fk * (m_lmax - lStart) + kStart;
      if ((hlmax >= m_hmin) && (hlmax <= m_hmax) && (klmax >= m_kmin) &&
          (klmax <= m_kmax)) {
        Mantid::Kernel::VMD v(hlmax, klmax, m_lmax, momlMax);
        intersections.push_back(v);
      }
    }
  }

  // intersections with dE
  if (!m_dEIntegrated) {
    for (size_t i = 0; i < eNBins; i++) {
      double kfi = m_eX[i];
      if ((kfi - m_kfmin) * (kfi - m_kfmax) <= 0) {
        double h = qin.X() - qout.X() * kfi;
        double k = qin.Y() - qout.Y() * kfi;
        double l = qin.Z() - qout.Z() * kfi;
        if ((h >= m_hmin) && (h <= m_hmax) && (k >= m_kmin) && (k <= m_kmax) &&
            (l >= m_lmin) && (l <= m_lmax)) {
          Mantid::Kernel::VMD v(h, k, l, kfi);
          intersections.push_back(v);
        }
      }
    }
  }

  // endpoints
  if ((hStart >= m_hmin) && (hStart <= m_hmax) && (kStart >= m_kmin) &&
      (kStart <= m_kmax) && (lStart >= m_lmin) && (lStart <= m_lmax)) {
    Mantid::Kernel::VMD v(hStart, kStart, lStart, m_kfmin);
    intersections.push_back(v);
  }
  if ((hEnd >= m_hmin) && (hEnd <= m_hmax) && (kEnd >= m_kmin) &&
      (kEnd <= m_kmax) && (lEnd >= m_lmin) && (lEnd <= m_lmax)) {
    Mantid::Kernel::VMD v(hEnd, kEnd, lEnd, m_kfmax);
    intersections.push_back(v);
  }

  // sort intersections by final momentum
  typedef std::vector<Mantid::Kernel::VMD>::iterator IterType;
  std::stable_sort<IterType, bool (*)(const Mantid::Kernel::VMD &,
                                      const Mantid::Kernel::VMD &)>(
      intersections.begin(), intersections.end(), compareMomentum);

  return intersections;
}

} // namespace MDAlgorithms
} // namespace Mantid
