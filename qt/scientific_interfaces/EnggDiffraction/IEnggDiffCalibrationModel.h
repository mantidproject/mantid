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

  virtual std::vector<GSASCalibrationParameters>
  createCalibration(const std::string &vanadiumRunNumber,
                    const std::string &ceriaRunNumber) const = 0;

  virtual std::vector<GSASCalibrationParameters>
  parseCalibrationFile(const std::string &filePath) const = 0;

  virtual void setCalibrationParams(
      const std::vector<GSASCalibrationParameters> &params) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONMODEL_H_
