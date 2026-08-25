// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDNormDirectSC.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Strings.h"

namespace Mantid::MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormDirectSC)

/// Algorithm's version for identification. @see Algorithm::version
int MDNormDirectSC::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNormDirectSC::category() const { return "MDAlgorithms\\Normalisation"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MDNormDirectSC::summary() const {
  return "Calculate normalization for an MDEvent workspace for single crystal "
         "direct geometry inelastic measurement.";
}

/// Algorithm's name for use in the GUI and help. @see Algorithm::name
const std::string MDNormDirectSC::name() const { return "MDNormDirectSC"; }

/**
 * Initialize the algorithm's properties.
 */
void MDNormDirectSC::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDWorkspace.");

  std::string dimChars = getDimensionChars();
  // --------------- Axis-aligned properties
  // ---------------------------------------
  for (size_t i = 0; i < dimChars.size(); i++) {
    std::string dim(" ");
    dim[0] = dimChars[i];
    std::string propName = "AlignedDim" + dim;
    declareProperty(std::make_unique<PropertyWithValue<std::string>>(propName, "", Direction::Input),
                    "Binning parameters for the " + Strings::toString(i) +
                        "th dimension.\n"
                        "Enter it as a comma-separated list of values with the format: "
                        "'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
  }

  auto solidAngleValidator = std::make_shared<CompositeValidator>();
  solidAngleValidator->add<InstrumentValidator>();
  solidAngleValidator->add<CommonBinsValidator>();

  m_progress = std::make_unique<API::Progress>(this, 0, 1, 1);

  declareProperty(std::make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "", Direction::Input,
                                                        PropertyMode::Optional, solidAngleValidator),
                  "An input workspace containing integrated vanadium (a measure of the "
                  "solid angle).");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("SkipSafetyCheck", false, Direction::Input),
                  "If set to true, the algorithm does "
                  "not check history if the workspace was modified since the"
                  "ConvertToMD algorithm was run, and assume that the direct "
                  "geometry inelastic mode is used.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryNormalizationWorkspace", "",
                                                                         Direction::Input, PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate normalization "
                  "from multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryDataWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate data from "
                  "multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void MDNormDirectSC::exec() {
  cacheInputs();
  auto outputWS = binInputWS();
  m_convention = Kernel::ConfigService::Instance().getString("Q.convention");
  outputWS->setDisplayNormalization(Mantid::API::NoNormalization);
  setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
  createNormalizationWS(*outputWS);
  m_normWS->setDisplayNormalization(Mantid::API::NoNormalization);
  setProperty("OutputNormalizationWorkspace", m_normWS);

  m_numExptInfos = outputWS->getNumExperimentInfo();
  m_signalArray = std::vector<std::atomic<signal_t>>(m_normWS->getNPoints());
  // loop over all experiment infos
  m_progress->resetNumSteps(m_numExptInfos, 0.3, 1.0);
  for (uint16_t expInfoIndex = 0; expInfoIndex < m_numExptInfos; expInfoIndex++) {
    const auto &currentExptInfo = *(m_inputWS->getExperimentInfo(expInfoIndex));
    if (!currentExptInfo.run().hasProperty("RUBW_MATRIX")) {
      throw std::runtime_error("Wokspace does not contain a log entry for the RUBW matrix."
                               "Cannot continue.");
    }
    // Check for other dimensions if we could measure anything in the original
    // data
    bool skipNormalization = false;
    const std::vector<coord_t> otherValues = getValuesFromOtherDimensions(skipNormalization, expInfoIndex);
    const auto affineTrans = findIntergratedDimensions(otherValues, skipNormalization);
    cacheDimensionXValues();

    if (!skipNormalization) {
      if (currentExptInfo.run().hasProperty("useLogTimes")) {
        calculateNormContinuous(otherValues, affineTrans, expInfoIndex);
      } else {
        calculateNormalization(otherValues, affineTrans, expInfoIndex);
      }
    } else {
      g_log.warning("Binning limits are outside the limits of the MDWorkspace. "
                    "Not applying normalization.");
    }
    m_progress->report();
  }
  std::copy(m_signalArray.cbegin(), m_signalArray.cend(), m_normWS->mutableSignalArray());

  // Set the display normalization based on the input workspace
  outputWS->setDisplayNormalization(m_inputWS->displayNormalizationHisto());
}

