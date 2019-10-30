// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CreateDetectorTable.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateDetectorTable)

void CreateDetectorTable::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the workspace to take as input.");

  declareProperty(std::make_unique<ArrayProperty<int>>("WorkspaceIndices",
                                                       Direction::Input),
                  "If left empty then all workspace indices are used.");
  setPropertySettings(
      "WorkspaceIndices",
      std::make_unique<EnabledWhenWorkspaceIsType<MatrixWorkspace>>(
          "InputWorkspace", true));

  declareProperty("IncludeData", false,
                  "Include the first value from each spectrum.");
  setPropertySettings(
      "IncludeData",
      std::make_unique<EnabledWhenWorkspaceIsType<MatrixWorkspace>>(
          "InputWorkspace", true));

  declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>(
          "DetectorTableWorkspace", "", Direction::Output,
          PropertyMode::Optional),
      "The name of the outputted detector table workspace, if left empty then "
      "the input workspace name + \"-Detectors\" is used.");
}

void CreateDetectorTable::exec() {
  Workspace_sptr inputWS = getProperty("InputWorkspace");
  bool includeData = getProperty("IncludeData");
  std::vector<int> indices = getProperty("WorkspaceIndices");

  ITableWorkspace_sptr detectorTable;
  // Standard MatrixWorkspace
  auto matrix = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS);
  if (matrix) {
    detectorTable =
        createDetectorTableWorkspace(matrix, indices, includeData, g_log);

    if (detectorTable == nullptr) {
      throw std::runtime_error("The instrument has no sample.");
    }
  } else {
    auto peaks = boost::dynamic_pointer_cast<IPeaksWorkspace>(inputWS);
    if (peaks) {
      detectorTable = peaks->createDetectorTable();
    }
  }

  if (detectorTable == nullptr) {
    throw std::runtime_error(
        "Detector table can only be created for matrix and peaks workspaces.");
  }

  if (getPropertyValue("DetectorTableWorkspace") == "") {
    setPropertyValue("DetectorTableWorkspace",
                     inputWS->getName() + "-Detectors");
  }

  setProperty("DetectorTableWorkspace", detectorTable);
}

/**
 * Create the instrument detector table workspace from a MatrixWorkspace
 * @param ws :: A pointer to a MatrixWorkspace
 * @param indices :: Limit the table to these workspace indices
 * @param includeData :: If true then first value from the each spectrum is
 * displayed
 * @param logger: The Mantid logger so errors can be written to it.
 *
 * @return A pointer to the table workspace of detector information
 */
ITableWorkspace_sptr
createDetectorTableWorkspace(const MatrixWorkspace_sptr &ws,
                             const std::vector<int> &indices,
                             const bool includeData, Logger &logger) {

  IComponent_const_sptr sample = ws->getInstrument()->getSample();
  if (!sample) {
    return nullptr;
  }

  // check if efixed value is available
  bool calcQ{true};

  // check if we have a scanning workspace
  const bool isScanning = ws->detectorInfo().isScanning();

  const auto &spectrumInfo = ws->spectrumInfo();
  if (spectrumInfo.hasDetectors(0)) {
    try {
      boost::shared_ptr<const IDetector> detector(&spectrumInfo.detector(0),
                                                  Mantid::NoDeleting());
      ws->getEFixed(detector);
    } catch (std::runtime_error &) {
      calcQ = false;
    }
  } else {
    // No detectors available
    calcQ = false;
  }

  // Prepare column names
  auto colNames = createColumns(isScanning, includeData, calcQ);

  const int ncols = static_cast<int>(colNames.size());
  const int nrows = indices.empty()
                        ? static_cast<int>(ws->getNumberHistograms())
                        : static_cast<int>(indices.size());

  auto t = WorkspaceFactory::Instance().createTable("TableWorkspace");

  // Set the column names
  for (int col = 0; col < ncols; ++col) {
    auto column = t->addColumn(colNames.at(col).first, colNames.at(col).second);
    column->setPlotType(0);
  }

  t->setRowCount(nrows);

  // Cache some frequently used values
  const auto beamAxisIndex =
      ws->getInstrument()->getReferenceFrame()->pointingAlongBeam();
  const auto sampleDist = sample->getPos()[beamAxisIndex];
  bool signedThetaParamRetrieved{false},
      showSignedTwoTheta{false}; // If true, signedVersion of the two theta
                                 // value should be displayed

  populateTable(t, ws, nrows, indices, spectrumInfo, signedThetaParamRetrieved,
                showSignedTwoTheta, beamAxisIndex, sampleDist, isScanning,
                includeData, calcQ, logger);

  return t;
}

std::vector<std::pair<std::string, std::string>>
createColumns(const bool isScanning, const bool includeData, const bool calcQ) {
  std::vector<std::pair<std::string, std::string>> colNames;
  colNames.push_back(std::make_pair("double", "Index"));
  colNames.push_back(std::make_pair("int", "Spectrum No"));
  colNames.push_back(std::make_pair("str", "Detector ID(s)"));
  if (isScanning)
    colNames.push_back(std::make_pair("str", "Time Indexes"));
  if (includeData) {
    colNames.push_back(std::make_pair("double", "Data Value"));
    colNames.push_back(std::make_pair("double", "Data Error"));
  }

  colNames.push_back(std::make_pair("double", "R"));
  colNames.push_back(std::make_pair("double", "Theta"));
  if (calcQ) {
    colNames.push_back(std::make_pair("double", "Q"));
  }
  colNames.push_back(std::make_pair("double", "Phi"));
  colNames.push_back(std::make_pair("str", "Monitor"));
  return colNames;
}

