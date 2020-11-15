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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <sstream>
#include <string>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using Types::Core::DateAndTime;

namespace CorelliCalibration {

CalibrationTableHandler::CalibrationTableHandler() { mCalibWS = nullptr; }

void CalibrationTableHandler::setCalibrationTable(
    DataObjects::TableWorkspace_sptr calibws) {
  mCalibWS = calibws;
}

std::vector<std::string> CalibrationTableHandler::getComponentNames() {
  std::vector<std::string> names{"source", "sample", "bank1"};
  return names;
}

void CalibrationTableHandler::load(const std::string &filename) {
  std::cout << "Load from " << filename << "\n";
}

void CalibrationTableHandler::saveCompomentDatabase(
    const std::string &filename) {
  std::cout << "Save to " << filename << "\n";
}

void CalibrationTableHandler::saveCalibrationTable(
    const std::string &filename) {
  std::cout << "Save to " << filename << "\n";
}

std::string CalibrationTableHandler::corelliComponentDatabaseName(
    const std::string &componentname, const std::string &directory) {
  std::string filename = directory + "/" + componentname + ".csv";
  return filename;
}

std::string CalibrationTableHandler::corelliCalibrationDatabaseName(
    const std::string &datestamp, const std::string &directory) {
  std::string filename = directory + "/" + datestamp + ".csv";
  return filename;
}
} // namespace CorelliCalibration

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
  declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>(
          "OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
      "An output calibration workspace.");
}

// Validate inputs workspace first.
std::map<std::string, std::string>
CorelliPowderCalibrationDatabase::validateInputs() {
  std::map<std::string, std::string> errors;

  inputWS = getProperty("InputWorkspace");

  //  std::vector<std::string> allowedCalibrationTableColumns {"ComponentName",
  //  "Xposition", "Yposition",
  //                                                           "Zposition",
  //                                                           "XdirectionCosine",
  //                                                           "YdirectionCosine",
  //                                                           "ZdirectionCosine",
  //                                                           "RotationAngle"};

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
  for (auto p : properties) {
    std::cout << "Property: " << p->name() << "\n";
  }

  // check for calibration patch table workspace
  inputCalibrationTableWS = getProperty("InputCalibrationPatchWorkspace");
  if (!inputCalibrationTableWS) {
    errors["InputCalibrationPatchWorkspace"] =
        "Input calibration patch workspace is not specified";
    return errors;
  }
  // Check columns
  else {
    std::vector<std::string> colNames =
        inputCalibrationTableWS->getColumnNames();
    if (colNames.size() !=
        CorelliCalibration::calibrationTableColumnNames.size()) {
      errors["InputCalibrationPatchWorkspace"] =
          "Number of columns in table workspace is not correct";
    }
    for (size_t i = 0; i < colNames.size(); ++i) {
      if (colNames[i] != CorelliCalibration::calibrationTableColumnNames[i]) {
        errors["InputCalibrationPatchWorkspace"] =
            "Calibration table columns do not match";
        break;
      }
    }
  }

  return errors;
}

//-----------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CorelliPowderCalibrationDatabase::exec() {
  // parse input
  if (!inputWS)
    throw std::runtime_error("input workspace not specified");
  if (!inputCalibrationTableWS)
    throw std::runtime_error("input calibration workspace not specified");

  std::string output_dir = getProperty("DatabaseDirectory");
  std::cout << "Output directory: " << output_dir << "\n";

  // Update component CSV files with
  // ... ...

  // Create the summary CSV file
  // ... ...

  // output
  // setProperty("OutputWorkspace", outputWS);
}

/**
 * @brief a
 * And the day stamp for this calibration is 20201025, the following line will
be appended to file corelli_source.csv : 20201025, 0, 0, -15.560, 0, 0, 0, 0 the
following line will be appended to file corelli_sample.csv: 20201025, 0.0001,
-0.0002, 0.003, 0, 0, 0, 0 and the following lines to corelli_bank001.csv:
20201025, 0.9678, 0.0056, 0.0003, 0.4563, -0.9999, 0.3424, 5.67
The header for files corelli_source.csv, corelli_sample.csv, and files
corelli_bankXXX.csv should be: # YYYMMDD, Xposition, Yposition, Zposition,
XdirectionCosine, YdirectionCosine, ZdirectionCosine, RotationAngle
 */
void CorelliPowderCalibrationDatabase::updateComponentDatabaseFiles() {

  std::string component = "sample";

  std::string filename =
      CorelliCalibration::CalibrationTableHandler::corelliComponentDatabaseName(
          component, "/tmp");
  std::cout << isFileExist(filename);
}

//-----------------------------------------------------------------------------
/**
 * @brief A static method to convert Mantid datetime string to YYYYMMDD format
 * @param run_start_time: str as run start time in format of YYYY-MM-DDTHH:MM:SS
 * @return
 */
std::string
CorelliPowderCalibrationDatabase::convertTimeStamp(std::string run_start_time) {
  // Get the first sub string by
  std::string date_str = run_start_time.substr(0, run_start_time.find("T"));
  std::cout << date_str << "\n";

  // Separate year date and time
  std::string year = date_str.substr(0, date_str.find("-"));
  std::string monthday = date_str.substr(
      date_str.find("-") + 1, date_str.size()); // +1 to ignore delimit '-'
  std::cout << "MondayDay = " << monthday << "\n";
  std::string month = monthday.substr(0, monthday.find("-"));
  std::string day = monthday.substr(monthday.find("-") + 1,
                                    monthday.size()); // +1 to ignore delimit

  std::cout << "Y M D: " << year << ", " << month << ", " << day << "\n";

  std::string datestamp = year + month + day;

  return datestamp;
}

//-----------------------------------------------------------------------------
bool CorelliPowderCalibrationDatabase::isFileExist(
    const std::string &filepath) {

  // TODO - replace by std::filesystem::exists(filename) until C++17 is properly
  // supported
  return boost::filesystem::exists(filepath);
}

std::string
CorelliPowderCalibrationDatabase::joinPath(const std::string directory,
                                           const std::string basename) {
  boost::filesystem::path dir(directory);
  boost::filesystem::path file(basename);
  boost::filesystem::path fullpath = dir / file;

  return fullpath.string();
}

} // namespace Algorithms
} // namespace Mantid
