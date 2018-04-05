#include "GSASCalibrationParameters.h"

namespace MantidQt {
namespace CustomInterfaces {

GSASCalibrationParameters::GSASCalibrationParameters(
    const size_t _bankID, const double _difa, const double _difc,
    const double _tzero, const std::string &_vanadiumRunNumber,
    const std::string &_ceriaRunNumber, const std::string &_filePath)
    : bankID(_bankID), difa(_difa), difc(_difc), tzero(_tzero),
      vanadiumRunNumber(_vanadiumRunNumber), ceriaRunNumber(_ceriaRunNumber),
      filePath(_filePath) {}

bool operator==(const GSASCalibrationParameters &lhs,
                const GSASCalibrationParameters &rhs) {
  return lhs.bankID == rhs.bankID && lhs.difa == rhs.difa &&
         lhs.difc == rhs.difc && lhs.tzero == rhs.tzero &&
         lhs.vanadiumRunNumber == rhs.vanadiumRunNumber &&
         lhs.ceriaRunNumber == rhs.ceriaRunNumber &&
         lhs.filePath == rhs.filePath;
}

bool operator!=(const GSASCalibrationParameters &lhs,
                const GSASCalibrationParameters &rhs) {
  return !(lhs == rhs);
}

} // MantidQt
} // CustomInterfaces
