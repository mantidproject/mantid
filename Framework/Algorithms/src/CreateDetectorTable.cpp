// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateDetectorTable.h"
#include "MantidAPI/EnabledWhenWorkspaceIsType.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/V3D.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid::Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateDetectorTable)

void CreateDetectorTable::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the workspace to take as input.");

  declareProperty(std::make_unique<ArrayProperty<int>>("WorkspaceIndices", Direction::Input),
                  "If left empty then all workspace indices are used.");
  setPropertySettings("WorkspaceIndices",
                      std::make_unique<EnabledWhenWorkspaceIsType<MatrixWorkspace>>("InputWorkspace", true));

  declareProperty("IncludeData", false, "Include the first value from each spectrum.");
  setPropertySettings("IncludeData",
                      std::make_unique<EnabledWhenWorkspaceIsType<MatrixWorkspace>>("InputWorkspace", true));

  declareProperty<bool>("IncludeDetectorPosition", false,
                        "Include the absolute position of the detector group for each spectrum.", Direction::Input);
  setPropertySettings("IncludeDetectorPosition",
                      std::make_unique<EnabledWhenWorkspaceIsType<MatrixWorkspace>>("InputWorkspace", true));

  declareProperty<bool>("OneDetectorIdPerRow", false, "Order rows in table by de.", Direction::Input);
  setPropertySettings("OneDetectorIdPerRow",
                      std::make_unique<EnabledWhenWorkspaceIsType<MatrixWorkspace>>("InputWorkspace", true));

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("DetectorTableWorkspace", "", Direction::Output,
                                                                      PropertyMode::Optional),
                  "The name of the outputted detector table workspace, if left empty then "
                  "the input workspace name + \"-Detectors\" is used.");
}

void CreateDetectorTable::exec() {
  API::Workspace_sptr inputWs = getProperty("InputWorkspace");
  includeData = getProperty("IncludeData");
  workspaceIndices = getProperty("WorkspaceIndices");
  includeDetectorPosition = getProperty("IncludeDetectorPosition");
  OneDetectorIdPerRow = getProperty("OneDetectorIdPerRow");

  if (auto peaks = std::dynamic_pointer_cast<IPeaksWorkspace>(inputWs)) {
    table = peaks->createDetectorTable();
    setTableToOutput();
    return;
  }

  if ((ws = std::dynamic_pointer_cast<MatrixWorkspace>(inputWs))) {
    if (ws->getInstrument()->getSample() == nullptr) {
      throw std::runtime_error("Matrix workspace has no instrument information");
    }
    setup();
    createColumns();
    if (OneDetectorIdPerRow) {
      // Used in Instrument View
      populateTableByDetID();
    } else {
      // General case
      populateTable();
    }
    setTableToOutput();
    return;
  }

  throw std::runtime_error("Detector table can only be created for matrix and peaks workspaces.");
}

void CreateDetectorTable::setTableToOutput() {
  if (table == nullptr) {
    throw std::runtime_error("Unknown error while creating detector table workspace");
  }
  API::Workspace_sptr inputWs = getProperty("InputWorkspace");
  if (getPropertyValue("DetectorTableWorkspace") == "") {
    setPropertyValue("DetectorTableWorkspace", inputWs->getName() + "-Detectors");
  }
  setProperty("DetectorTableWorkspace", table);
}

/*
 * Validate the input parameters
 * @returns map with keys corresponding to properties with errors and values
 * containing the error messages.
 */
std::map<std::string, std::string> CreateDetectorTable::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;

  Workspace_sptr inputWS = getProperty("InputWorkspace");
  const auto matrix = std::dynamic_pointer_cast<MatrixWorkspace>(inputWS);

  if (matrix) {
    const int numSpectra = static_cast<int>(matrix->getNumberHistograms());
    const std::vector<int> indices = getProperty("WorkspaceIndices");

    if (std::any_of(indices.cbegin(), indices.cend(),
                    [numSpectra](const auto index) { return (index >= numSpectra) || (index < 0); })) {
      validationOutput["WorkspaceIndices"] = "One or more indices out of range of available spectra.";
    }
  }

  return validationOutput;
}