void populateTable(ITableWorkspace_sptr &t, const MatrixWorkspace_sptr &ws,
                   const int nrows, const std::vector<int> &indices,
                   const SpectrumInfo &spectrumInfo,
                   bool signedThetaParamRetrieved, bool showSignedTwoTheta,
                   const PointingAlong &beamAxisIndex, const double sampleDist,
                   const bool isScanning, const bool includeData,
                   const bool calcQ, Logger &logger) {
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*ws))
  for (int row = 0; row < nrows; ++row) {
    TableRow colValues = t->getRow(row);
    size_t wsIndex = indices.empty() ? static_cast<size_t>(row) : indices[row];
    colValues << static_cast<double>(wsIndex);
    const double dataY0{ws->y(wsIndex)[0]}, dataE0{ws->e(wsIndex)[0]};
    try {
      auto &spectrum = ws->getSpectrum(wsIndex);
      Mantid::specnum_t specNo = spectrum.getSpectrumNo();
      const auto &ids =
          dynamic_cast<const std::set<int> &>(spectrum.getDetectorIDs());
      std::string detIds = createTruncatedList(ids);

      // Geometry
      if (!spectrumInfo.hasDetectors(wsIndex))
        throw std::runtime_error("No detectors found.");
      if (!signedThetaParamRetrieved) {
        const std::vector<std::string> &parameters =
            spectrumInfo.detector(wsIndex).getStringParameter(
                "show-signed-theta", true); // recursive
        showSignedTwoTheta =
            (!parameters.empty() && find(parameters.begin(), parameters.end(),
                                         "Always") != parameters.end());
        signedThetaParamRetrieved = true;
      }

      double R{0.0}, theta{0.0}, phi{0.0};
      // theta used as a dummy variable
      // Note: phi is the angle around Z, not necessarily the beam direction.
      spectrumInfo.position(wsIndex).getSpherical(R, theta, phi);
      // R is actually L2 (same as R if sample is at (0,0,0)), except for
      // monitors which are handled below.
      R = spectrumInfo.l2(wsIndex);
      // Theta is actually 'twoTheta' for detectors (twice the scattering
      // angle), if Z is the beam direction this corresponds to theta in
      // spherical coordinates.
      // For monitors we follow historic behaviour and display theta
      const bool isMonitor = spectrumInfo.isMonitor(wsIndex);
      if (!isMonitor) {
        try {
          theta = showSignedTwoTheta ? spectrumInfo.signedTwoTheta(wsIndex)
                                     : spectrumInfo.twoTheta(wsIndex);
          theta *= 180.0 / M_PI; // To degrees
        } catch (const std::exception &ex) {
          // Log the error and leave theta as it is
          logger.error(ex.what());
        }
      } else {
        const auto dist = spectrumInfo.position(wsIndex)[beamAxisIndex];
        theta = sampleDist > dist ? 180.0 : 0.0;
      }
      const std::string isMonitorDisplay = isMonitor ? "yes" : "no";
      colValues << static_cast<int>(specNo) << detIds;
      if (isScanning) {
        std::set<int> timeIndexSet;
        for (auto def : spectrumInfo.spectrumDefinition(wsIndex)) {
          timeIndexSet.insert(int(def.second));
        }

        std::string timeIndexes = createTruncatedList(timeIndexSet);
        colValues << timeIndexes;
      }
      // Y/E
      if (includeData) {
        colValues << dataY0 << dataE0; // data
      }
      // If monitors are before the sample in the beam, DetectorInfo
      // returns a negative l2 distance.
      if (isMonitor) {
        R = std::abs(R);
      }
      colValues << R << theta;

      if (calcQ) {
        if (isMonitor) {
          // twoTheta is not defined for monitors.
          colValues << std::nan("");
        } else {
          try {

            // Get unsigned theta and efixed value
            IDetector_const_sptr det{&spectrumInfo.detector(wsIndex),
                                     Mantid::NoDeleting()};
            double efixed = ws->getEFixed(det);
            double usignTheta = spectrumInfo.twoTheta(wsIndex) * 0.5;

            double q = UnitConversion::convertToElasticQ(usignTheta, efixed);
            colValues << q;
          } catch (std::runtime_error &) {
            colValues << "No Efixed";
          }
        }
      }

      colValues << phi               // rtp
                << isMonitorDisplay; // monitor
    } catch (std::exception) {
      // spectrumNo=-1, detID=0
      colValues << -1 << "0";
      // Y/E
      if (includeData) {
        colValues << dataY0 << dataE0; // data
      }
      colValues << 0.0 << 0.0; // rt
      if (calcQ) {
        colValues << 0.0; // efixed
      }
      colValues << 0.0    // rtp
                << "n/a"; // monitor
    }                     // End catch for no spectrum
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
    truncated = std::to_string(first) + "," + std::to_string(second) + "...(" +
                std::to_string(ndets - 4) + " more)...";
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

} // namespace Algorithms
} // namespace Mantid
