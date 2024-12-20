// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/MuonPreProcess.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include "MantidMuon/MuonAlgorithmHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace {} // namespace

namespace Mantid::Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonPreProcess)

void MuonPreProcess::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input workspace containing data from detectors that the "
      "grouping/pairing will be applied to.");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace group with all corrections applied. For single "
                  "period data, a group is returned with a single workspace.");

  declareProperty("TimeMin", EMPTY_DBL(), "Start time for the data in micro seconds.", Direction::Input);

  declareProperty("TimeMax", EMPTY_DBL(), "End time for the data in micro seconds.", Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<double>>("RebinArgs", Direction::Input),
                  "Parameters used for rebinning. If empty - rebinning is not done.");

  declareProperty("TimeOffset", EMPTY_DBL(),
                  "Shift the times of all data by a fixed amount (in micro "
                  "seconds). The value given corresponds to the bin that will "
                  "become 0.0 seconds.",
                  Direction::Input);

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("TimeZeroTable", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "TableWorkspace with time zero information, used to apply time zero "
                  "correction");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("DeadTimeTable", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "TableWorkspace with dead time information, used to apply dead time "
                  "correction.");

  std::string analysisGrp("Analysis Options");
  setPropertyGroup("TimeMin", analysisGrp);
  setPropertyGroup("TimeMax", analysisGrp);
  setPropertyGroup("RebinArgs", analysisGrp);
  setPropertyGroup("TimeOffset", analysisGrp);
  setPropertyGroup("DeadTimeTable", analysisGrp);
}

void MuonPreProcess::validateTableInputs(std::map<std::string, std::string> &errors) {
  Workspace_sptr inputWS = this->getProperty("InputWorkspace");
  if (auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    // Dead time
    TableWorkspace_sptr deadTimeTable = this->getProperty("DeadTimeTable");
    if (deadTimeTable) {
      if (deadTimeTable->rowCount() > ws->getNumberHistograms()) {
        errors["DeadTimeTable"] = "DeadTimeTable must have as many rows as "
                                  "there are spectra in InputWorkspace.";
      }
    }
    // Time zero
    TableWorkspace_sptr timeZeroTable = this->getProperty("TimeZeroTable");
    if (timeZeroTable) {
      if (timeZeroTable->rowCount() > ws->getNumberHistograms()) {
        errors["TimeZeroTable"] = "TimeZeroTable must have as many rows as "
                                  "there are spectra in InputWorkspace. Use TimeOffset to apply same "
                                  "time correcton to all data";
      }
    }
  }
}

std::map<std::string, std::string> MuonPreProcess::validateInputs() {
  std::map<std::string, std::string> errors;

  double tmin = this->getProperty("TimeMin");
  double tmax = this->getProperty("TimeMax");
  if (tmin != EMPTY_DBL() && tmax != EMPTY_DBL()) {
    if (tmin > tmax) {
      errors["TimeMin"] = "TimeMin > TimeMax";
    }
    if (tmin != EMPTY_DBL() && tmin == tmax) {
      errors["TimeMin"] = "TimeMin and TimeMax must be different";
    }
  }

  // Check for and validate dead time and time zero tables
  validateTableInputs(errors);

  Workspace_sptr inputWS = this->getProperty("InputWorkspace");
  if (auto ws = std::dynamic_pointer_cast<WorkspaceGroup>(inputWS)) {
    if (ws->getNumberOfEntries() == 0) {
      errors["InputWorkspace"] = "Input WorkspaceGroup is empty.";
    } else {
      auto nSpectra = std::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0))->getNumberHistograms();
      for (int index = 1; index < ws->getNumberOfEntries(); index++) {
        if (std::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(index))->getNumberHistograms() != nSpectra) {
          errors["InputWorkspace"] = "Numbers of spectra should be identical across all workspaces in "
                                     "the workspace group.";
        }
      }
    }
  }

  return errors;
}

