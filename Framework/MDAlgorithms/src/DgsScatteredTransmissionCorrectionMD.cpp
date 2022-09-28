// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// local
#include "MantidMDAlgorithms/DgsScatteredTransmissionCorrectionMD.h"

// 3rd party
#include "MantidAPI/ExperimentInfo.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"

// standard
#include <limits>
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid::MDAlgorithms {

constexpr double EMPTY_FLT() noexcept { return std::numeric_limits<float>::max() / 2; }

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsScatteredTransmissionCorrectionMD)

//---------------------------------------------------------------------------------------------------------

void DgsScatteredTransmissionCorrectionMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input MDEventWorkspace. Either QSample (or QLab) frame plus DeltaE, or just Qmod plus DeltaE");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  mustBePositive->setLowerExclusive(true);
  declareProperty(
      std::make_unique<PropertyWithValue<double>>("ExponentFactor", EMPTY_DBL(), mustBePositive, Direction::Input),
      "Depletion rate exponent");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output MDEventWorkspace with the correction applied");
}

//---------------------------------------------------------------------------------------------------------

std::map<std::string, std::string> DgsScatteredTransmissionCorrectionMD::validateInputs() {
  std::map<std::string, std::string> output;
  // validate input workspace
  std::string workspace_error = checkInputWorkspace();
  if (workspace_error != "")
    output["InputWorkspace"] = workspace_error;
  // ExponentFactor should be positive
  double c = getProperty("ExponentFactor");
  if (c <= 0.0)
    output["ExponentFactor"] = "ExponentFactor should be positive";
  return output;
}

//---------------------------------------------------------------------------------------------------------

std::string DgsScatteredTransmissionCorrectionMD::checkInputWorkspace() {

  // Verify the dimension of the input workspace
  std::string dimensionError = checkInputMDDimensions();
  if (dimensionError != "")
    return dimensionError;

  // Verify input workspace has an Efixed metadata
  IMDEventWorkspace_sptr inputws = getProperty("InputWorkspace");
  uint16_t nRuns(inputws->getNumExperimentInfo());
  mEfixedValues.resize(nRuns);
  for (uint16_t i = 0; i < inputws->getNumExperimentInfo(); i++) {
    const auto expInfo = inputws->getExperimentInfo(i);
    try {
      mEfixedValues[i] = static_cast<float>(expInfo->getEFixed());
    } catch (std::runtime_error &e) {
      return e.what();
    }
  }
  return ""; // all is well
}

//---------------------------------------------------------------------------------------------------------

std::string DgsScatteredTransmissionCorrectionMD::checkInputMDDimensions() {
  std::string errormsg("");
  IMDEventWorkspace_sptr inputws = getProperty("InputWorkspace");
  size_t numdims = inputws->getNumDims();

  // Get and check the dimensions: Q3D or Q1D
  const SpecialCoordinateSystem coordsys = inputws->getSpecialCoordinateSystem();
  size_t qdim(0);
  std::string qdimstr("Not Q3D or |Q|");
  if (coordsys == SpecialCoordinateSystem::QLab || coordsys == SpecialCoordinateSystem::QSample) {
    // q3d
    qdim = 3;
    qdimstr = "Q3D";
  } else {
    // search Q1D: at any place
    for (size_t i = 0; i < numdims; ++i) {
      if (inputws->getDimension(i)->getName() == "|Q|") {
        qdim = 1;
        qdimstr = "|Q|";
        break;
      }
    }
  }

  // Check dimension index for dimension "DeltaE"
  bool qModCase = (qdim == 1) && (inputws->getDimension(1)->getName() == "DeltaE");
  bool qVecCase = (qdim == 3) && (inputws->getDimension(3)->getName() == "DeltaE");
  if (!qModCase && !qVecCase) { // both allowed q-cases failed
    g_log.error() << "Coordinate system = " << coordsys << " does not meet requirement: \n";
    for (size_t i = 0; i < numdims; ++i) {
      g_log.error() << i << "-th dim: " << inputws->getDimension(i)->getName() << "\n";
    }
    errormsg += "Q Dimension (" + qdimstr +
                ") is neither Q3D nor |Q|.  Or DeltaE is found in an improper place (2nd or 4th dimension).";
  }

  return errormsg;
}

//---------------------------------------------------------------------------------------------------------

template <typename MDE, size_t nd>
void DgsScatteredTransmissionCorrectionMD::correctForTransmission(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  double cDouble = getProperty("ExponentFactor");
  float c = static_cast<float>(cDouble);
  size_t deltaEIndex = ws->getNumDims() - 1;

  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  // Add the boxes in parallel. They should be spread out enough on each core to avoid stepping on each other.

  PRAGMA_OMP( parallel for if (!ws->isFileBacked()))
  for (int i = 0; i < numBoxes; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      std::vector<MDE> &events = box->getEvents();
      for (auto it = events.begin(); it != events.end(); ++it) {
        float Ei = mEfixedValues[it->getExpInfoIndex()];
        float Ef = Ei - it->getCenter(deltaEIndex); // Ei - deltaE
        float correction(exp(c * Ef));

        it->setSignal(it->getSignal() * correction);
        it->setErrorSquared(it->getErrorSquared() * correction * correction); // no error from `correction`
      }
    }
    box->releaseEvents();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

//---------------------------------------------------------------------------------------------------------

void DgsScatteredTransmissionCorrectionMD::exec() {
  // Get input workspace
  IMDEventWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Initialize the output workspace
  std::string outputWsName = getPropertyValue("OutputWorkspace");
  IMDEventWorkspace_sptr outputWs(nullptr);
  if (inputWs->getName() == outputWsName)
    outputWs = inputWs;
  else
    outputWs = inputWs->clone();

  // Apply detailed balance to MDEvents
  CALL_MDEVENT_FUNCTION(correctForTransmission, outputWs);

  // refresh cache for MDBoxes: set correct Box signal
  outputWs->refreshCache();

  // Clear masking (box flags) from the output workspace
  outputWs->clearMDMasking();

  // Set output
  setProperty("OutputWorkspace", outputWs);
}

} // namespace Mantid::MDAlgorithms