void CreateDetectorTable::setup() {

  isScanning = ws->detectorInfo().isScanning();

  spectrumInfo = &ws->spectrumInfo();
  detectorInfo = &ws->detectorInfo();

  // check if efixed value is available
  calcQ = true;
  if (spectrumInfo->hasDetectors(0)) {
    try {
      std::shared_ptr<const IDetector> detector(&spectrumInfo->detector(0), Mantid::NoDeleting());
      ws->getEFixed(detector);
    } catch (std::invalid_argument &) {
      calcQ = false;
    } catch (std::runtime_error &) {
      calcQ = false;
    }
  } else {
    // No detectors available
    calcQ = false;
  }

  hasDiffConstants = (ws->getEMode() == DeltaEMode::Elastic);

  beamAxisIndex = ws->getInstrument()->getReferenceFrame()->pointingAlongBeam();
  sampleDist = ws->getInstrument()->getSample()->getPos()[beamAxisIndex];
  signedThetaParamRetrieved = false;
  showSignedTwoTheta = false; // If true, signedVersion of the two theta

  if (workspaceIndices.empty()) {
    workspaceIndices = std::vector<int>(ws->getNumberHistograms());
    std::iota(workspaceIndices.begin(), workspaceIndices.end(), 0);
  }

  if (OneDetectorIdPerRow) {
    nrows = detectorInfo->detectorIDs().size();
  } else {
    nrows = workspaceIndices.size();
  }
  table = WorkspaceFactory::Instance().createTable("TableWorkspace");
  table->setRowCount(nrows);
}

void CreateDetectorTable::createColumns() {
  std::vector<std::pair<std::string, std::string>> colNames;
  colNames.emplace_back("int", "Index");
  colNames.emplace_back("int", "Spectrum No");
  if (OneDetectorIdPerRow) {
    colNames.emplace_back("int", "Detector ID(s)");
  } else {
    colNames.emplace_back("str", "Detector ID(s)");
  }
  if (isScanning)
    colNames.emplace_back("str", "Time Indexes");
  if (includeData) {
    colNames.emplace_back("double", "Data Value");
    colNames.emplace_back("double", "Data Error");
  }

  colNames.emplace_back("double", "R");
  colNames.emplace_back("double", "Theta");
  if (calcQ) {
    colNames.emplace_back("double", "Q elastic");
  }
  colNames.emplace_back("double", "Phi");
  colNames.emplace_back("str", "Monitor");
  if (hasDiffConstants) {
    colNames.emplace_back("double", "DIFA");
    colNames.emplace_back("double", "DIFC");
    colNames.emplace_back("double", "DIFC - Uncalibrated");
    colNames.emplace_back("double", "TZERO");
  }
  if (includeDetectorPosition) {
    colNames.emplace_back("V3D", "Position");
  }

  // Set the column names
  for (size_t col = 0; col < colNames.size(); ++col) {
    auto column = table->addColumn(colNames.at(col).first, colNames.at(col).second);
    column->setPlotType(0);
  }
  return;
}

void CreateDetectorTable::get_spherical_coordinates(size_t wsIndex, double &R, double &theta, double &phi) {
  // theta used as a dummy variable
  // Note: phi is the angle around Z, not necessarily the beam direction.
  spectrumInfo->position(wsIndex).getSpherical(R, theta, phi);
  // R is actually L2 (same as R if sample is at (0,0,0)), except for
  // monitors which are handled below.
  R = spectrumInfo->l2(wsIndex);
  // Theta is actually 'twoTheta' for detectors (twice the scattering
  // angle), if Z is the beam direction this corresponds to theta in
  // spherical coordinates.
  // For monitors we follow historic behaviour and display theta
  if (!spectrumInfo->isMonitor(wsIndex)) {
    try {
      theta = showSignedTwoTheta ? spectrumInfo->signedTwoTheta(wsIndex) : spectrumInfo->twoTheta(wsIndex);
      theta *= 180.0 / M_PI; // To degrees
    } catch (const std::exception &ex) {
      // Log the error and leave theta as it is
      g_log.error(ex.what());
    }
  } else {
    const auto dist = spectrumInfo->position(wsIndex)[beamAxisIndex];
    theta = sampleDist > dist ? 180.0 : 0.0;

    // If monitors are before the sample in the beam, DetectorInfo
    // returns a negative l2 distance.
    R = std::abs(R);
  }
}

const std::string CreateDetectorTable::get_time_indexes(size_t wsIndex) {
  if (!isScanning) {
    return "0";
  }
  std::set<int> timeIndexSet;
  for (const auto &def : spectrumInfo->spectrumDefinition(wsIndex)) {
    timeIndexSet.insert(int(def.second));
  }
  const std::string timeIndexes = createTruncatedList(timeIndexSet);
  return timeIndexes;
}

