// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ApplyCalibration.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid {
namespace Algorithms {

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
          "PositionTable", "", Direction::Input),
      "The name of the table workspace containing the new "
      "positions of detectors");
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
  API::ITableWorkspace_sptr PosTable = getProperty("PositionTable");

  size_t numDetector = PosTable->rowCount();
  ColumnVector<int> detID = PosTable->getVector("Detector ID");
  ColumnVector<V3D> detPos = PosTable->getVector("Detector Position");
  // numDetector needs to be got as the number of rows in the table and the
  // detID got from the (i)th row of table.
  auto &detectorInfo = inputWS->mutableDetectorInfo();
  for (size_t i = 0; i < numDetector; ++i) {
    const auto index = detectorInfo.indexOf(detID[i]);
    detectorInfo.setPosition(index, detPos[i]);
  }
}

} // namespace Algorithms
} // namespace Mantid