/**
 * Set up starting values for cached variables
 */
void MDNormDirectSC::cacheInputs() {
  m_inputWS = getProperty("InputWorkspace");
  bool skipCheck = getProperty("SkipSafetyCheck");
  if (!skipCheck && (inputEnergyMode() != "Direct")) {
    throw std::invalid_argument("Invalid energy transfer mode. Algorithm only "
                                "supports direct geometry spectrometers.");
  }
  // Min/max dimension values
  const auto hdim(m_inputWS->getDimension(0)), kdim(m_inputWS->getDimension(1)), ldim(m_inputWS->getDimension(2)),
      edim(m_inputWS->getDimension(3));
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
  if (source == nullptr || sample == nullptr) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }
  m_samplePos = sample->getPos();
  m_beamDir = normalize(m_samplePos - source->getPos());

  double originaldEmin = exptInfoZero.run().getBinBoundaries().front();
  double originaldEmax = exptInfoZero.run().getBinBoundaries().back();
  if (exptInfoZero.run().hasProperty("Ei")) {
    m_Ei = exptInfoZero.run().getPropertyValueAsType<double>("Ei");
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
  const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass * PhysicalConstants::meV * 1e-20 /
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
  const auto &lastAlgHist = hist.getAlgorithmHistory(nalgs - 1);

  std::string emode;
  if (lastAlgHist->name() == "ConvertToMD") {
    // get dEAnalysisMode
    emode = lastAlgHist->getPropertyValue("dEAnalysisMode");
  } else {
    if ((lastAlgHist->name() == "Load" || lastAlgHist->name() == "LoadMD") && nalgs > 1) {
      const auto &penultimateAlgHist = hist.getAlgorithmHistory(nalgs - 2);
      if (penultimateAlgHist->name() == "ConvertToMD") {
        return penultimateAlgHist->getPropertyValue("dEAnalysisMode");
      }
    }
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
  auto binMD = createChildAlgorithm("BinMD", 0.0, 0.3);
  binMD->setPropertyValue("AxisAligned", "1");
  for (auto prop : props) {
    const auto &propName = prop->name();
    if (propName != "SolidAngleWorkspace" && propName != "TemporaryNormalizationWorkspace" &&
        propName != "OutputNormalizationWorkspace" && propName != "SkipSafetyCheck") {
      binMD->setPropertyValue(propName, prop->value());
    }
  }
  binMD->executeAsChildAlg();
  Workspace_sptr outputWS = binMD->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
}

/**
 * Create & cached the normalization workspace
 * @param dataWS The binned workspace that will be used for the data
 */
void MDNormDirectSC::createNormalizationWS(const MDHistoWorkspace &dataWS) {
  // Copy the MDHisto workspace, and change signals and errors to 0.
  std::shared_ptr<IMDHistoWorkspace> tmp = this->getProperty("TemporaryNormalizationWorkspace");
  m_normWS = std::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
  if (!m_normWS) {
    m_normWS = dataWS.clone();
    m_normWS->setTo(0., 0., 0.);
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
std::vector<coord_t> MDNormDirectSC::getValuesFromOtherDimensions(bool &skipNormalization,
                                                                  uint16_t expInfoIndex) const {
  const auto &currentRun = m_inputWS->getExperimentInfo(expInfoIndex)->run();

  std::vector<coord_t> otherDimValues;
  for (size_t i = 4; i < m_inputWS->getNumDims(); i++) {
    const auto dimension = m_inputWS->getDimension(i);
    auto dimMin = static_cast<float>(dimension->getMinimum());
    auto dimMax = static_cast<float>(dimension->getMaximum());
    auto *dimProp = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(currentRun.getProperty(dimension->getName()));
    if (dimProp) {
      auto value = static_cast<coord_t>(dimProp->firstValue());
      otherDimValues.emplace_back(value);
      // in the original MD data no time was spent measuring between dimMin and
      // dimMax
      if (value < dimMin || value > dimMax) {
        skipNormalization = true;
      }
    }
  }
  return otherDimValues;
}

} // namespace Mantid::MDAlgorithms
