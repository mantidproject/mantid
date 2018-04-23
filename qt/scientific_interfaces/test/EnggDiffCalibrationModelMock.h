#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONMODELMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffCalibrationModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffCalibrationModel : public IEnggDiffCalibrationModel {
public:
  MOCK_CONST_METHOD2(createCalibration, std::vector<GSASCalibrationParameters>(
                                            const std::string &vanadiumRun,
                                            const std::string &ceriaRun));

  MOCK_CONST_METHOD3(createCalibrationByBank,
                     std::vector<GSASCalibrationParameters>(
                         const size_t bankID, const std::string &vanadiumRun,
                         const std::string &ceriaRun));

  MOCK_CONST_METHOD4(
      createCalibrationBySpectra,
      std::vector<GSASCalibrationParameters>(const std::string &specNums,
                                             const std::string &bankName,
                                             const std::string &vanadiumRun,
                                             const std::string &ceriaRun));

  MOCK_CONST_METHOD1(
      parseCalibrationFile,
      std::vector<GSASCalibrationParameters>(const std::string &filePath));

  MOCK_METHOD1(setCalibrationParams,
               void(const std::vector<GSASCalibrationParameters> &params));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONMODELMOCK_H_
