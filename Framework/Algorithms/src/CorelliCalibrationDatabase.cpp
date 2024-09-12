// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CorelliCalibrationDatabase.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <filesystem>
#include <sstream>
#include <string>

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using Types::Core::DateAndTime;

namespace CorelliCalibration {

//-----------------------------------------------------------------------------
/**
 * @brief CalibrationTableHandler constructor
 */
CalibrationTableHandler::CalibrationTableHandler() : mCalibWS{nullptr}, isSingleComponentTable{false} {}

//-----------------------------------------------------------------------------
/**
 * @brief Create an empty calibration table (multi-component) or an emtpy
 * history-of-compoment-positions table
 *
 * @param wsname: workspace name.  If we pass a name, then register the
 * table in the Analysis Data Service
 * @param iscomponent: if True, then create a history-of-compoment-positions
 * table, otherwise a calibration table
 *
 * @return shared pointer to the table workspace
 */
DataObjects::TableWorkspace_sptr CalibrationTableHandler::createCalibrationTableWorkspace(const std::string &wsname,
                                                                                          bool iscomponent) {

  // Create table workspace
  ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();

  // Add to ADS if workspace name is given
  if (wsname.size() > 0) {
    AnalysisDataService::Instance().addOrReplace(wsname, itablews);
  }

  TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(itablews);

  // Set up columns
  if (iscomponent)
    tablews->addColumn("str", "YYYMMDD"); // first column as date stamp
  else {
    tablews->addColumn("str", "ComponentName"); // first column as date stamp
  }
  for (size_t i = 1; i < CorelliCalibration::calibrationTableColumnNames.size(); ++i) {
    std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
    std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
    tablews->addColumn(type, colname);
  }

  return tablews;
}

//-----------------------------------------------------------------------------
/**
 * @brief Check whether a TableWorkspace is a valid Corelli calibration table
 * for all components.
 *
 * @details Cheks performed are correct number of columns and columns names
 *
 * @param calibws: Calibration table workspace
 * @param errormsg: (output) error message
 * @return
 */
bool CalibrationTableHandler::isValidCalibrationTableWorkspace(const DataObjects::TableWorkspace_sptr &calibws,
                                                               std::string &errormsg) {
  // Check columns of
  std::vector<std::string> colNames = calibws->getColumnNames();

  bool valid = true;

  if (colNames.size() != CorelliCalibration::calibrationTableColumnNames.size()) {
    // column numbers mismatch
    std::stringstream errorss;
    errorss << "Calibration table workspace requires " << CorelliCalibration::calibrationTableColumnNames.size()
            << " columns.  Input workspace " << calibws->getName() << " get " << calibws->getColumnNames().size()
            << "instead.";
    errormsg = errorss.str();
    valid = false;
  } else {

    // Check columns one by one
    for (size_t i = 0; i < colNames.size(); ++i) {
      if (colNames[i] != CorelliCalibration::calibrationTableColumnNames[i]) {
        std::stringstream errorss;
        errorss << i << "-th column is supposed to be " << CorelliCalibration::calibrationTableColumnNames[i]
                << ", but instead in TableWorkspace " << calibws->getName() << " it is " << colNames[i];
        errormsg = errorss.str();
        valid = false;
        break;
      }
    }
  }

  return valid;
}

//-----------------------------------------------------------------------------
/**
 * @brief Append a new dated position to the table of history of positions
 *
 * @param tablews: table workspace
 * @param datestamp: a day-stamp with format YYYYMMDD
 * @param pos: location and orientation of the component
 */
void CalibrationTableHandler::appendCalibration(const DataObjects::TableWorkspace_sptr &tablews,
                                                const std::string &datestamp, const ComponentPosition &pos) {
  // check
  if (tablews->columnCount() != calibrationTableColumnNames.size()) {
    throw std::runtime_error("Single component calibration table workspace is not correct.");
  }

  // Append a new row
  Mantid::API::TableRow sourceRow = tablews->appendRow();
  // Date and positions
  sourceRow << datestamp << pos.x << pos.y << pos.z << pos.xCosine << pos.yCosine << pos.zCosine << pos.rotAngle;
}

/**
 * @brief Set a valid calibration table to the handler. This cannot be a
 * table for history-of-component-positions, but a calibration table.
 * @param calibws
 */
void CalibrationTableHandler::setCalibrationTable(const DataObjects::TableWorkspace_sptr &calibws) {

  std::string errmsg{""};

  if (!isValidCalibrationTableWorkspace(calibws, errmsg))
    throw std::runtime_error(errmsg);

  // Set
  mCalibWS = calibws;
}

/**
 * @brief Get component names of the table workspace
 *
 * @details the component names are the first column of the table
 *
 * @throws std::runtime_error for single-component tables
 *
 * @return names as a vector or strings
 */
std::vector<std::string> CalibrationTableHandler::getComponentNames() {

  // Check workspace type
  if (isSingleComponentTable)
    throw std::runtime_error("TableWorkspace contains a single component's "
                             "calibration in various dates");

  std::vector<std::string> names = mCalibWS->getColVector<std::string>(0);

  return names;
}

/**
 * @brief extract the location and orientation for one of the components
 * in a calibration table
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
    if (mCalibWS->cell<std::string>(i, 0) == component) {
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
 * @brief Load a single-component database file to a table workspace of
 * history of positions for the component
 *
 * @details the single-component calibration file contains the history of
 * positions for said component
 *
 * @param filename
 * @package tablewsname : name for TableWorkspace the file is loaded to
 * @return shared pointer to the table workspace
 */
DataObjects::TableWorkspace_sptr
CalibrationTableHandler::loadComponentCalibrationTable(const std::string &filename, const std::string &tablewsname) {
  // Get algorithm handler
  auto loadAsciiAlg = AlgorithmFactory::Instance().create("LoadAscii", 2);
  // Set parameters
  if (tablewsname.size() == 0) {
    throw std::runtime_error("Failed to load ASCII as OutputWorkspace name is empty string.");
  }
  loadAsciiAlg->initialize();
  loadAsciiAlg->setPropertyValue("Filename", filename);
  loadAsciiAlg->setPropertyValue("OutputWorkspace", tablewsname);
  loadAsciiAlg->setPropertyValue("Separator", "CSV");
  loadAsciiAlg->setPropertyValue("CommentIndicator", "#");
  loadAsciiAlg->execute();
  // Convert to TableWorkspace
  TableWorkspace_sptr tablews =
      std::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve(tablewsname));

