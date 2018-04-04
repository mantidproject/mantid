#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONVIEWMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffCalibrationView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffCalibrationView : public IEnggDiffCalibrationView {
public:
  MOCK_METHOD1(displayLoadedCeriaRunNumber, void(const std::string &runNumber));

  MOCK_METHOD1(displayLoadedVanadiumRunNumber,
               void(const std::string &runNumber));

  MOCK_CONST_METHOD0(getInputFilename, boost::optional<std::string>());

  MOCK_CONST_METHOD0(getNewCalibCeriaRunNumber, boost::optional<std::string>());

  MOCK_CONST_METHOD0(getNewCalibVanadiumRunNumber,
                     boost::optional<std::string>());

  MOCK_METHOD2(userWarning, void(const std::string &warningTitle,
                                 const std::string &warningDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONVIEWMOCK_H_