void MuonPreProcess::exec() {
  this->setRethrows(true);

  Workspace_sptr inputWS = getProperty("InputWorkspace");

  // If single period, add workspace to a group
  auto allPeriodsWS = std::make_shared<WorkspaceGroup>();
  if (auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    allPeriodsWS->addWorkspace(ws);
  } else if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(inputWS)) {
    allPeriodsWS = group;
  }

  allPeriodsWS = correctWorkspaces(allPeriodsWS);

  addPreProcessSampleLogs(allPeriodsWS);

  setProperty("OutputWorkspace", allPeriodsWS);
}

/**
 * Applies offset, crops and rebin the workspaces in the group according to
 * specified params.
 * @param wsGroup :: Workspaces to correct
 * @return Corrected workspaces
 */
WorkspaceGroup_sptr MuonPreProcess::correctWorkspaces(const WorkspaceGroup_sptr &wsGroup) {
  WorkspaceGroup_sptr outWS = std::make_shared<WorkspaceGroup>();
  for (auto &&workspace : *wsGroup) {
    if (auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
      outWS->addWorkspace(correctWorkspace(ws));
    }
  }
  return outWS;
}

/**
 * Applies offset, crops and rebin the workspace according to specified
 * params.
 * @param ws :: Workspace to correct
 * @return Corrected workspace
 */
MatrixWorkspace_sptr MuonPreProcess::correctWorkspace(MatrixWorkspace_sptr ws) {
  double offset = getProperty("TimeOffset");
  double xMin = getProperty("TimeMin");
  double xMax = getProperty("TimeMax");
  std::vector<double> rebinParams = getProperty("RebinArgs");
  TableWorkspace_sptr deadTimes = getProperty("DeadTimeTable");
  TableWorkspace_sptr timeZeroTable = getProperty("TimeZeroTable");

  ws = applyDTC(ws, deadTimes);
  ws = applyTimeZeroTable(ws, timeZeroTable);
  ws = applyTimeOffset(ws, offset);
  ws = applyCropping(ws, xMin, xMax);
  ws = applyRebinning(ws, rebinParams);

  if (deadTimes == nullptr && offset == EMPTY_DBL() && (xMin == EMPTY_DBL() || xMax == EMPTY_DBL()) &&
      rebinParams.empty()) {
    ws = cloneWorkspace(ws);
  }

  return ws;
}

MatrixWorkspace_sptr MuonPreProcess::applyDTC(MatrixWorkspace_sptr ws, const TableWorkspace_sptr &dt) {
  if (dt != nullptr) {
    auto dtc = createChildAlgorithm("ApplyDeadTimeCorr");
    dtc->setProperty("InputWorkspace", ws);
    dtc->setProperty("DeadTimeTable", dt);
    dtc->execute();
    return dtc->getProperty("OutputWorkspace");
  } else {
    return ws;
  }
}

MatrixWorkspace_sptr MuonPreProcess::applyTimeOffset(MatrixWorkspace_sptr ws, const double &offset) {
  if (offset != EMPTY_DBL()) {
    auto changeOffset = createChildAlgorithm("ChangeBinOffset");
    changeOffset->setProperty("InputWorkspace", ws);
    changeOffset->setProperty("Offset", offset);
    changeOffset->execute();
    return changeOffset->getProperty("OutputWorkspace");
  } else {
    return ws;
  }
}

MatrixWorkspace_sptr MuonPreProcess::applyTimeZeroTable(const MatrixWorkspace_sptr &ws,
                                                        const TableWorkspace_sptr &timeZeroTable) {
  auto cloneWs = cloneWorkspace(ws);
  if (!timeZeroTable) {
    return cloneWs;
  }
  const auto numSpec = cloneWs->getNumberHistograms();
  for (auto specNum = 0u; specNum < numSpec; ++specNum) {
    auto &xData = cloneWs->mutableX(specNum);
    for (auto &xValue : xData) {
      API::TableRow row = timeZeroTable->getRow(specNum);
      xValue -= row.Double(0);
    }
  }
  return cloneWs;
}

MatrixWorkspace_sptr MuonPreProcess::applyCropping(MatrixWorkspace_sptr ws, const double &xMin, const double &xMax) {
  if (xMin == EMPTY_DBL() && xMax == EMPTY_DBL())
    return ws;

  if (getPropertyValue("TimeZeroTable").empty())
    return cropWithSingleValues(ws, xMin, xMax);
  else
    return cropWithVectors(ws, xMin, xMax);
}