double CreateDetectorTable::get_q(size_t wsIndex) {
  if (!calcQ) {
    return std::nan("");
  }
  if (spectrumInfo->isMonitor(wsIndex)) {
    // twoTheta is not defined for monitors.
    return std::nan("");
  }
  try {
    // Get unsigned theta and efixed value
    IDetector_const_sptr det{&spectrumInfo->detector(wsIndex), Mantid::NoDeleting()};
    double efixed = ws->getEFixed(det);
    double usignTheta = spectrumInfo->twoTheta(wsIndex) * 0.5;
    double q = UnitConversion::convertToElasticQ(usignTheta, efixed);
    return q;
  } catch (std::runtime_error &) {
    // No Efixed
    return std::nan("");
  }
}

void CreateDetectorTable::get_diff_consts(size_t wsIndex, double &difa, double &difc, double &difcUnc, double &tzero) {
  if (!hasDiffConstants) {
    difa = difc = difcUnc = tzero = 0;
    return;
  }
  if (spectrumInfo->isMonitor(wsIndex)) {
    difa = difc = difcUnc = tzero = 0;
    return;
  }
  auto diffConsts = spectrumInfo->diffractometerConstants(wsIndex);
  // map will create an entry with zero value if not present already
  difa = diffConsts[UnitParams::difa];
  difc = diffConsts[UnitParams::difc];
  difcUnc = spectrumInfo->difcUncalibrated(wsIndex);
  tzero = diffConsts[UnitParams::tzero];
}

void CreateDetectorTable::writeRowToTable(const size_t row, const size_t wsIndex, const int specNo,
                                          const std::set<int> &detIds, const std::string &timeIndexes,
                                          const double dataY0, const double dataE0, const double R, const double theta,
                                          const double q, const double phi, const std::string &isMonitor,
                                          const double difa, const double difc, const double difcUnc,
                                          const double tzero, const Kernel::V3D detPosition) {
  TableRow colValues = table->getRow(row);
  colValues << static_cast<int>(wsIndex);
  colValues << specNo;
  if (OneDetectorIdPerRow) {
    // Populate detector column with first det id in set
    colValues << static_cast<int>(*detIds.begin());
  } else {
    // Populate detector column with a truncated string of all det ids
    colValues << createTruncatedList(detIds);
  }
  if (isScanning) {
    colValues << timeIndexes;
  }
  if (includeData) {
    colValues << dataY0 << dataE0; // data
  }
  colValues << R << theta;
  if (calcQ) {
    colValues << q;
  }
  colValues << phi;       // rtp
  colValues << isMonitor; // monitor
  if (hasDiffConstants) {
    colValues << difa << difc << difcUnc << tzero;
  }
  if (includeDetectorPosition) {
    colValues << detPosition;
  }
}

void CreateDetectorTable::calculateWsIdxData(const size_t wsIndex, int &specNo, std::set<int> &detIds,
                                             std::string &timeIndexes, double &dataY0, double &dataE0, double &R,
                                             double &theta, double &q, double &phi, std::string &isMonitor,
                                             double &difa, double &difc, double &difcUnc, double &tzero,
                                             Kernel::V3D &detPosition) {

  // Geometry
  if (!spectrumInfo->hasDetectors(wsIndex))
    throw std::runtime_error("No detectors found.");
  if (!signedThetaParamRetrieved) {
    const std::vector<std::string> &parameters =
        spectrumInfo->detector(wsIndex).getStringParameter("show-signed-theta", true); // recursive
    showSignedTwoTheta =
        (!parameters.empty() && find(parameters.begin(), parameters.end(), "Always") != parameters.end());
    signedThetaParamRetrieved = true;
  }

  // Spec No
  auto &spectrum = ws->getSpectrum(wsIndex);
  specNo = spectrum.getSpectrumNo();
  detIds = dynamic_cast<const std::set<int> &>(spectrum.getDetectorIDs());
  // Time indexes
  timeIndexes = get_time_indexes(wsIndex);
  // data Y/E
  dataY0 = ws->y(wsIndex)[0];
  dataE0 = ws->e(wsIndex)[0];
  // R, Theta, Phi
  get_spherical_coordinates(wsIndex, R, theta, phi);
  // Q
  q = get_q(wsIndex);
  // Is monitor
  isMonitor = spectrumInfo->isMonitor(wsIndex) ? "yes" : "no";
  // Diff consts
  get_diff_consts(wsIndex, difa, difc, difcUnc, tzero);
  // Detector position
  detPosition = spectrumInfo->position(wsIndex);
}

