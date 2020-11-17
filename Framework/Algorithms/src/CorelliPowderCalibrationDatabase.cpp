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
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"

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

CalibrationTableHandler::CalibrationTableHandler() : mCalibWS {nullptr}, isSingleComponentTable {false} {}

//-----------------------------------------------------------------------------
/**
 * @brief Create single component calibration table
 * @param wsname
 * @return
 */
DataObjects::TableWorkspace_sptr CalibrationTableHandler::createComponentCalibrationTable(const std::string &wsname) {

    // Create table workspace
    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();

    // Add to ADS if workspace name is given
    if (wsname.size() > 0) {
        AnalysisDataService::Instance().addOrReplace(wsname, itablews);
    }

    TableWorkspace_sptr tablews =
        std::dynamic_pointer_cast<TableWorkspace>(itablews);

    // Set up columns
    tablews->addColumn("str", "YYYMMDD");  // first column as date stamp
    for (size_t i = 1;
         i < CorelliCalibration::calibrationTableColumnNames.size() - 1; ++i) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    return tablews;
}

/**
 * @brief Append a calibration position to the table
 * @param tablews
 * @param datestamp
 * @param pos
 */
void CalibrationTableHandler::appendCalibration(DataObjects::TableWorkspace_sptr tablews, const std::string &datestamp, ComponentPosition& pos) {
    // Append a new row
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    // Date and positions
    sourceRow << datestamp << pos.x << pos.y << pos.z << pos.xCosine << pos.yCosine << pos.zCosine << pos.rotAngle;
}


/**
 * @brief Set a valid calibration table to the handler
 * @param calibws
 */
void CalibrationTableHandler::setCalibrationTable(
    DataObjects::TableWorkspace_sptr calibws) {

    // Check the column of input calibration workspace
    std::vector<std::string> colnames = calibws->getColumnNames();
    if (colnames.size() != calibrationTableColumnNames.size())
        throw std::runtime_error("Input TableWorkspace does not have expected number of columns");
    for(size_t i = 0; i < colnames.size(); ++i)
        if (colnames[i] != calibrationTableColumnNames[i])
            throw std::runtime_error("Input TableWorkspace does not have the expected column name");

    // Set
    mCalibWS = calibws;
}

/**
 * @brief Get column names of the table workspace
 * @return
 */
std::vector<std::string> CalibrationTableHandler::getComponentNames() {

  // Check workspace type
  if (isSingleComponentTable)
      throw std::runtime_error("TableWorkspace contains a single component's calibration in various dates");

  std::vector<std::string> names{};

  for (size_t i = 0; i < mCalibWS->rowCount(); ++i) {
      std::string compname = mCalibWS->cell<std::string>(i, 0);
      names.push_back(compname);
  }

  return names;
}

/**
 * @brief Get the calibration position of a single
 * @param component
 * @return
 */
ComponentPosition CalibrationTableHandler::getComponentCalibratedPosition(const std::string &component) {
    // Check
    if (!mCalibWS)
        throw std::runtime_error("Calibration workspace has not been set up yet.");

    // Get the row number of the specified component
    size_t row_number = mCalibWS->rowCount();
    for (size_t i = 0; i < row_number; ++i) {
        if (mCalibWS->cell<std::string>(i, 0) == component)
        {
            row_number = i;
            break;
        }
    }
    // Check
    if (row_number == mCalibWS->rowCount())
        throw std::runtime_error("Specified component does not exist");

    // Get the values
    ComponentPosition pos;
    pos.x = mCalibWS->cell<double>(row_number, 1);
    pos.y = mCalibWS->cell<double>(row_number, 2);
    pos.z = mCalibWS->cell<double>(row_number, 3);
    pos.xCosine = mCalibWS->cell<double>(row_number, 4);
    pos.yCosine = mCalibWS->cell<double>(row_number, 5);
    pos.zCosine = mCalibWS->cell<double>(row_number, 6);
    pos.rotAngle = mCalibWS->cell<double>(row_number, 7);

    return pos;
}

/**
 * @brief Load single component calibration file to table workspace
 * @param filename
 * @return
 */
DataObjects::TableWorkspace_sptr CalibrationTableHandler::loadComponentCalibrationTable(const std::string &filename,
                                                                                        const std::string &tablewsname) {
    // Get algorithm handler
    IAlgorithm_sptr loadAsciiAlg =
        AlgorithmFactory::Instance().create("LoadAscii", 2);
    // Set parameters
    loadAsciiAlg->initialize();
    loadAsciiAlg->setPropertyValue("Filename", filename);
    loadAsciiAlg->setPropertyValue("OutputWorkspace", tablewsname);
    loadAsciiAlg->setPropertyValue("Separator", "CSV");
    loadAsciiAlg->setPropertyValue("CommentIndicator", "#");
    loadAsciiAlg->execute();
    // Convert to TableWorkspace
    TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve(tablewsname));

    return tablews;
}


