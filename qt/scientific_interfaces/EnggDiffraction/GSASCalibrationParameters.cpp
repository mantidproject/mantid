#include "GSASCalibrationParameters.h"

namespace MantidQt {
namespace CustomInterfaces {

GSASCalibrationParameters::GSASCalibrationParameters(const size_t _bankID,
                                                     const double _difa,
                                                     const double _difc,
                                                     const double _tzero)
    : bankID(_bankID), difa(_difa), difc(_difc), tzero(_tzero) {}

bool operator==(const GSASCalibrationParameters &lhs,
                const GSASCalibrationParameters &rhs) {
  return lhs.bankID == rhs.bankID && lhs.difa == rhs.difa &&
         lhs.difc == rhs.difc && lhs.tzero == rhs.tzero;
}

bool operator!=(const GSASCalibrationParameters &lhs,
                const GSASCalibrationParameters &rhs) {
  return !(lhs == rhs);
}

} // MantidQt
} // CustomInterfaces
