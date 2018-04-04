#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_

#include "GSASCalibrationParameters.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffCalibrationModel {

public:
  virtual ~IEnggDiffCalibrationModel() = default;

  virtual GSASCalibrationParameters
  parseCalibrationFile(const std::string &filePath) const = 0;

  virtual void
  setCalibrationParams(const GSASCalibrationParameters &params) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_
