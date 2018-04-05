#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_

#include "GSASCalibrationParameters.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffCalibrationModel {

public:
  virtual ~IEnggDiffCalibrationModel() = default;

  /**
   * Run a normal (uncropped) calibration
   * @param vanadiumRun Vanadium run name (either a path to a file or
   * <INSTNAME><RUNNUMBER>
   * @param ceriaRun Ceria run name (either a path to a file or
   * <INSTNAME><RUNNUMBER>
   * @return Vector of calibrations corresponding to the banks used
   */
  virtual std::vector<GSASCalibrationParameters>
  createCalibration(const std::string &vanadiumRun,
                    const std::string &ceriaRun) const = 0;

  /**
   * Run a calibration on a single bank
   * @param bankID The bank ID - 1 for North, 2 for South
   * @param vanadiumRun Vanadium run name (either a path to a file or
   * <INSTNAME><RUNNUMBER>
   * @param ceriaRun Ceria run name (either a path to a file or
   * <INSTNAME><RUNNUMBER>
   * @return Vector of calibrations corresponding to the banks used
   */
  virtual std::vector<GSASCalibrationParameters>
  createCalibrationByBank(const size_t bankID, const std::string &vanadiumRun,
                          const std::string &ceriaRun) const = 0;

  /**
   * Run a calibration on a set of spectra
   * @param specNums The spectra to calibrate against
   * @param bankName A custom bank name for the calibration file
   * @param vanadiumRun Vanadium run name (either a path to a file or
   * <INSTNAME><RUNNUMBER>
   * @param ceriaRun Ceria run name (either a path to a file or
   * <INSTNAME><RUNNUMBER>
   * @return Vector of calibrations corresponding to the banks used
   */
  virtual std::vector<GSASCalibrationParameters> createCalibrationBySpectra(
      const std::string &specNums, const std::string &bankName,
      const std::string &vanadiumRun, const std::string &ceriaRun) const = 0;

  /**
   * Load a calibration from file
   * @param filePath Path to the calibration file
   * @return Vector of calibrations corresponding to the banks used
   */
  virtual std::vector<GSASCalibrationParameters>
  parseCalibrationFile(const std::string &filePath) const = 0;

  virtual void setCalibrationParams(
      const std::vector<GSASCalibrationParameters> &params) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_