  return tablews;
}

//-----------------------------------------------------------------------------
/**
 * @brief Save a specific component to database (csv) file
 *
 * @details Extract the position of a specific component from the full
 * calibration table, and append to a database file. If the database file
 * does not exits, then create it.
 *
 * @param datestamp: YYYYMMDD date stamp
 * @param component: component name
 * @param filename: full path of the database file for the specific component
 *
 * @returns table of history of positions for the specific component
 */
TableWorkspace_sptr CalibrationTableHandler::saveCompomentDatabase(const std::string &datestamp,
                                                                   const std::string &component,
                                                                   const std::string &filename) {

  std::string tablewsname = component + "_" + datestamp;

  // Load the database file for the specific component to a table workspace
  // if extant, otherwise instantiate an empty table
  TableWorkspace_sptr compcaltable = nullptr;
  if (std::filesystem::exists(filename)) {
    compcaltable = loadComponentCalibrationTable(filename, tablewsname);
  } else {
    compcaltable = createCalibrationTableWorkspace(tablewsname, true);
  }

  // Append a new row to the table containing the hisotry of positions for
  // the specific component
  ComponentPosition componentpos = getComponentCalibratedPosition(component);
  appendCalibration(compcaltable, datestamp, componentpos);

  // save the updated history of positions to the database file. Will overwrite
  // the file if extant
  // Note: only version 2 of SaveAscii can work with TableWorkspace
  auto saveAsciiAlg = AlgorithmFactory::Instance().create("SaveAscii", 2);
  saveAsciiAlg->initialize();
  saveAsciiAlg->setProperty("InputWorkspace", compcaltable);
  saveAsciiAlg->setProperty("Filename", filename);
  saveAsciiAlg->setPropertyValue("CommentIndicator", "#");
  saveAsciiAlg->setPropertyValue("Separator", "CSV");
  saveAsciiAlg->setProperty("ColumnHeader", true);
  saveAsciiAlg->setProperty("AppendToFile",
                            false); // always overwrite original file
  // run
  saveAsciiAlg->execute();

  return compcaltable;
}

//-----------------------------------------------------------------------------
/**
 * @brief Save full calibration table to a database (CSV) file
 * @param filename
 */
void CalibrationTableHandler::saveCalibrationTable(const std::string &filename) {
  // create algorithm: only version 2 of SaveAscii can work with TableWorkspace
  auto saveAsciiAlg = AlgorithmFactory::Instance().create("SaveAscii", 2);
  saveAsciiAlg->initialize();
  saveAsciiAlg->setProperty("InputWorkspace", mCalibWS);
  saveAsciiAlg->setProperty("Filename", filename);
  saveAsciiAlg->setPropertyValue("CommentIndicator", "#");
  saveAsciiAlg->setPropertyValue("Separator", "CSV");
  saveAsciiAlg->setProperty("ColumnHeader", true);
  // run
  saveAsciiAlg->execute();
}

