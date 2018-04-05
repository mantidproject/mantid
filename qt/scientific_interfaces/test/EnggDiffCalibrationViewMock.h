#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONVIEWMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffCalibrationView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffCalibrationView : public IEnggDiffCalibrationView {
public:
  MOCK_CONST_METHOD0(getInputFilename, boost::optional<std::string>());

  MOCK_CONST_METHOD0(getNewCalibCeriaInput, std::string());

  MOCK_CONST_METHOD0(getNewCalibVanadiumInput, std::string());

  MOCK_METHOD1(setCalibFilePath, void(const std::string &filePath));

  MOCK_METHOD1(setCurrentCalibCeriaRunNumber,
               void(const std::string &runNumber));

  MOCK_METHOD1(setCurrentCalibVanadiumRunNumber,
               void(const std::string &runNumber));

  MOCK_METHOD2(userWarning, void(const std::string &warningTitle,
                                 const std::string &warningDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONVIEWMOCK_H_
