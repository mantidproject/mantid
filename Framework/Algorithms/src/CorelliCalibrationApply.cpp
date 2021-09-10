// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CorelliCalibrationApply.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;
namespace {
Logger logger("CorelliCalibrationApply");
}

DECLARE_ALGORITHM(CorelliCalibrationApply)

/**
 * @brief Initialization
 *
 */
void CorelliCalibrationApply::init() {

  // InputWorkspace
  // [Input, Required, MatrixWorkspace or EventsWorkspace]
  // workspace to which the calibration should be applied
  auto wsValidator = std::make_shared<InstrumentValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "", Direction::InOut,
                                                                       PropertyMode::Mandatory, wsValidator),
                  "CORELLI workspace to calibrate");

  // CalibrationTable
  // [Input, Mandatory, TableWorkspace]
  // workspace resulting from uploading
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("CalibrationTable", "", Direction::Input,
                                                                      PropertyMode::Mandatory),
                  "TableWorkspace containing calibration table");
}

/**
 * @brief Validate algorithm inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> CorelliCalibrationApply::validateInputs() {
  std::map<std::string, std::string> issues;
  ws = getProperty("Workspace");

  // 1_check: input workspace is from CORELLI
  if (ws->getInstrument()->getName() != "CORELLI") {
    issues["Workspace"] = "CORELLI only algorithm, aborting";
  }

  // 2_check: headers of calibration table
  calTable = getProperty("CalibrationTable");
  auto refCalTableHeader = CorelliCalibration::calibrationTableColumnNames;
  std::vector<std::string> colnames = calTable->getColumnNames();
  if (colnames.size() != refCalTableHeader.size()) {
    issues["CalibrationTable"] = "Headers of input calibration table does not match required";
  }
  for (size_t i = 0; i < colnames.size(); i++) {
    if (colnames[i] != refCalTableHeader[i]) {
      issues["CalibrationTable"] = "Header mismatch at " + std::to_string(i);
      break;
    }
  }

  return issues;
}

/**
 * @brief Executes the algorithm.
 *
 */
void CorelliCalibrationApply::exec() {
  g_log.notice() << "Start applying CORELLI calibration\n";

  // Parse input arguments
  ws = getProperty("Workspace");
  auto wsName = ws->getName();

  calTable = getProperty("CalibrationTable");
  const auto componentNames = calTable->getColumn(0);
  const auto x_poss = calTable->getColumn(1);
  const auto y_poss = calTable->getColumn(2);
  const auto z_poss = calTable->getColumn(3);
  const auto rotxs = calTable->getColumn(4);
  const auto rotys = calTable->getColumn(5);
  const auto rotzs = calTable->getColumn(6);
  const auto rotangs = calTable->getColumn(7); // unit: degrees

  /**
  Translate each component in the instrument
  [moderator, sample-position, bank1,.. bank92]
  NOTE:
  1. moderator: this is often called "source" in other settings, but CORELLI
                instrument uses the term moderator for it.
  2. sample-position: this is often referred to as "sample" in other
                      instrument definition, but the CORELLI instrument uses
                      a more verbose version here.
  3. the tranlation vector calcuated in upstream is the absolute coordinates
     of each component (in lab frame), therefore we need to toggle the option
     RelativePosition off here.
  */
  g_log.notice() << "Translating each component using given Calibration table";
  auto moveAlg = createChildAlgorithm("MoveInstrumentComponent");
  moveAlg->initialize();
  moveAlg->setProperty("Workspace", wsName);
  for (size_t row_num = 0; row_num < calTable->rowCount(); row_num++) {
    moveAlg->setProperty("ComponentName", componentNames->cell<std::string>(row_num));
    moveAlg->setProperty("X", x_poss->cell<double>(row_num));
    moveAlg->setProperty("Y", y_poss->cell<double>(row_num));
    moveAlg->setProperty("Z", z_poss->cell<double>(row_num));
    // [IMPORTANT]
    // The position data from calibration table are ABSOLUTE values (lab frame)
    moveAlg->setProperty("RelativePosition", false);
    moveAlg->execute();
  }

  /**
  Rotate each component in the instrument
  IMPORTANT NOTE:
  0. The upstream calculation is a minimization routine that searches the
     detector position by minimizaing DIFC, therefore the reported values
     are ABSOLUTE values.
  1. In Mantid, the same component, let's take bank42 as an example, can have
     different reference frameworks depending how it is called.
     - bank42:  the reference framework is the lab, where the origin (0,0,0)
                is the sample location;
     - bank42/sixteenpack: the reference framework is the sixteenpack, where
                           the origin is the geometry center of the
                           sixteenpack.
     Given that the rotation angle is calucated w.r.t. sixteenpack, we need
     to make sure that the provided table has the correct component names
     defined in it.
  2. The Algorithm, RotateInstrumentComponent, actually does the rotation in
  three steps:
     - translate the component to the origin of the defined reference
       framework (lab for bank42, and sixteenpack for bank42/sixteenpack)
     - perform the rotation using given rotation axis and rotation angle
       (in degrees)
     - translate the component back to its starting position
     More information can be found in the documentation of the algorithm.
  */
  g_log.notice() << "Rotating each component using given Calibration table";
  auto rotateAlg = createChildAlgorithm("RotateInstrumentComponent");
  rotateAlg->initialize();
  rotateAlg->setProperty("Workspace", wsName);
  for (size_t row_num = 0; row_num < calTable->rowCount(); row_num++) {
    if (abs(rotangs->cell<double>(row_num)) < 1e-8) {
      continue;
    }
    rotateAlg->setProperty("ComponentName", componentNames->cell<std::string>(row_num));
    rotateAlg->setProperty("X", rotxs->cell<double>(row_num));
    rotateAlg->setProperty("Y", rotys->cell<double>(row_num));
    rotateAlg->setProperty("Z", rotzs->cell<double>(row_num));
    rotateAlg->setProperty("Angle", rotangs->cell<double>(row_num));
    // [IMPORTANT]
    // The rotation required here has to be the RELATIVE rotation angle in
    // degrees
    rotateAlg->setProperty("RelativeRotation", false);
    rotateAlg->execute();
  }

  // Config output
  setProperty("Workspace", ws);

  g_log.notice() << "Finished applying CORELLI calibration\n";
}

} // namespace Algorithms

} // namespace Mantid