//-----------------------------------------------------------------------------
ComponentPosition
CalibrationTableHandler::getLatestCalibratedPosition(const DataObjects::TableWorkspace_sptr &componentcaltable) {

  size_t num_rows = componentcaltable->rowCount();
  ComponentPosition pos = CalibrationTableHandler::getCalibratedPosition(componentcaltable, num_rows - 1);

  return pos;
}

//-----------------------------------------------------------------------------
ComponentPosition
CalibrationTableHandler::getCalibratedPosition(const DataObjects::TableWorkspace_sptr &componentcaltable,
                                               size_t rownumber) {
  // Get the values
  ComponentPosition pos;

  pos.x = componentcaltable->cell<double>(rownumber, 1);
  pos.y = componentcaltable->cell<double>(rownumber, 2);
  pos.z = componentcaltable->cell<double>(rownumber, 3);
  pos.xCosine = componentcaltable->cell<double>(rownumber, 4);
  pos.yCosine = componentcaltable->cell<double>(rownumber, 5);
  pos.zCosine = componentcaltable->cell<double>(rownumber, 6);
  pos.rotAngle = componentcaltable->cell<double>(rownumber, 7);

  return pos;
}

} // namespace CorelliCalibration

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorelliCalibrationDatabase)

/** Initialize the algorithm's properties.
 */
void CorelliCalibrationDatabase::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<InstrumentValidator>();

  // Input MatrixWorkspace which the calibration run is from
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "Workspace containing the day-stamp of the calibration");

  // Input calibration patch TableWorkspace
  declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>("InputCalibrationPatchWorkspace", "", Direction::Input),
      "Table workspace containing calibrated positions and "
      "orientations for a subset of the banks");

  // Output directory
  declareProperty(std::make_unique<FileProperty>("DatabaseDirectory", "", FileProperty::Directory),
                  "The directory that the database (csv) files are saved to");

  // Optional output calibration TableWorkspace
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Table workspace containing calibrated positions and "
                  "orientations for all banks");
}

// Validate inputs workspace first.
std::map<std::string, std::string> CorelliCalibrationDatabase::validateInputs() {
  std::map<std::string, std::string> errors;

  mInputWS = getProperty("InputWorkspace");

  // check for null pointers - this is to protect against workspace groups
  if (!mInputWS) {
    return errors;
  }

  // This algorithm will only work for CORELLI, check for CORELLI.
  if (mInputWS->getInstrument()->getName() != "CORELLI")
    errors["InputWorkspace"] = "This Algorithm will only work for Corelli.";
  // Must include start_time
  else if (!mInputWS->run().hasProperty("start_time") && !mInputWS->run().hasProperty("run_start"))
    errors["InputWorkspace"] = "Workspace is missing property start_time.";

  // check for calibration patch table workspace
  mInputCalibrationTableWS = getProperty("InputCalibrationPatchWorkspace");
  if (!mInputCalibrationTableWS) {
    errors["InputCalibrationPatchWorkspace"] = "Input calibration patch workspace is not specified";
    return errors;
  }
  // Check columns
  else {
    std::string error_msg{""};
    bool isvalid = CorelliCalibration::CalibrationTableHandler::isValidCalibrationTableWorkspace(
        mInputCalibrationTableWS, error_msg);

    if (!isvalid) {
      errors["InputCalibrationPatchWorkspace"] = error_msg;
    }
  }

  return errors;
}

//-----------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CorelliCalibrationDatabase::exec() {
  // parse input
  if (!mInputWS)
    throw std::runtime_error("input workspace not specified");
  if (!mInputCalibrationTableWS)
    throw std::runtime_error("input calibration workspace not specified");

  std::string calibDatabaseDir = getProperty("DatabaseDirectory");

  // map for (component name, component calibration workspace
  std::vector<std::string> orderedcomponents = retrieveInstrumentComponents(mInputWS);

  std::map<std::string, TableWorkspace_sptr> component_caibws_map;
  setComponentMap(orderedcomponents, component_caibws_map);

  // Update component CSV files
  updateComponentDatabaseFiles(calibDatabaseDir, component_caibws_map);

  // Load data file if necessary and possible: component_caibws_map
  loadNonCalibratedComponentDatabase(calibDatabaseDir, component_caibws_map);

  // Create summary calibration workspace: input: component_caibws_map output:
  // new calibration workspace
  createOutputCalibrationTable(component_caibws_map, orderedcomponents);

  // Create the summary CSV file
  saveCalibrationTable(calibDatabaseDir);

  // Clean up memory
  for (const auto &[compname, calibws] : component_caibws_map) {
    if (calibws) {
      g_log.debug() << "Removing " << compname << "calibration table from ADS\n";
      AnalysisDataService::Instance().remove(calibws->getName());
    }
  }

  // output
  setProperty("OutputWorkspace", mOutputWS);
}

