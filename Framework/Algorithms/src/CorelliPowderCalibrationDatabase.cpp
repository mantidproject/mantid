// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CorelliPowderCalibrationDatabase.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <sstream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using Types::Core::DateAndTime;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorelliPowderCalibrationDatabase)

/** Initialize the algorithm's properties.
 */
void CorelliPowderCalibrationDatabase::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<InstrumentValidator>();

  // Input MatrixWorkspace which the calibration run is from
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "An input workspace.");

  // Input calibration patch TableWorkspace
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "InputCalibrationPatchWorkspace", "", Direction::Input),
                  "An input table workspace for calibration patch.");

  // Output directory
  declareProperty(std::make_unique<FileProperty>("DatabaseDirectory", "",
                                                 FileProperty::Directory),
                  "The directory that the database (csv) files are saved to");

  // Optional output calibration TableWorkspace
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
                  "An output calibration workspace.");

}

// Validate inputs workspace first.
std::map<std::string, std::string> CorelliPowderCalibrationDatabase::validateInputs() {
  std::map<std::string, std::string> errors;

  inputWS = getProperty("InputWorkspace");

//  std::vector<std::string> allowedCalibrationTableColumns {"ComponentName", "Xposition", "Yposition",
//                                                           "Zposition", "XdirectionCosine", "YdirectionCosine",
//                                                           "ZdirectionCosine", "RotationAngle"};

  // check for null pointers - this is to protect against workspace groups
  if (!inputWS) {
    return errors;
  }

  // This algorithm will only work for CORELLI, check for CORELLI.
  if (inputWS->getInstrument()->getName() != "CORELLI")
    errors["InputWorkspace"] = "This Algorithm will only work for Corelli.";
  // Must include start_time
  else if (!inputWS->run().hasProperty("start_time"))
    errors["InputWorkspace"] = "Workspace is missing property start_time.";

  auto run = inputWS->run();
  auto properties = run.getProperties();
  std::cout << "Number of properties: " << properties.size() << "\n";
  for (auto p: properties) {
      std::cout << "Property: " << p->name() << "\n";
  }

  // check for calibration patch table workspace
  inputCalibrationTableWS = getProperty("InputCalibrationPatchWorkspace");
  if (!inputCalibrationTableWS) {
      errors["InputCalibrationPatchWorkspace"] = "Input calibration patch workspace is not specified";
      return errors;
  }
  // Check columns
  else {
     std::vector<std::string> colNames = inputCalibrationTableWS->getColumnNames();
     if (colNames.size() != CorelliCalibration::calibrationTableColumnNames.size()) {
         errors["InputCalibrationPatchWorkspace"] = "Number of columns in table workspace is not correct";
     }
     for (size_t i = 0; i < colNames.size(); ++i) {
         if (colNames[i] != CorelliCalibration::calibrationTableColumnNames[i]) {
             errors["InputCalibrationPatchWorkspace"] = "Calibration table columns do not match";
             break;
         }
     }
  }

  return errors;
}

/** Execute the algorithm.
 */
void CorelliPowderCalibrationDatabase::exec() {
  // parse input
  inputWS = getProperty("InputWorkspace");
  inputCalibrationTableWS = getProperty("InputCalibrationPatchWorkspace");


  // output
  // setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
