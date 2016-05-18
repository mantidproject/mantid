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

  // int currentCropCalibBankName
  MOCK_CONST_METHOD0(currentCropCalibBankName, int());

  // std::string currentCalibSpecNos
  MOCK_CONST_METHOD0(currentCalibSpecNos, std::string());

  // std::string currentCalibCustomisedBankName
  MOCK_CONST_METHOD0(currentCalibCustomisedBankName, std::string());

  // int currentPlotType
  MOCK_CONST_METHOD0(currentPlotType, int());

  // int currentMultiRunMode
  MOCK_CONST_METHOD0(currentMultiRunMode, int());

  // virtual std::vector<std::string> newVanadiumNo() const;
  MOCK_CONST_METHOD0(newVanadiumNo, std::vector<std::string>());

  // virtual std::vector<std::string> newCeriaNo() const;
  MOCK_CONST_METHOD0(newCeriaNo, std::vector<std::string>());

  // virtual std::string outCalibFilename() const;
  MOCK_CONST_METHOD0(outCalibFilename, std::string());

  // virtual void newCalibLoaded(const std::string &vanadiumNo, const
  // std::string &ceriaNo, std::string &fname);
  MOCK_METHOD3(newCalibLoaded, void(const std::string &, const std::string &,
                                    const std::string &));

  // virtual std::string enggRunPythonCode(const std::string &pyCode)
  MOCK_METHOD1(enggRunPythonCode, std::string(const std::string &));

  // virtual void enableTabs(bool enable);
  MOCK_METHOD1(enableTabs, void(bool));

  // virtual void enableCalibrateAndFocusActions(bool enable);
  MOCK_METHOD1(enableCalibrateAndFocusActions, void(bool));

  // virtual std::string focusingDir() const;
  MOCK_CONST_METHOD0(focusingDir, std::string());

  // virtual std::vector<std::string> focusingRunNo() const;
  MOCK_CONST_METHOD0(focusingRunNo, std::vector<std::string>());

  // virtual std::vector<std::string> focusingCroppedRunNo() const;
  MOCK_CONST_METHOD0(focusingCroppedRunNo, std::vector<std::string>());

  // virtual std::vector<std::string> focusingTextureRunNo() const;
  MOCK_CONST_METHOD0(focusingTextureRunNo, std::vector<std::string>());

  // virtual int focusingBank() const;
  MOCK_CONST_METHOD0(focusingBanks, std::vector<bool>());

  // virtual std::string focusingCroppedSpectrumNos() const;
  MOCK_CONST_METHOD0(focusingCroppedSpectrumNos, std::string());

  // virtual std::string focusingTextureGroupingFile() const;
  MOCK_CONST_METHOD0(focusingTextureGroupingFile, std::string());

  // virtual void resetFocus();
  MOCK_METHOD0(resetFocus, void());

  // virtual std::vector<std::string> currentPreprocRunNo() const;
  MOCK_CONST_METHOD0(currentPreprocRunNo, std::vector<std::string>());

  // virtual double rebinningTimeBin() const;
  MOCK_CONST_METHOD0(rebinningTimeBin, double());

  // virtual size_t rebinningNumberPeriods() const;
  MOCK_CONST_METHOD0(rebinningPulsesNumberPeriods, size_t());

  // virtual double rebinningPulsesPerPeriod() const;
  MOCK_CONST_METHOD0(rebinningPulsesTime, double());

  // virtual std::string fittingRunNo() const;
  MOCK_CONST_METHOD0(getFittingRunNo, std::string());

  // virtual std::string fittingPeaksData() const;
  MOCK_CONST_METHOD0(fittingPeaksData, std::string());

  // virtual bool focusedOutWorkspace() const;
  MOCK_CONST_METHOD0(focusedOutWorkspace, bool());

  // virtual Splits the fitting directory if the ENGINX found
  MOCK_METHOD1(splitFittingDirectory,
               std::vector<std::string>(std::string &selectedfPath));

  // adds the number of banks to the combo-box widget on the interface
  MOCK_METHOD2(addBankItems, void(std::vector<std::string> splittedBaseName,
                                  QString selectedFile));

  // adds the run number to the list view widget on the interface
  MOCK_METHOD2(addRunNoItem,
               void(std::vector<std::string> runNumVector, bool multiRun));

  // checks if the text-inputted is a valid run
  MOCK_METHOD1(isDigit, bool(std::string text));

  // emits the signal within view when run number / bank changed
  MOCK_METHOD0(setBankEmit, void());

  // gets the set focus directory within the setting tab
  MOCK_METHOD0(getFocusDir, std::string());

  // gets the global vector in view containing focused file directory
  MOCK_METHOD0(getFittingRunNumVec, std::vector<std::string>());

  // sets the global vector in view containing focused file directory
  MOCK_METHOD1(setFittingRunNumVec, void(std::vector<std::string> assignVec));

  // sets the fitting run number according to path
  MOCK_METHOD1(setFittingRunNo, void(QString path));

  // To determine whether the current loop is multi-run or single to avoid
  // regenerating the list - view widget when not required
  MOCK_METHOD0(getFittingMultiRunMode, bool());

  // sets the fitting mode to multi-run or single to avoid
  // regenerating the list - view widget when not required
  MOCK_METHOD1(setFittingMultiRunMode, void(bool mode));

  // virtual bool plotCalibWorkspace
  MOCK_CONST_METHOD0(plotCalibWorkspace, bool());

  // void saveSettings() const;
  MOCK_CONST_METHOD0(saveSettings, void());

  // virtual bool saveFocusedOutputFiles
  MOCK_CONST_METHOD0(saveFocusedOutputFiles, bool());

  // void plotFocusStatus();
  MOCK_METHOD0(plotFocusStatus, void());

  // void plotRepChanged();
  MOCK_METHOD1(plotRepChanged, void(int idx));

  // virtual void plotFocusedSpectrum();
  MOCK_METHOD1(plotFocusedSpectrum, void(const std::string &));

  // virtual void plotWaterfallSpectrum
  MOCK_METHOD1(plotWaterfallSpectrum, void(const std::string &wsName));

  // virtual void plotReplacingWindow
  MOCK_METHOD3(plotReplacingWindow,
               void(const std::string &wsName, const std::string &spectrum,
                    const std::string &type));

  // virtual void setDataVector
  MOCK_METHOD2(setDataVector,
               void(std::vector<boost::shared_ptr<QwtData>> &data,
                    bool focused));

  // virtual void plotCalibOutput();
  MOCK_METHOD1(plotCalibOutput, void(const std::string &pyCode));
};

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
