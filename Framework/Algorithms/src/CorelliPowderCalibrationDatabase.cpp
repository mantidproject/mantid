// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CorelliPowderCalibrationDatabase.h"
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

CalibrationTableHandler::CalibrationTableHandler()
    : mCalibWS{nullptr}, isSingleComponentTable{false} {}

//-----------------------------------------------------------------------------
/**
 * @brief Create single component calibration table
 * @param wsname
 * @param iscomponent: flag if True single component workspace
 * @return
 */
DataObjects::TableWorkspace_sptr
CalibrationTableHandler::createCalibrationTableWorkspace(
    const std::string &wsname, bool iscomponent) {

  // Create table workspace
  ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();

  // Add to ADS if workspace name is given
  if (wsname.size() > 0) {
    AnalysisDataService::Instance().addOrReplace(wsname, itablews);
  }

  TableWorkspace_sptr tablews =
      std::dynamic_pointer_cast<TableWorkspace>(itablews);

  // Set up columns
  if (iscomponent)
    tablews->addColumn("str", "YYYMMDD"); // first column as date stamp
  else {
    tablews->addColumn("str", "ComponentName"); // first column as date stamp
  }
  for (size_t i = 1; i < CorelliCalibration::calibrationTableColumnNames.size();
       ++i) {
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
void CalibrationTableHandler::appendCalibration(
    DataObjects::TableWorkspace_sptr tablews, const std::string &datestamp,
    ComponentPosition &pos) {
  if (tablews->columnCount() != 8) {
    throw std::runtime_error(
        "Single component calibration table workspace is not correct.");
  }

  // Append a new row
  Mantid::API::TableRow sourceRow = tablews->appendRow();
  // Date and positions
  sourceRow << datestamp << pos.x << pos.y << pos.z << pos.xCosine
            << pos.yCosine << pos.zCosine << pos.rotAngle;
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
    throw std::runtime_error(
        "Input TableWorkspace does not have expected number of columns");
  for (size_t i = 0; i < colnames.size(); ++i)
    if (colnames[i] != calibrationTableColumnNames[i])
      throw std::runtime_error(
          "Input TableWorkspace does not have the expected column name");

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
    throw std::runtime_error("TableWorkspace contains a single component's "
                             "calibration in various dates");

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
ComponentPosition CalibrationTableHandler::getComponentCalibratedPosition(
    const std::string &component) {
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
 * @brief Load single component calibration file to table workspace
 * @param filename
 * @package tablewsname : name for TableWorkspace the file is loaded to
 * @return
 */
DataObjects::TableWorkspace_sptr
CalibrationTableHandler::loadComponentCalibrationTable(
    const std::string &filename, const std::string &tablewsname) {
  // Get algorithm handler
  IAlgorithm_sptr loadAsciiAlg =
      AlgorithmFactory::Instance().create("LoadAscii", 2);
  // Set parameters
  std::cout << "About to load " << filename << " to workspace " << tablewsname
            << "\n";
  if (tablewsname.size() == 0) {
    std::cout << "[BUG........] OutputWorkspace is empty string\n";
    throw std::runtime_error("Failed to load ascii.");
  }
  loadAsciiAlg->initialize();
  loadAsciiAlg->setPropertyValue("Filename", filename);
  loadAsciiAlg->setPropertyValue("OutputWorkspace", tablewsname);
  loadAsciiAlg->setPropertyValue("Separator", "CSV");
  loadAsciiAlg->setPropertyValue("CommentIndicator", "#");
  loadAsciiAlg->execute();
  // Convert to TableWorkspace
  TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(
      AnalysisDataService::Instance().retrieve(tablewsname));

  return tablews;
}

/**
 * @brief CalibrationTableHandler::load
 * @param filename
 */
void CalibrationTableHandler::load(const std::string &filename) {
  std::cout << "Load from " << filename
            << ": existence = " << boost::filesystem::exists(filename) << "\n";
}

//-----------------------------------------------------------------------------
/**
 * @brief Save a specific component to database (csv) file
 * @param component
 * @param filename
 */
TableWorkspace_sptr
CalibrationTableHandler::saveCompomentDatabase(const std::string &datestamp,
                                               const std::string &component,
                                               const std::string &filename) {

  // create worksapce name
  std::string tablewsname = component + "_" + datestamp;

  // Check whether the file does exist or not: new or append
  std::cout << "Create single component calibration table\n";
  TableWorkspace_sptr compcaltable = nullptr;
  if (boost::filesystem::exists(filename)) {
    compcaltable = loadComponentCalibrationTable(filename, tablewsname);
  } else {
    compcaltable = createCalibrationTableWorkspace(tablewsname, true);
  }

  // Append the row
  ComponentPosition componentpos = getComponentCalibratedPosition(component);
  appendCalibration(compcaltable, datestamp, componentpos);

  std::cout << "Save " << mCalibWS->getName() << " to " << filename << "\n";

  // create algorithm: only version 2 of SaveAscii can work with TableWorkspace
  IAlgorithm_sptr saveAsciiAlg =
      AlgorithmFactory::Instance().create("SaveAscii", 2);
  saveAsciiAlg->initialize();
  //  saveAsciiAlg->setPropertyValue(
  //      "InputWorkspace",
  //      tablewsname); //
  //      std::dynamic_pointer_cast<ITableWorkspace>(mCalibWS));
  //                    // //mCalibWS->getName());
  saveAsciiAlg->setProperty("InputWorkspace", compcaltable);
  saveAsciiAlg->setProperty("Filename", filename);
  saveAsciiAlg->setPropertyValue("CommentIndicator", "#");
  saveAsciiAlg->setPropertyValue("Separator", "CSV");
  saveAsciiAlg->setProperty("ColumnHeader", true);
  saveAsciiAlg->setProperty("AppendToFile",
                            false); // always overwrite original file

  std::cout << "Execute Save .... \n";
  saveAsciiAlg->execute();

  return compcaltable;
}

//-----------------------------------------------------------------------------
/**
 * @brief Save full calibration table to a database (CSV) file
 * @param filename
 */
void CalibrationTableHandler::saveCalibrationTable(
    const std::string &filename) {
  std::cout << "Save calibration table" << mCalibWS->getName() << " to "
            << filename << "\n";
  // create algorithm: only version 2 of SaveAscii can work with TableWorkspace
  IAlgorithm_sptr saveAsciiAlg =
      AlgorithmFactory::Instance().create("SaveAscii", 2);
  saveAsciiAlg->initialize();
  //  saveAsciiAlg->setPropertyValue(
  //      "InputWorkspace",
  //      mCalibWS
  //          ->getName()); //
  //          std::dynamic_pointer_cast<ITableWorkspace>(mCalibWS));
  //                        // //mCalibWS->getName());
  saveAsciiAlg->setProperty("InputWorkspace", mCalibWS);
  saveAsciiAlg->setProperty("Filename", filename);
  saveAsciiAlg->setPropertyValue("CommentIndicator", "#");
  saveAsciiAlg->setPropertyValue("Separator", "CSV");
  saveAsciiAlg->setProperty("ColumnHeader", true);

  std::cout << "Execute Save .... \n";
  saveAsciiAlg->execute();
}

//-----------------------------------------------------------------------------
ComponentPosition CalibrationTableHandler::getLatestCalibratedPosition(
    DataObjects::TableWorkspace_sptr componentcaltable) {

  size_t num_rows = componentcaltable->rowCount();
  ComponentPosition pos = CalibrationTableHandler::getCalibratedPosition(
      componentcaltable, num_rows - 1);

  return pos;
}

//-----------------------------------------------------------------------------
ComponentPosition CalibrationTableHandler::getCalibratedPosition(
    DataObjects::TableWorkspace_sptr componentcaltable, size_t rownumber) {
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
                      "OutputWorkspace", "", Direction::Output),
                  "An output calibration workspace.");
}

// Validate inputs workspace first.
std::map<std::string, std::string>
CorelliPowderCalibrationDatabase::validateInputs() {
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
  else if (!mInputWS->run().hasProperty("start_time"))
    errors["InputWorkspace"] = "Workspace is missing property start_time.";

  auto run = mInputWS->run();
  auto properties = run.getProperties();
  std::cout << "Number of properties: " << properties.size() << "\n";
  for (auto p : properties) {
    std::cout << "Property: " << p->name() << "\n";
  }

  // check for calibration patch table workspace
  mInputCalibrationTableWS = getProperty("InputCalibrationPatchWorkspace");
  if (!mInputCalibrationTableWS) {
    errors["InputCalibrationPatchWorkspace"] =
        "Input calibration patch workspace is not specified";
    return errors;
  }
  // Check columns
  else {
    std::vector<std::string> colNames =
        mInputCalibrationTableWS->getColumnNames();
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
  if (!mInputWS)
    throw std::runtime_error("input workspace not specified");
  if (!mInputCalibrationTableWS)
    throw std::runtime_error("input calibration workspace not specified");

  std::string calibDatabaseDir = getProperty("DatabaseDirectory");
  std::cout << "Output directory: " << calibDatabaseDir << "\n";

  // map for (component name, component calibration workspace
  std::vector<std::string> orderedcomponents =
      retrieveInstrumentComponents(mInputWS);

  std::map<std::string, TableWorkspace_sptr> component_caibws_map;
  setComponentMap(orderedcomponents, component_caibws_map);

  // Update component CSV files
  updateComponentDatabaseFiles(calibDatabaseDir, component_caibws_map);

  // Load data file if necessary and possible: component_caibws_map
  loadNonCalibratedComponentDatabase(calibDatabaseDir, component_caibws_map);

  // Create summary calibrtion workspace: input: component_caibws_map output:
  // new calibration workspace
  createOutputCalibrationTable(component_caibws_map, orderedcomponents);

  // Create the summary CSV file
  saveCalibrtionTable(calibDatabaseDir);

  // output
  setProperty("OutputWorkspace", mOutputWS);
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
void CorelliPowderCalibrationDatabase::updateComponentDatabaseFiles(
    const std::string &calibdbdir,
    std::map<std::string, TableWorkspace_sptr> &calibwsmap) {
  // Date stamp
  mDateStamp =
      convertTimeStamp(mInputWS->run().getProperty("start_time")->value());

  // Handler
  CorelliCalibration::CalibrationTableHandler handler =
      CorelliCalibration::CalibrationTableHandler();
  handler.setCalibrationTable(mInputCalibrationTableWS);

  // Loop over all the components that have been calibrated in the calibration
  // table
  size_t num_rows = mInputCalibrationTableWS->rowCount();
  for (size_t i = 0; i < num_rows; ++i) {
    // get component name
    std::string compname =
        mInputCalibrationTableWS->cell<const std::string>(i, 0);
    // get file name
    std::string compdbname = corelliComponentDatabaseName(compname, calibdbdir);
    // save
    TableWorkspace_sptr comptablews =
        handler.saveCompomentDatabase(mDateStamp, compname, compdbname);
    // add the map
    calibwsmap[compname] = comptablews;
    m_log.notice() << "Component " << compname << " is updated to "
                   << compdbname << " and saved to " << comptablews->getName()
                   << "\n";
  }
}

/**
 * @brief Load data file if necessary and possible: component_caibws_map
 * @param calibdrdir : calibration database directory
 * @param calibwsmap
 */
void CorelliPowderCalibrationDatabase::loadNonCalibratedComponentDatabase(
    const std::string &calibdbdir,
    std::map<std::string, TableWorkspace_sptr> &calibwsmap) {
  // go through all the components
  for (auto &[componentname, componentcaltable] : calibwsmap) {
    // check whether the calibration workspace has been loaded
    if (componentcaltable) // skip if it has been loaded
      continue;

    // locate the file
    std::string compdbname =
        corelliComponentDatabaseName(componentname, calibdbdir);
    if (!isFileExist(compdbname)) // skip if the file does not exist
    {
      // skip if the database file does not exist
      g_log.notice() << "Component " << componentname
                     << ": No database file is found at " << compdbname << "\n";
      continue;
    }

    // load the database (csv) file and set
    TableWorkspace_sptr loaded_compcalibws = CorelliCalibration::
        CalibrationTableHandler::loadComponentCalibrationTable(
            compdbname, componentname + "_" + mDateStamp);
    calibwsmap[componentname] = loaded_compcalibws;

    m_log.notice() << "Component " << componentname << " is loaded from "
                   << compdbname << " and saved to "
                   << loaded_compcalibws->getName() << "\n";
  }
}

// Create summary calibrtion workspace: input: component_caibws_map output:
// new calibration workspace
void CorelliPowderCalibrationDatabase::createOutputCalibrationTable(
    std::map<std::string, TableWorkspace_sptr> &calibwsmap,
    std::vector<std::string> orderedcomponents) {
  // Create an empty calibration table without setting to analysis data service
  mOutputWS = CorelliCalibration::CalibrationTableHandler::
      createCalibrationTableWorkspace("", false);
  CorelliCalibration::CalibrationTableHandler handler =
      CorelliCalibration::CalibrationTableHandler();
  handler.setCalibrationTable(mOutputWS);

  for (auto componentname : orderedcomponents) {
    auto componentcaltable = calibwsmap[componentname];
    if (componentcaltable) {
      // only take care of calibrated components (before and now)
      auto lastpos = CorelliCalibration::CalibrationTableHandler::
          getLatestCalibratedPosition(componentcaltable);
      handler.appendCalibration(mOutputWS, componentname, lastpos);
    }
  }

  //  // set values to the output table workspace
  //  for (auto& [componentname, componentcaltable]: calibwsmap) {
  //    if(componentcaltable) {
  //      // only take care of calibrated components (before and now)
  //    auto lastpos =
  //    CorelliCalibration::CalibrationTableHandler::getLatestCalibratedPosition(componentcaltable);
  //    handler.appendCalibration(mOutputWS, mDateStamp, lastpos);
  //    }
  //  }
}

// Create the summary CSV file
// File name exampe: corelli_instrument_20202015.csv
void CorelliPowderCalibrationDatabase::saveCalibrtionTable(
    const std::string &calibdbdir) {
  // file name
  std::string filename = "corelli_instrument_" + mDateStamp + ".csv";
  filename = joinPath(calibdbdir, filename);

  // Call the handler: set and save
  CorelliCalibration::CalibrationTableHandler handler =
      CorelliCalibration::CalibrationTableHandler();
  handler.setCalibrationTable(mOutputWS);
  handler.saveCalibrationTable(filename);
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
std::string CorelliPowderCalibrationDatabase::corelliComponentDatabaseName(
    const std::string &componentname, const std::string &directory) {
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

/**
 * @brief Set up component map
 * @param componentnames
 * @param compmap
 */
void CorelliPowderCalibrationDatabase::setComponentMap(
    std::vector<std::string> componentnames,
    std::map<std::string, DataObjects::TableWorkspace_sptr> &compmap) {
  // Add entries
  for (auto compname : componentnames)
    compmap[compname] = nullptr;
}

/**
 * @brief Retrieve the compoments including banks, source and sample names from
 * an workspace
 * @param ws : Any workspace containing instrument
 * @return  : vector including of all the compoments in the order as source,
 * sample, bank1, bank2, ...
 */
std::vector<std::string>
CorelliPowderCalibrationDatabase::retrieveInstrumentComponents(
    MatrixWorkspace_sptr ws) {
  // Get access to instrument information
  const auto &component_info = ws->componentInfo();

  // Init output
  std::vector<std::string> componentnames = {"source", "sample"};

  // Loop over all the compoments for bank
  const size_t num_components = component_info.size();
  std::vector<std::string> banknames{};
  for (size_t i = 0; i < num_components; ++i) {
    std::string compname = component_info.name(i);
    if (compname[0] == 'b') {
      componentnames.push_back(compname);
    }
  }

  return componentnames;
}

} // namespace Algorithms
} // namespace Mantid
