// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H

#include "../EnggDiffraction/IEnggDiffractionView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

class QwtData;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffractionView
    : public MantidQt::CustomInterfaces::IEnggDiffractionView {

public:
  // virtual void splashMessage(bool visible, const std::string &shortMsg,
  // const std::string &description);
  MOCK_METHOD3(splashMessage, void(bool visible, const std::string &shortMsg,
                                   const std::string &description));

  // virtual void showStatus(const std::string &sts);
  MOCK_METHOD1(showStatus, void(const std::string &sts));

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

  // virtual std::vector<GSASCalibrationParms> currentCalibration() const
  MOCK_CONST_METHOD0(
      currentCalibration,
      std::vector<MantidQt::CustomInterfaces::GSASCalibrationParms>());

  // virtual std::string enggRunPythonCode(const std::string &pyCode)
  MOCK_METHOD1(enggRunPythonCode, std::string(const std::string &));

  // virtual void enableTabs(bool enable);
  MOCK_METHOD1(enableTabs, void(bool));

  // virtual void showInvalidRBNumber(const bool rbNumberIsValid);
  MOCK_METHOD1(showInvalidRBNumber, void(const bool));

  // virtual void enableCalibrateFocusFitUserActions(bool enable);
  MOCK_METHOD1(enableCalibrateFocusFitUserActions, void(bool));

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

  // virtual bool focusedOutWorkspace() const;
  MOCK_CONST_METHOD0(focusedOutWorkspace, bool());

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
  MOCK_METHOD3(setDataVector,
               void(std::vector<boost::shared_ptr<QwtData>> &data, bool focused,
                    bool plotSinglePeaks));

  // virtual void plotCalibOutput();
  MOCK_METHOD1(plotCalibOutput, void(const std::string &pyCode));

  // virtual void updateTabsInstrument(const std::string & newInstrument) = 0;
  MOCK_METHOD1(updateTabsInstrument, void(const std::string &newInstrument));
};
GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONVIEWMOCK_H
