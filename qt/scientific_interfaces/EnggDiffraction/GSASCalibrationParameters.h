#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASCALIBPARAMS_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASCALIBPARAMS_H_

#include "DllConfig.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

enum class CalibCropType { NORTH_BANK, SOUTH_BANK, SPEC_NUMS };

struct MANTIDQT_ENGGDIFFRACTION_DLL GSASCalibrationParameters {
  GSASCalibrationParameters(const size_t _bankID, const double _difa,
                            const double _difc, const double _tzero,
                            const std::string &_vanadiumRunNumber,
                            const std::string &_ceriaRunNumber,
                            const std::string &_filePath);

  const size_t bankID;
  const double difa;
  const double difc;
  const double tzero;

  const std::string vanadiumRunNumber;
  const std::string ceriaRunNumber;
  const std::string filePath;
};

MANTIDQT_ENGGDIFFRACTION_DLL bool
operator==(const GSASCalibrationParameters &lhs,
           const GSASCalibrationParameters &rhs);

MANTIDQT_ENGGDIFFRACTION_DLL bool
operator!=(const GSASCalibrationParameters &lhs,
           const GSASCalibrationParameters &rhs);

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASCALIBPARAMS_H_
