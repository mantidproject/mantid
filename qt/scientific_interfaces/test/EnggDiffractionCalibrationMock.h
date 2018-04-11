#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRACTIONCALIBRATIONMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRACTIONCALIBRATIONMOCK_

#include "../EnggDiffraction/IEnggDiffractionCalibration.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffractionCalibration : public IEnggDiffractionCalibration {

public:
  MOCK_CONST_METHOD0(currentCalibration, std::vector<GSASCalibrationParms>());

};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRACTIONCALIBRATIONMOCK_H_
