#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H

#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h"

#include <gmock/gmock.h>

// This is a simple mock for the tomo interface view when using SCARF.
class MockEnggDiffractionView
    : public MantidQt::CustomInterfaces::IEnggDiffractionView {

public:
  // virtual void userWarning(const std::string &warn, const std::string
  // &description);
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // virtual void userError(const std::string &err, const std::string
  // &description);
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // virtual std::string askNewCalibrationFilename(const std::string
  // &suggestedFname);
  MOCK_METHOD1(askNewCalibrationFilename,
               std::string(const std::string &suggestedFname));

  // std::string askExistingCalibFilename();
  MOCK_METHOD0(askExistingCalibFilename, std::string());

  // std::vector<std::string> logMsgs() const;
  MOCK_CONST_METHOD0(logMsgs, std::vector<std::string>());

  // virtual std::string getRBNumber() const;
  MOCK_CONST_METHOD0(getRBNumber, std::string());

  // virtual EnggDiffCalibSettings currentCalibSettings() const;
  MOCK_CONST_METHOD0(currentCalibSettings,
                     MantidQt::CustomInterfaces::EnggDiffCalibSettings());

  // std::string currentInstrument() const;
  MOCK_CONST_METHOD0(currentInstrument, std::string());

  // virtual std::string currentVanadiumNo() const;
  MOCK_CONST_METHOD0(currentVanadiumNo, std::string());

  // virtual std::string currentCeriaNo() const;
  MOCK_CONST_METHOD0(currentCeriaNo, std::string());

  // virtual std::string currentCalibFile() const;
  MOCK_CONST_METHOD0(currentCalibFile, std::string());

  // virtual std::string newVanadiumNo() const;
  MOCK_CONST_METHOD0(newVanadiumNo, std::string());

  // virtual std::string newCeriaNo() const;
  MOCK_CONST_METHOD0(newCeriaNo, std::string());

  // virtual std::string outCalibFilename() const;
  MOCK_CONST_METHOD0(outCalibFilename, std::string());

  // virtual void newCalibLoaded(const std::string &vanadiumNo, const
  // std::string &ceriaNo, std::string &fname);
  MOCK_METHOD3(newCalibLoaded, void(const std::string &, const std::string &,
                                    const std::string &));

  // virtual void writeOutCalibFile(const std::string &outFilename,
  //                                const std::vector<double> &difc,
  //                                const std::vector<double> &tzero)
  MOCK_METHOD3(writeOutCalibFile,
               void(const std::string &, const std::vector<double> &,
                    const std::vector<double> &));

  // virtual void enableCalibrateAndFocusActions(bool enable);
  MOCK_METHOD1(enableCalibrateAndFocusActions, void(bool));

  // virtual std::string focusingDir() const;
  MOCK_CONST_METHOD0(focusingDir, std::string());

  // virtual std::string focusingRunNo() const;
  MOCK_CONST_METHOD0(focusingRunNo, std::string());

  // virtual int focusingBank() const;
  MOCK_CONST_METHOD0(focusingBank, int());

  // void saveSettings() const;
  MOCK_CONST_METHOD0(saveSettings, void());
};

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
