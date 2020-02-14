// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ApplyCalibration.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Algorithms {

namespace {
  Kernel::Logger logger("ApplyCalibration");
}

DECLARE_ALGORITHM(ApplyCalibration)

using namespace Kernel;
using namespace API;

/// Initialisation method.
void ApplyCalibration::init() {

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "Workspace", "", Direction::InOut),
      "The name of the input workspace to apply the calibration to");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "CalibrationTable", "", Direction::Input, PropertyMode::Optional),
      "The name of the table workspace containing the new "
      "positions of detectors");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
              "PositionTable", "", Direction::Input, PropertyMode::Optional),
      "Deprecated: Use Property 'CalibrationTable'");
}

/** Executes the algorithm. Moving detectors of input workspace to positions
 *indicated in table workspace
 *
 *  @throw FileError Thrown if unable to get instrument from workspace,
 *                   table workspace is incompatible with instrument
 */
void ApplyCalibration::exec() {
  // Get pointers to the workspace, parameter map and table
  API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");
  API::ITableWorkspace_sptr CalTable = getProperty("CalibrationTable");
  API::ITableWorkspace_sptr PosTable = getProperty("PositionTable");

  // Elucidate if using property PositionTable table instead of CalibrationTable
  if(!CalTable && !PosTable){
    throw std::runtime_error("Either CalibrationTable or PositionTable must be supplied");
  }
  if(PosTable && !CalTable){
    logger.notice("Property 'PositionTable' has been deprecated. Please use 'CalibrationTable' in its place\n");
    CalTable = PosTable;
  }

  // initialize variables common to all calibrations
  std::vector<std::string> columnNames = CalTable->getColumnNames();
  size_t numDetector = CalTable->rowCount();
  ColumnVector<int> detectorID = CalTable->getVector("Detector ID");

  // Default calibration
  if (std::find(columnNames.begin(), columnNames.end(), "Detector Position") !=
      columnNames.end()) {
    auto &detectorInfo = inputWS->mutableDetectorInfo();
    ColumnVector<V3D> detPos = CalTable->getVector("Detector Position");
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (size_t i = 0; i < numDetector; ++i) {
      const auto index = detectorInfo.indexOf(detectorID[i]);
      detectorInfo.setPosition(index, detPos[i]);
    }
  }

  // Bar scan calibration: pixel Y-coordinate
  if (std::find(columnNames.begin(), columnNames.end(),
                "Detector Y Coordinate") != columnNames.end()) {
    // the detectorInfo index of a particular pixel detector is the same as the
    // componentInfo index for the same pixel detector
    auto &detectorInfo = inputWS->mutableDetectorInfo();
    ColumnVector<double> yCoordinate =
        CalTable->getVector("Detector Y Coordinate");
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (size_t i = 0; i < numDetector; ++i) {
      const auto index = detectorInfo.indexOf(detectorID[i]);
      V3D xyz = detectorInfo.position(index);
      detectorInfo.setPosition(index, V3D(xyz.X(), yCoordinate[i], xyz.Z()));
    }
  }

  // Apparent tube width calibration along X-coordinate
  if (std::find(columnNames.begin(), columnNames.end(), "Detector Width") !=
      columnNames.end()) {
    auto &detectorInfo = inputWS->detectorInfo();
    auto &componentInfo = inputWS->mutableComponentInfo();
    ColumnVector<double> widths = CalTable->getVector("Detector Width");
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (size_t i = 0; i < numDetector; ++i) {
      const auto index = detectorInfo.indexOf(detectorID[i]);
      double nominalWidth =
          componentInfo.shape(index).getBoundingBox().width().X();
      V3D oldScaleFactor = componentInfo.scaleFactor(index);
      componentInfo.setScaleFactor(index,
                                   V3D(widths[i] / nominalWidth,
                                       oldScaleFactor.Y(), oldScaleFactor.Z()));
    }
  }

  // Bar scan calibration: pixel height
  if (std::find(columnNames.begin(), columnNames.end(), "Detector Height") !=
      columnNames.end()) {
    // the detectorInfo index of a particular pixel detector is the same as the
    // componentInfo index for the same pixel detector
    auto &detectorInfo = inputWS->mutableDetectorInfo();
    auto &componentInfo = inputWS->mutableComponentInfo();
    ColumnVector<double> height = CalTable->getVector("Detector Height");
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (size_t i = 0; i < numDetector; ++i) {
      const auto index = detectorInfo.indexOf(detectorID[i]);
      // update pixel height along Y coordinate
      double nominalHeight =
          componentInfo.shape(index).getBoundingBox().width().Y();
      V3D oldScaleFactor = componentInfo.scaleFactor(index);
      componentInfo.setScaleFactor(index, V3D(oldScaleFactor.X(),
                                              height[i] / nominalHeight,
                                              oldScaleFactor.Z()));
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