/**
 * @brief update component database files from table
 * @param calibdbdir: directory where dababase files are stored
 * @param calibwsmap: in/out map for bank and calibration tableworkspace in ADS
 */
void CorelliCalibrationDatabase::updateComponentDatabaseFiles(const std::string &calibdbdir,
                                                              std::map<std::string, TableWorkspace_sptr> &calibwsmap) {
  // Date stamp
  std::string timestampstr{""};
  if (mInputWS->run().hasProperty("start_time")) {
    // string value from start_time
    timestampstr = mInputWS->run().getProperty("start_time")->value();
  } else {
    // string value from run_start if start_time does not exist.
    // with input workspace validate, there must be at least one exist
    // between start_time and run_start
    timestampstr = mInputWS->run().getProperty("run_start")->value();
  }
  // convert
  mDateStamp = convertTimeStamp(timestampstr);

  // Handler
  CorelliCalibration::CalibrationTableHandler handler = CorelliCalibration::CalibrationTableHandler();
  handler.setCalibrationTable(mInputCalibrationTableWS);

  // Loop over all the components that have been calibrated in the calibration
  // table
  size_t num_rows = mInputCalibrationTableWS->rowCount();
  for (size_t i = 0; i < num_rows; ++i) {
    // get component name
    std::string compname = mInputCalibrationTableWS->cell<const std::string>(i, 0);
    // get file name
    std::string compdbname = corelliComponentDatabaseName(compname, calibdbdir);
    // save
    TableWorkspace_sptr comptablews = handler.saveCompomentDatabase(mDateStamp, compname, compdbname);
    // add the map
    calibwsmap[compname] = comptablews;
    g_log.debug() << "Component " << compname << " is updated to " << compdbname << " and saved to "
                  << comptablews->getName() << "\n";
  }
}

/**
 * @brief Load database file of certain banks if they do not appear in mCalibWS
 * @param calibdbdir: calibration database directory
 * @param calibwsmap: map from component name to calibration table workspace
 */
void CorelliCalibrationDatabase::loadNonCalibratedComponentDatabase(
    const std::string &calibdbdir, std::map<std::string, TableWorkspace_sptr> &calibwsmap) {
  // go through all the components
  for (const auto &[componentname, componentcaltable] : calibwsmap) {
    // check whether the calibration workspace has been loaded
    if (componentcaltable) // skip if it has been loaded
      continue;

    // locate the file
    std::string compdbname = corelliComponentDatabaseName(componentname, calibdbdir);
    if (!isFileExist(compdbname)) // skip if the file does not exist
    {
      // skip if the database file does not exist
      g_log.debug() << "Component " << componentname << ": No database file is found at " << compdbname << "\n";
      continue;
    }

    // load the database (csv) file and set
    TableWorkspace_sptr loaded_compcalibws = CorelliCalibration::CalibrationTableHandler::loadComponentCalibrationTable(
        compdbname, componentname + "_" + mDateStamp);
    calibwsmap[componentname] = loaded_compcalibws;

    g_log.debug() << "Component " << componentname << " is loaded from " << compdbname << " and saved to "
                  << loaded_compcalibws->getName() << "\n";
  }
}

// Create summary calibration workspace: input: component_caibws_map output:
// new calibration workspace
void CorelliCalibrationDatabase::createOutputCalibrationTable(std::map<std::string, TableWorkspace_sptr> &calibwsmap,
                                                              const std::vector<std::string> &orderedcomponents) {
  // Create an empty calibration table without setting to analysis data service
  mOutputWS = CorelliCalibration::CalibrationTableHandler::createCalibrationTableWorkspace("", false);
  CorelliCalibration::CalibrationTableHandler handler = CorelliCalibration::CalibrationTableHandler();
  handler.setCalibrationTable(mOutputWS);

  for (const auto &componentname : orderedcomponents) {
    const auto componentcaltable = calibwsmap[componentname];
    if (componentcaltable) {
      // only take care of calibrated components (before and now)
      auto lastpos = CorelliCalibration::CalibrationTableHandler::getLatestCalibratedPosition(componentcaltable);
      handler.appendCalibration(mOutputWS, componentname, lastpos);
    }
  }
}