MatrixWorkspace_sptr MuonPreProcess::cropWithSingleValues(const MatrixWorkspace_sptr &ws, const double xMin,
                                                          const double xMax) {
  auto crop = createChildAlgorithm("CropWorkspace");
  crop->setProperty("InputWorkspace", ws);
  if (xMin != EMPTY_DBL())
    crop->setProperty("Xmin", xMin);
  if (xMax != EMPTY_DBL())
    crop->setProperty("Xmax", xMax);
  crop->execute();
  return crop->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr MuonPreProcess::cropWithVectors(const MatrixWorkspace_sptr &ws, const double xMin,
                                                     const double xMax) {
  std::vector<double> xMinVec;
  std::vector<double> xMaxVec;

  if (xMin != EMPTY_DBL())
    xMinVec.insert(xMinVec.end(), ws->getNumberHistograms(), xMin);
  else {
    for (auto specNum = 0u; specNum < ws->getNumberHistograms(); ++specNum) {
      auto &xData = ws->mutableX(specNum);
      xMinVec.emplace_back(xData[0]); // Append first value for each spectrum
    }
  }

  if (xMax != EMPTY_DBL())
    xMaxVec.insert(xMaxVec.end(), ws->getNumberHistograms(), xMax);
  else {
    for (auto specNum = 0u; specNum < ws->getNumberHistograms(); ++specNum) {
      auto &xData = ws->mutableX(specNum);
      xMaxVec.emplace_back(xData[xData.size() - 1]); // Append last value for each spectrum
    }
  }

  auto cropRagged = createChildAlgorithm("CropWorkspaceRagged");
  cropRagged->setProperty("InputWorkspace", ws);
  cropRagged->setProperty("XMin", xMinVec);
  cropRagged->setProperty("XMax", xMaxVec);
  cropRagged->execute();
  return cropRagged->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr MuonPreProcess::applyRebinning(MatrixWorkspace_sptr ws, const std::vector<double> &rebinArgs) {
  if (!rebinArgs.empty()) {
    auto rebin = createChildAlgorithm("Rebin");
    rebin->setProperty("InputWorkspace", ws);
    rebin->setProperty("Params", rebinArgs);
    rebin->setProperty("FullBinsOnly", false);
    rebin->execute();
    return rebin->getProperty("OutputWorkspace");
  } else {
    return ws;
  }
}

MatrixWorkspace_sptr MuonPreProcess::cloneWorkspace(const MatrixWorkspace_sptr &ws) {
  auto cloneWorkspace = createChildAlgorithm("CloneWorkspace");
  cloneWorkspace->setProperty("InputWorkspace", ws);
  cloneWorkspace->execute();
  Workspace_sptr wsClone = cloneWorkspace->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MatrixWorkspace>(wsClone);
}

void MuonPreProcess::addPreProcessSampleLogs(const WorkspaceGroup_sptr &group) {
  const std::string numPeriods = std::to_string(group->getNumberOfEntries());

  for (auto &&workspace : *group) {

    MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_periods",
                                      numPeriods);

    std::vector<double> rebinArgs = getProperty("RebinArgs");
    if (rebinArgs.empty()) {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_rebin_args",
                                        "");
    } else {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_rebin_args",
                                        getPropertyValue("RebinArgs"));
    }

    double xmin = getProperty("TimeMin");
    if (xmin == EMPTY_DBL()) {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_crop_x_min",
                                        "");
    } else {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_crop_x_min",
                                        getPropertyValue("TimeMin"));
    }

    double xmax = getProperty("TimeMax");
    if (xmax == EMPTY_DBL()) {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_crop_x_max",
                                        "");
    } else {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_crop_x_max",
                                        getPropertyValue("TimeMax"));
    }

    double offset = getProperty("TimeOffset");
    if (offset == EMPTY_DBL()) {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_time_offset",
                                        "");
    } else {
      MuonAlgorithmHelper::addSampleLog(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), "analysis_time_offset",
                                        getPropertyValue("TimeOffset"));
    }
  }
}

// Allow WorkspaceGroup property to function correctly.
bool MuonPreProcess::checkGroups() { return false; }

} // namespace Mantid::Muon
