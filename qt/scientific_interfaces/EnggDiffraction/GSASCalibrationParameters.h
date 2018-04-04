#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASCALIBPARAMS_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASCALIBPARAMS_H_

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ENGGDIFFRACTION_DLL GSASCalibrationParameters {
  GSASCalibrationParameters(const size_t _bankID, const double _difa,
                            const double _difc, const double _tzero);

  const size_t bankID;
  const double difa;
  const double difc;
  const double tzero;
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