void CreateDetectorTable::populateTable() {

  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*ws))
  for (size_t row = 0; row < workspaceIndices.size(); row++) {

    size_t wsIndex = workspaceIndices[row];

    try {
      int specNo = 0;
      std::set<int> detIds;
      std::string timeIndexes, isMonitor;
      double dataY0, dataE0, R, theta, q, phi, difa, difc, difcUnc, tzero;
      dataY0 = dataE0 = R = theta = q = phi = difa = difc = difcUnc = tzero = 0;
      Kernel::V3D detPosition(0, 0, 0);

      calculateWsIdxData(wsIndex, specNo, detIds, timeIndexes, dataY0, dataE0, R, theta, q, phi, isMonitor, difa, difc,
                         difcUnc, tzero, detPosition);

      writeRowToTable(row, wsIndex, specNo, detIds, timeIndexes, dataY0, dataE0, R, theta, q, phi, isMonitor, difa,
                      difc, difcUnc, tzero, detPosition);

    } catch (const std::exception &) {
      double dataY0 = ws->y(wsIndex)[0];
      double dataE0 = ws->e(wsIndex)[0];

      writeRowToTable(row, wsIndex, -1, std::set<int>{0}, "0", dataY0, dataE0, 0, 0, 0, 0, "n/a", 0, 0, 0, 0,
                      V3D(0, 0, 0));
    } // End catch for no spectrum
  }
}

void CreateDetectorTable::populateTableByDetID() {
  auto detIdToWorkspaceIndexMap = ws->getDetectorIDToWorkspaceIndexMap();
  const auto &workspaceDetectorIds = detectorInfo->detectorIDs();

  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*ws))
  for (size_t row = 0; row < workspaceDetectorIds.size(); row++) {

    int detId = workspaceDetectorIds[row];
    size_t wsIndex = detIdToWorkspaceIndexMap[detId];

    try {
      std::set<int> detIds;
      int specNo = 0;
      std::string timeIndexes, isMonitor;
      double dataY0, dataE0, R, theta, q, phi, difa, difc, difcUnc, tzero;
      dataY0 = dataE0 = R = theta = q = phi = difa = difc = difcUnc = tzero = 0;
      Kernel::V3D detPosition(0, 0, 0);

      calculateWsIdxData(wsIndex, specNo, detIds, timeIndexes, dataY0, dataE0, R, theta, q, phi, isMonitor, difa, difc,
                         difcUnc, tzero, detPosition);

      writeRowToTable(row, wsIndex, specNo, std::set<int>{detId}, timeIndexes, dataY0, dataE0, R, theta, q, phi,
                      isMonitor, difa, difc, difcUnc, tzero, detPosition);

    } catch (const std::exception &) {
      double dataY0 = ws->y(wsIndex)[0];
      double dataE0 = ws->e(wsIndex)[0];

      writeRowToTable(row, wsIndex, -1, std::set<int>{0}, "0", dataY0, dataE0, 0, 0, 0, 0, "n/a", 0, 0, 0, 0,
                      V3D(0, 0, 0));
    } // End catch for no spectrum
  }
}

/**
 * Converts a set of ints to a string with each element separated by a
 * comma. If there are more than 10 elements, the format "a,b...(n more)...y,z"
 * is used.
 *
 * @param elements :: The set of elements to be converted
 *
 * @return The truncated list as a string
 */
std::string createTruncatedList(const std::set<int> &elements) {
  std::string truncated;
  size_t ndets = elements.size();
  auto iter = elements.begin();
  auto itEnd = elements.end();
  if (ndets > 10) {
    const Mantid::detid_t first{*iter++}, second{*iter++};
    truncated = std::to_string(first) + "," + std::to_string(second) + "...(" + std::to_string(ndets - 4) + " more)...";
    auto revIter = elements.rbegin();
    const Mantid::detid_t last{*revIter++}, lastm1{*revIter++};
    truncated += std::to_string(lastm1) + "," + std::to_string(last);
  } else {
    for (; iter != itEnd; ++iter) {
      truncated += std::to_string(*iter) + ",";
    }

    if (!truncated.empty()) {
      truncated.pop_back(); // Drop last comma
    }
  }

  return truncated;
}

} // namespace Mantid::Algorithms
