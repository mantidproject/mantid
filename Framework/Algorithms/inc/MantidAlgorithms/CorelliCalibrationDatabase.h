// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {

namespace CorelliCalibration {
static const std::vector<std::string> calibrationTableColumnNames{
    "ComponentName",    "Xposition",        "Yposition",        "Zposition",
    "XdirectionCosine", "YdirectionCosine", "ZdirectionCosine", "RotationAngle"};
static const std::vector<std::string> calibrationTableColumnTypes{"str",    "double", "double", "double",
                                                                  "double", "double", "double", "double"};

/// Structure to handle all the calibration component positions
struct ComponentPosition {
  double x;
  double y;
  double z;
  double xCosine;
  double yCosine;
  double zCosine;
  double rotAngle;

  bool equalTo(const ComponentPosition &otherpos, double abstol) {
    if (fabs(x - otherpos.x) >= abstol || fabs(y - otherpos.y) >= abstol || fabs(z - otherpos.z) >= abstol ||
        fabs(xCosine - otherpos.xCosine) >= abstol || fabs(yCosine - otherpos.yCosine) >= abstol ||
        fabs(zCosine - otherpos.zCosine) >= abstol || fabs(rotAngle - otherpos.rotAngle) >= abstol) {
      return false;
    }
    return true;
  }
};

/// Class containing static and member methods to work with calibration table
/// workspaces
class MANTID_ALGORITHMS_DLL CalibrationTableHandler {
public:
  /// Load a single-component calibration table
  static DataObjects::TableWorkspace_sptr loadComponentCalibrationTable(const std::string &filename,
                                                                        const std::string &tablewsname);

  /// Check whether a TableWorkspace is a valid Corelli geometry calibration
  /// table for all components
  static bool isValidCalibrationTableWorkspace(DataObjects::TableWorkspace_sptr calibws, std::string &errormsg);

  /// Create a calibration TableWorkspace from scratch for either single
  /// component or full set of components
  static DataObjects::TableWorkspace_sptr createCalibrationTableWorkspace(const std::string &wsname, bool iscomponent);

  /// Get the calibration of a component
  ComponentPosition getComponentCalibratedPosition(const std::string &component);

  /// Append a new row to single component calibration table
  static void appendCalibration(DataObjects::TableWorkspace_sptr tablews, const std::string &datestamp,
                                ComponentPosition &pos);

  /// Get the last entry (latest update) of a compoent calibrated position
  static ComponentPosition getLatestCalibratedPosition(DataObjects::TableWorkspace_sptr componentcaltable);

  /// Get the calibration position in the table (component table or full
  /// calibration table)
  static ComponentPosition getCalibratedPosition(DataObjects::TableWorkspace_sptr componentcaltable, size_t rownumber);

  /// Constructor
  CalibrationTableHandler();

  /// Set calibration table file
  void setCalibrationTable(DataObjects::TableWorkspace_sptr calibws);

  /// Get component name from the table
  std::vector<std::string> getComponentNames();
  /// Get calibration workspace
  DataObjects::TableWorkspace_sptr getCalibrationWorkspace() { return mCalibWS; }

  /// Save a single component in the calibration workspace
  DataObjects::TableWorkspace_sptr saveCompomentDatabase(const std::string &datestamp, const std::string &component,
                                                         const std::string &filename);
  /// Save the calibration table (of a single date)
  void saveCalibrationTable(const std::string &filename);

private:
  DataObjects::TableWorkspace_sptr mCalibWS;
  bool isSingleComponentTable;
};

} // namespace CorelliCalibration

/** CorelliCalibrationDatabase: blablabla TODO
 */
class MANTID_ALGORITHMS_DLL CorelliCalibrationDatabase : public API::Algorithm {
public:
  const std::string name() const override { return "CorelliCalibrationDatabase"; };
  int version() const override { return 1; };
  const std::string category() const override { return "Diffraction\\Calibration"; };

  /// Extra help info
  const std::vector<std::string> seeAlso() const override {
    return {"CorelliPowderCalibrationCreate", "CorelliPowderCalibrationLoad", "CorelliCalibrationApply"};
  };

  const std::string summary() const override {
    return "Save calibrated components' positions and orientations to "
           "database.";
  };

  /// get standard component calibration database (CSV) file name
  static inline std::string corelliComponentDatabaseName(const std::string &componentname,
                                                         const std::string &directory);
  /// get standard date-base calibration database (CSV) file name
  static inline std::string corelliCalibrationDatabaseName(const std::string &datestamp, const std::string &directory);

  static std::string convertTimeStamp(std::string run_start_time);

  /// Check whether a given file does exist
  static inline bool isFileExist(const std::string &filepath);

  /// Join two string for a new path
  static inline std::string joinPath(const std::string directory, const std::string basename);

  /// Retrieve the bank level components names in order
  static std::vector<std::string> retrieveInstrumentComponents(API::MatrixWorkspace_sptr ws);

private:
  std::map<std::string, std::string> validateInputs() override;
  void init() override;
  void exec() override;

  /// append the newly calibration to each component csv file
  void updateComponentDatabaseFiles(const std::string &calibdbdir,
                                    std::map<std::string, DataObjects::TableWorkspace_sptr> &calibwsmap);
  /// Load data file if necessary and possible: component_caibws_map
  void loadNonCalibratedComponentDatabase(const std::string &calibddir,
                                          std::map<std::string, DataObjects::TableWorkspace_sptr> &calibwsmap);
  /// Create output full set calibration workspace
  void createOutputCalibrationTable(std::map<std::string, DataObjects::TableWorkspace_sptr> &calibwsmap,
                                    std::vector<std::string> orderedcomponents);
  // Create the summary CSV file
  void saveCalibrationTable(const std::string &calibdbdir);
  /// Set up a component name - TableWorkspace map for single component
  /// calibration
  void setComponentMap(std::vector<std::string> componentnames,
                       std::map<std::string, DataObjects::TableWorkspace_sptr> &compmap);

  /// Input workspace where the calibration is from
  API::MatrixWorkspace_sptr mInputWS;
  /// Input calibration worksapce
  DataObjects::TableWorkspace_sptr mInputCalibrationTableWS;
  /// Output calibration worksapce (merged with previous calibrated data)
  DataObjects::TableWorkspace_sptr mOutputWS;
  /// Date stamp: YYYYMMDD
  std::string mDateStamp;
};

} // namespace Algorithms
} // namespace Mantid
