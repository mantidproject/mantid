// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDNormSCD.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Strings.h"

namespace Mantid::MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormSCD)

/// Algorithm's version for identification. @see Algorithm::version
int MDNormSCD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNormSCD::category() const { return "MDAlgorithms\\Normalisation"; }

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

  auto fluxValidator = std::make_shared<CompositeValidator>();
  fluxValidator->add<WorkspaceUnitValidator>("Momentum");
  fluxValidator->add<InstrumentValidator>();
  fluxValidator->add<CommonBinsValidator>();
  auto solidAngleValidator = fluxValidator->clone();

  declareProperty(std::make_unique<WorkspaceProperty<>>("FluxWorkspace", "", Direction::Input, fluxValidator),
                  "An input workspace containing momentum dependent flux.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "", Direction::Input, solidAngleValidator),
      "An input workspace containing momentum integrated vanadium "
      "(a measure of the solid angle).");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("SkipSafetyCheck", false, Direction::Input),
                  "If set to true, the algorithm does "
                  "not check history if the workspace was modified since the"
                  "ConvertToMD algorithm was run, and assume that the elastic "
                  "mode is used.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryNormalizationWorkspace", "",
                                                                         Direction::Input, PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate normalization "
                  "from multiple MDEventWorkspaces. "
                  "If unspecified a blank MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryDataWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate data from "
                  "multiple MDEventWorkspaces. If "
                  "unspecified a blank MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
}

/**
 * Execute the algorithm.
 */
void MDNormSCD::exec() {
  cacheInputs();
  auto outputWS = binInputWS();
  m_convention = Kernel::ConfigService::Instance().getString("Q.convention");
  outputWS->setDisplayNormalization(Mantid::API::NoNormalization);
  setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
  createNormalizationWS(*outputWS);
  m_normWS->setDisplayNormalization(Mantid::API::NoNormalization);
  setProperty("OutputNormalizationWorkspace", m_normWS);
  m_diffraction = true;

  m_numExptInfos = outputWS->getNumExperimentInfo();
  m_signalArray = std::vector<std::atomic<signal_t>>(m_normWS->getNPoints());
  // loop over all experiment infos
  for (uint16_t expInfoIndex = 0; expInfoIndex < m_numExptInfos; expInfoIndex++) {
    // Check for other dimensions if we could measure anything in the original
    // data
    bool skipNormalization = false;
    const std::vector<coord_t> otherValues = getValuesFromOtherDimensions(skipNormalization, expInfoIndex);
    findIntergratedDimensions(otherValues, skipNormalization);
    cacheDimensionXValues();

    if (!skipNormalization) {
      calculateNormalization(otherValues, expInfoIndex);
    } else {
      g_log.warning("Binning limits are outside the limits of the MDWorkspace. "
                    "Not applying normalization.");
    }
  }
  std::copy(m_signalArray.cbegin(), m_signalArray.cend(), m_normWS->mutableSignalArray());
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
  const auto hdim(m_inputWS->getDimension(0)), kdim(m_inputWS->getDimension(1)), ldim(m_inputWS->getDimension(2));
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

} // namespace Mantid::MDAlgorithms
