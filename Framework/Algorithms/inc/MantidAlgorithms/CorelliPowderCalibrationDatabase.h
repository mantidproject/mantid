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
    "ComponentName",    "Xposition",        "Yposition",
    "Zposition",        "XdirectionCosine", "YdirectionCosine",
    "ZdirectionCosine", "RotationAngle"};
static const std::vector<std::string> calibrationTableColumnTypes{
    "str",    "double", "double", "double",
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
    if (fabs(x - otherpos.x) >= abstol || fabs(y - otherpos.y) >= abstol ||
        fabs(z - otherpos.z) >= abstol ||
        fabs(xCosine - otherpos.xCosine) >= abstol || fabs(yCosine - otherpos.yCosine) >= abstol ||
        fabs(zCosine - otherpos.zCosine) >= abstol || fabs(rotAngle - otherpos.rotAngle) >= abstol) {
      return false;
    }
    return true;
  }
};

/// Class containing static and member methods to work with calibration table workspaces
class MANTID_ALGORITHMS_DLL CalibrationTableHandler {
public:
  CalibrationTableHandler();
  void setCalibrationTable(DataObjects::TableWorkspace_sptr calibws);
  std::vector<std::string> getComponentNames();
  DataObjects::TableWorkspace_sptr getCalibrationWorkspace() {
      return mCalibWS;
  }
  void load(const std::string &filename);
  void saveCompomentDatabase(const std::string &datestamp, const std::string &component, const std::string &filename);
  /// Save the calibration table (of a single date)
  void saveCalibrationTable(const std::string &filename);  
  /// Load a single component calibration table
  static DataObjects::TableWorkspace_sptr loadComponentCalibrationTable(const std::string &filename);
  /// Create a single component calibration TableWorkspace from scratch
  static DataObjects::TableWorkspace_sptr createComponentCalibrationTable(const std::string &wsname);
  /// Get the calibration of a component
  ComponentPosition getComponentCalibratedPosition(const std::string &component);
  /// Append a new row to single component calibration table
  static void appendCalibration(DataObjects::TableWorkspace_sptr tablews, const std::string &datestamp, ComponentPosition& pos);

private:
  DataObjects::TableWorkspace_sptr mCalibWS;
  bool isSingleComponentTable;
};

} // namespace CorelliCalibration

/** CorelliPowderCalibrationDatabase: blablabla TODO
 */
class MANTID_ALGORITHMS_DLL CorelliPowderCalibrationDatabase
    : public API::Algorithm {
public:
  const std::string name() const override {
    return "CorelliPowderCalibrationDatabase";
  };
  int version() const override { return 1; };
  const std::string category() const override {
    return "Diffraction\\Calibration;Events";
  };
  const std::string summary() const override {
    return ".... ...."; // TODO
  };

  /// get standard component calibration database (CSV) file name
  static inline std::string
  corelliComponentDatabaseName(const std::string &componentname,
                               const std::string &directory);
  /// get standard date-base calibration database (CSV) file name
  static inline std::string
  corelliCalibrationDatabaseName(const std::string &datestamp,
                                 const std::string &directory);

  static std::string convertTimeStamp(std::string run_start_time);

  /// Check whether a given file does exist
  static inline bool isFileExist(const std::string &filepath);

  /// Join two string for a new path
  static inline std::string joinPath(const std::string directory,
                                     const std::string basename);

private:
  std::map<std::string, std::string> validateInputs() override;
  void init() override;
  void exec() override;

  /// append the newly calibration to each component csv file
  void updateComponentDatabaseFiles();

  /// Input workspace
  API::MatrixWorkspace_const_sptr inputWS;
  DataObjects::TableWorkspace_const_sptr inputCalibrationTableWS;
  DataObjects::TableWorkspace_sptr outputWS;
};

} // namespace Algorithms
} // namespace Mantid
