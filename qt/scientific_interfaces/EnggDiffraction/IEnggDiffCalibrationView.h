#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONVIEW_H_

#include "GSASCalibrationParameters.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffCalibrationView {
public:
  /// Get crop type for cropped calibration - either North, South, or using
  /// spectrum numbers
  virtual CalibCropType getCalibCropType() const = 0;

  /// Get the custom bank name for cropped calib with spectrum numbers
  virtual std::string getCustomBankName() const = 0;

  /// Get the name of the current calibration file
  virtual boost::optional<std::string> getInputFilename() const = 0;

  virtual std::string getNewCalibCeriaInput() const = 0;

  virtual std::string getNewCalibVanadiumInput() const = 0;

  virtual std::string getSpectrumNumbers() const = 0;

  virtual void setCalibFilePath(const std::string &filePath) = 0;

  virtual void setCurrentCalibCeriaRunNumber(const std::string &runNumber) = 0;

  virtual void
  setCurrentCalibVanadiumRunNumber(const std::string &runNumber) = 0;

  virtual void userWarning(const std::string &warningTitle,
                           const std::string &warningDescription) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONVIEW_H_