// Create the summary CSV file
// File name exampe: corelli_instrument_20202015.csv
void CorelliCalibrationDatabase::saveCalibrationTable(const std::string &calibdbdir) {
  // file name
  std::string filename = "corelli_instrument_" + mDateStamp + ".csv";
  filename = joinPath(calibdbdir, filename);

  // Call the handler: set and save
  CorelliCalibration::CalibrationTableHandler handler = CorelliCalibration::CalibrationTableHandler();
  handler.setCalibrationTable(mOutputWS);
  handler.saveCalibrationTable(filename);
}

//-----------------------------------------------------------------------------
/**
 * @brief A static method to convert Mantid datetime string to YYYYMMDD format
 * @param run_start_time: str as run start time in format of YYYY-MM-DDTHH:MM:SS
 * @return
 */
std::string CorelliCalibrationDatabase::convertTimeStamp(const std::string &run_start_time) {
  // Get the first sub string by
  std::string date_str = run_start_time.substr(0, run_start_time.find("T"));

  // Separate year date and time
  std::string year = date_str.substr(0, date_str.find("-"));
  std::string monthday = date_str.substr(date_str.find("-") + 1, date_str.size()); // +1 to ignore delimit '-'
  std::string month = monthday.substr(0, monthday.find("-"));
  std::string day = monthday.substr(monthday.find("-") + 1,
                                    monthday.size()); // +1 to ignore delimit
  std::string datestamp = year + month + day;

  return datestamp;
}

//-----------------------------------------------------------------------------
/**
 * @brief Compose a standard full path of a component CSV file
 *
 * @details names for bank components follow the pattern bankXX/sixteenpack
 * and we drop the "/sixteenpack" suffix when composing the name of the CSV file
 *
 * @param componentname
 * @param directory
 * @return
 */
std::string CorelliCalibrationDatabase::corelliComponentDatabaseName(const std::string &componentname,
                                                                     const std::string &directory) {

  // drop the suffix "/sixteenpack" if found in the component name
  std::string shortName = componentname;
  std::string suffix{"/sixteenpack"};
  size_t pos = componentname.find(suffix);
  if (pos != std::string::npos)
    shortName.erase(pos, suffix.length());

  std::string basename = shortName + ".csv";
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
std::string CorelliCalibrationDatabase::corelliCalibrationDatabaseName(const std::string &datestamp,
                                                                       const std::string &directory) {
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
bool CorelliCalibrationDatabase::isFileExist(const std::string &filepath) {

  // TODO - replace by std::filesystem::exists(filename) until C++17 is properly
  // supported
  return std::filesystem::exists(filepath);
}

//-----------------------------------------------------------------------------
/**
 * @brief Join directory and base file name for a full path
 * @param directory
 * @param basename
 * @return
 */
std::string CorelliCalibrationDatabase::joinPath(const std::string &directory, const std::string &basename) {
  std::filesystem::path dir(directory);
  std::filesystem::path file(basename);
  std::filesystem::path fullpath = dir / file;

  return fullpath.string();
}

/**
 * @brief Set up component map
 *
 * @details keys of this map are "moderator", "sample-position",
 * "bank1/sixteenpack",..,"bank91/sixteenpack"
 * @param componentnames
 * @param compmap
 */
void CorelliCalibrationDatabase::setComponentMap(const std::vector<std::string> &componentnames,
                                                 std::map<std::string, DataObjects::TableWorkspace_sptr> &compmap) {
  // Add entries
  for (const auto &compname : componentnames)
    compmap[compname] = nullptr;
}

/**
 * @brief Retrieve the names of certain intrument compoments
 *
 * @details names of interest are are "moderator", "sample-position",
 * "bank1/sixteenpack",..,"bank91/sixteenpack"
 *
 * @param ws : Any workspace containing instrument
 * @return  : vector including of all the compoments in the order as
 * moderator, sample-position, bank1, bank2, ...
 */
std::vector<std::string> CorelliCalibrationDatabase::retrieveInstrumentComponents(const MatrixWorkspace_sptr &ws) {
  // Get access to instrument information
  const auto &component_info = ws->componentInfo();

  // Init output
  std::vector<std::string> componentnames = {"moderator", "sample-position"};

  // Loop over all the components for bankX/sixteenpack
  const size_t num_components = component_info.size();
  for (size_t i = 0; i < num_components; ++i) {
    std::string compname = component_info.name(i);
    // a component starts with bank must be a bank
    if (compname.compare(0, 4, "bank") == 0) {
      componentnames.push_back(compname + "/sixteenpack");
    }
  }

  return componentnames;
}

} // namespace Mantid::Algorithms
