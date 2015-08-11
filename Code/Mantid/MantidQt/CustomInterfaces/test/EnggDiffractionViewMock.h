#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H

#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h"

#include <gmock/gmock.h>

// This is a simple mock for the tomo interface view when using SCARF.
class MockEnggDiffractionView
    : public MantidQt::CustomInterfaces::IEnggDiffractionView {

public:
  // void userWarning(const std::string &warn, const std::string &description);
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description);
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // std::string askNewCalibrationFilename(const std::string &suggestedFname) =
  // 0;
  MOCK_METHOD1(askNewCalibrationFilename,
               std::string(const std::string &suggestedFname));

  // std::vector<std::string> logMsgs() const;
  MOCK_CONST_METHOD0(logMsgs, std::vector<std::string>());

  // virtual std::string getRBNumber() const;
  MOCK_CONST_METHOD0(getRBNumber, std::string());

  // EnggDiffCalibSettings currentCalibSettings() const;
  MOCK_CONST_METHOD0(currentCalibSettings,
                     MantidQt::CustomInterfaces::EnggDiffCalibSettings());

  // std::string currentInstrument() const;
  MOCK_CONST_METHOD0(currentInstrument, std::string());

  // virtual std::string currentVanadiumNo() const;
  MOCK_CONST_METHOD0(currentVanadiumNo, std::string());

  // virtual std::string currentCeriaNo() const;
  MOCK_CONST_METHOD0(currentCeriaNo, std::string());

  // virtual std::string newVanadiumNo() const;
  MOCK_CONST_METHOD0(newVanadiumNo, std::string());

  // virtual std::string newCeriaNo() const;
  MOCK_CONST_METHOD0(newCeriaNo, std::string());

  // virtual std::string outCalibFilename() const;
  MOCK_CONST_METHOD0(outCalibFilename, std::string());

  // void saveSettings() const;
  MOCK_CONST_METHOD0(saveSettings, void());
};

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