/**
 * @brief CalibrationTableHandler::load
 * @param filename
 */
void CalibrationTableHandler::load(const std::string &filename) {
  std::cout << "Load from " << filename << ": existence = " << boost::filesystem::exists(filename) << "\n";
}

//-----------------------------------------------------------------------------
/**
 * @brief Save a specific component to database (csv) file
 * @param component
 * @param filename
 */
void CalibrationTableHandler::saveCompomentDatabase(const std::string &datestamp,
    const std::string &component,
    const std::string &filename) {

  // create worksapce name
  std::string tablewsname = component + "_" + datestamp;

  // Check whether the file does exist or not: new or append
  TableWorkspace_sptr compcaltable = nullptr;
  bool appendmode{false};
  if (boost::filesystem::exists(filename)) {
      compcaltable = loadComponentCalibrationTable(filename, tablewsname);
      appendmode = true;
  } else {
      compcaltable = createComponentCalibrationTable(tablewsname);
  }

  // Append the row
  ComponentPosition componentpos = getComponentCalibratedPosition(component);
  appendCalibration(compcaltable, datestamp, componentpos);

  std::cout << "Save " << mCalibWS->getName() << " to " << filename << "\n";

  // create algorithm: only version 2 of SaveAscii can work with TableWorkspace
  IAlgorithm_sptr saveAsciiAlg =
      AlgorithmFactory::Instance().create("SaveAscii", 2);
  saveAsciiAlg->initialize();
  saveAsciiAlg->setPropertyValue("InputWorkspace", mCalibWS->getName());  // std::dynamic_pointer_cast<ITableWorkspace>(mCalibWS));  //mCalibWS->getName());
  saveAsciiAlg->setProperty("Filename", filename);
  saveAsciiAlg->setPropertyValue("CommentIndicator", "#");
  saveAsciiAlg->setPropertyValue("Separator", "CSV");
  saveAsciiAlg->setProperty("ColumnHeader", true);
  saveAsciiAlg->setProperty("AppendToFile", appendmode);

  std::cout << "Execute Save .... \n";
  saveAsciiAlg->execute();

  return;
}

//-----------------------------------------------------------------------------
/**
 * @brief Save full calibration table to a database (CSV) file
 * @param filename
 */
void CalibrationTableHandler::saveCalibrationTable(
    const std::string &filename) {
  std::cout << "Save calibration table" << mCalibWS->getName() << " to " << filename << "\n";
  // create algorithm: only version 2 of SaveAscii can work with TableWorkspace
  IAlgorithm_sptr saveAsciiAlg =
      AlgorithmFactory::Instance().create("SaveAscii", 2);
  saveAsciiAlg->initialize();
  saveAsciiAlg->setPropertyValue("InputWorkspace", mCalibWS->getName());  // std::dynamic_pointer_cast<ITableWorkspace>(mCalibWS));  //mCalibWS->getName());
  saveAsciiAlg->setProperty("Filename", filename);
  saveAsciiAlg->setPropertyValue("CommentIndicator", "#");
  saveAsciiAlg->setPropertyValue("Separator", "CSV");
  saveAsciiAlg->setProperty("ColumnHeader", true);

  std::cout << "Execute Save .... \n";
  saveAsciiAlg->execute();
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

  std::string filename = corelliComponentDatabaseName(
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
/**
 * @brief Compose a standard full path of a component CSV file
 * @param componentname
 * @param directory
 * @return
 */
std::string CorelliPowderCalibrationDatabase::corelliComponentDatabaseName(const std::string &componentname, const std::string &directory) {
  std::string basename = componentname + ".csv";
  std::string filename = joinPath(directory, basename);

  return filename;
}

//-----------------------------------------------------------------------------
/**
 * @brief Compose a standard full path of a calibration table CSV file
 * @param datestamp
 * @param directory
 * @return
 */
std::string CorelliPowderCalibrationDatabase::corelliCalibrationDatabaseName(
    const std::string &datestamp, const std::string &directory) {
  std::string basename = datestamp + ".csv";
  std::string filename = joinPath(directory, basename);
  return filename;
}

//-----------------------------------------------------------------------------
/**
 * @brief Check whether a file does exist
 * @param filepath
 * @return
 */
bool CorelliPowderCalibrationDatabase::isFileExist(
    const std::string &filepath) {

  // TODO - replace by std::filesystem::exists(filename) until C++17 is properly
  // supported
  return boost::filesystem::exists(filepath);
}

//-----------------------------------------------------------------------------
/**
 * @brief Join directory and base file name for a full path
 * @param directory
 * @param basename
 * @return
 */
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
