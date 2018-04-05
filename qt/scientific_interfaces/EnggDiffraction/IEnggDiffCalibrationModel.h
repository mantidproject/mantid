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
