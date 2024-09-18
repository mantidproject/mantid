// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ApplyDiffCal.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::DataHandling {

using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyDiffCal)

/// Algorithms name for identification. @see Algorithm::name
const std::string ApplyDiffCal::name() const { return "ApplyDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int ApplyDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ApplyDiffCal::category() const { return "DataHandling\\Instrument;Diffraction\\DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ApplyDiffCal::summary() const {
  return "Applies a calibration to a workspace for powder diffraction";
}

/** Initialize the algorithm's properties.
 */
void ApplyDiffCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InstrumentWorkspace", "", Direction::InOut),
                  "Set the workspace whose instrument should be updated");
  const std::vector<std::string> exts{".h5", ".hd5", ".hdf", ".cal"};
  declareProperty(std::make_unique<FileProperty>("CalibrationFile", "", FileProperty::OptionalLoad, exts),
                  "Optional: The .cal file containing the position correction factors. "
                  "Either this, CalibrationWorkspace or OffsetsWorkspace needs to be "
                  "specified.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("CalibrationWorkspace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Optional: Set the Diffraction Calibration workspace");
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>("OffsetsWorkspace", "", Direction::Input,
                                                                        PropertyMode::Optional),
                  "Optional: A OffsetsWorkspace containing the calibration offsets. Either "
                  "this, CalibrationWorkspace or CalibrationFile needs to be specified.");
  declareProperty("ClearCalibration", false, "Remove any existing calibration from the workspace");
  setPropertySettings("CalibrationFile", std::make_unique<Kernel::EnabledWhenProperty>(
                                             "ClearCalibration", Kernel::ePropertyCriterion::IS_EQUAL_TO, "0"));
  setPropertySettings("CalibrationWorkspace", std::make_unique<Kernel::EnabledWhenProperty>(
                                                  "ClearCalibration", Kernel::ePropertyCriterion::IS_EQUAL_TO, "0"));
  setPropertySettings("OffsetsWorkspace", std::make_unique<Kernel::EnabledWhenProperty>(
                                              "ClearCalibration", Kernel::ePropertyCriterion::IS_EQUAL_TO, "0"));
}

std::map<std::string, std::string> ApplyDiffCal::validateInputs() {
  std::map<std::string, std::string> result;

  // Check workspace type has ExperimentInfo fields
  using API::ExperimentInfo_sptr;
  using API::Workspace_sptr;
  Workspace_sptr inputWS = getProperty("InstrumentWorkspace");
  if (!std::dynamic_pointer_cast<ExperimentInfo>(inputWS)) {
    result["InstrumentWorkspace"] = "InputWorkspace type invalid. "
                                    "Expected MatrixWorkspace, "
                                    "PeaksWorkspace.";
  }

  int numWays = 0;

  const std::string calFileName = getProperty("CalibrationFile");
  if (!calFileName.empty())
    numWays += 1;

  ITableWorkspace_const_sptr calibrationWS = getProperty("CalibrationWorkspace");
  if (bool(calibrationWS))
    numWays += 1;

  OffsetsWorkspace_const_sptr offsetsWS = getProperty("OffsetsWorkspace");
  if (bool(offsetsWS))
    numWays += 1;

  bool clearCalibration = getProperty("ClearCalibration");
  std::string message;
  if ((clearCalibration) && (numWays > 0)) {
    message = "You cannot supply a calibration input when clearing the calibration.";
  }
  if (!clearCalibration) {
    if (numWays == 0) {
      message = "You must specify only one of CalibrationFile, "
                "CalibrationWorkspace, OffsetsWorkspace.";
    }
    if (numWays > 1) {
      message = "You must specify one of CalibrationFile, "
                "CalibrationWorkspace, OffsetsWorkspace.";
    }
  }

  if (!message.empty()) {
    result["CalibrationFile"] = message;
    result["CalibrationWorkspace"] = message;
  }

  return result;
}

void ApplyDiffCal::loadCalFile(const Workspace_sptr &inputWS, const std::string &filename) {
  auto alg = createChildAlgorithm("LoadDiffCal");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("Filename", filename);
  alg->setProperty<bool>("MakeCalWorkspace", true);
  alg->setProperty<bool>("MakeGroupingWorkspace", false);
  alg->setProperty<bool>("MakeMaskWorkspace", false);
  alg->setPropertyValue("WorkspaceName", "temp");
  alg->executeAsChildAlg();

  m_calibrationWS = alg->getProperty("OutputCalWorkspace");
}

void ApplyDiffCal::getCalibrationWS(const Workspace_sptr &inputWS) {
  m_calibrationWS = getProperty("CalibrationWorkspace");
  if (m_calibrationWS)
    return; // nothing more to do

  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  if (offsetsWS) {
    auto alg = createChildAlgorithm("ConvertDiffCal");
    alg->setProperty("OffsetsWorkspace", offsetsWS);
    alg->executeAsChildAlg();
    m_calibrationWS = alg->getProperty("OutputWorkspace");
    m_calibrationWS->setTitle(offsetsWS->getTitle());
    return;
  }

  const std::string calFileName = getPropertyValue("CalibrationFile");
  if (!calFileName.empty()) {
    progress(0.0, "Reading calibration file");
    loadCalFile(inputWS, calFileName);
    return;
  }

  throw std::runtime_error("Failed to determine calibration information");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ApplyDiffCal::exec() {

  Workspace_sptr InstrumentWorkspace = getProperty("InstrumentWorkspace");
  // validateInputs guarantees this will be an ExperimentInfo object
  auto experimentInfo = std::dynamic_pointer_cast<API::ExperimentInfo>(InstrumentWorkspace);
  auto instrument = experimentInfo->getInstrument();
  auto &paramMap = experimentInfo->instrumentParameters();
  bool clearCalibration = getProperty("ClearCalibration");
  if (clearCalibration) {
    paramMap.clearParametersByName("DIFC");
    paramMap.clearParametersByName("DIFA");
    paramMap.clearParametersByName("TZERO");
  } else {
    this->getCalibrationWS(InstrumentWorkspace);

    Column_const_sptr detIdColumn = m_calibrationWS->getColumn("detid");
    Column_const_sptr difcColumn = m_calibrationWS->getColumn("difc");
    Column_const_sptr difaColumn = m_calibrationWS->getColumn("difa");
    Column_const_sptr tzeroColumn = m_calibrationWS->getColumn("tzero");

    auto detids = instrument->getDetectorIDs();
    std::sort(detids.begin(), detids.end());

    for (size_t i = 0; i < m_calibrationWS->rowCount(); ++i) {
      auto detid = static_cast<detid_t>((*detIdColumn)[i]);
      double difc = (*difcColumn)[i];
      double difa = (*difaColumn)[i];
      double tzero = (*tzeroColumn)[i];

      if (std::binary_search(detids.begin(), detids.end(), detid)) {
        // found the detector
        auto det = instrument->getDetector(detid);
        paramMap.addDouble(det->getComponentID(), "DIFC", difc);
        paramMap.addDouble(det->getComponentID(), "DIFA", difa);
        paramMap.addDouble(det->getComponentID(), "TZERO", tzero);
      } else {
        // cannot find the detector, use default zero for difc, difa, and tzero
        g_log.information() << "Cannot find det " << detid << ", skipping.\n";
      }
    }
  }
}

} // namespace Mantid::DataHandling
