// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGVIEWMOCK_H

#include "../EnggDiffraction/IEnggDiffFittingView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffFittingView
    : public MantidQt::CustomInterfaces::IEnggDiffFittingView {

public:
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

  // virtual void enableCalibrateFocusFitUserActions(bool enable);
  MOCK_METHOD1(enableCalibrateFocusFitUserActions, void(bool));

  // virtual EnggDiffCalibSettings currentCalibSettings() const;
  MOCK_CONST_METHOD0(currentCalibSettings,
                     MantidQt::CustomInterfaces::EnggDiffCalibSettings());

  // virtual std::string enggRunPythonCode(const std::string &pyCode)
  MOCK_METHOD1(enggRunPythonCode, std::string(const std::string &));

  // virtual std::string fittingRunNo() const;
  MOCK_CONST_METHOD0(getFocusedFileNames, std::string());

  // virtual std::string getExpectedPeaksInput() const;
  MOCK_CONST_METHOD0(getExpectedPeaksInput, std::string());

  // virtual bool focusedOutWorkspace() const;
  MOCK_CONST_METHOD0(focusedOutWorkspace, bool());

  // adds the number of banks to the combo-box widget on the interface
  MOCK_METHOD1(addBankItem, void(std::string bankID));

  // adds the run number to the list view widget on the interface
  MOCK_METHOD1(addRunNoItem, void(std::string runNo));

  // emits the signal within view when run number / bank changed
  MOCK_METHOD0(setBankEmit, void());

  // sets the bank combo-box according to given index
  MOCK_METHOD1(setBankIdComboBox, void(int idx));

  // deletes all items from the fitting combo-box widget
  MOCK_CONST_METHOD0(clearFittingComboBox, void());

  // enables or disables the fitting combo-box
  MOCK_CONST_METHOD1(enableFittingComboBox, void(bool enable));

  // gets the index of the bank according to text found
  MOCK_CONST_METHOD1(getFittingComboIdx, int(std::string bank));

  // deletes all items from the fitting list widget
  MOCK_CONST_METHOD0(clearFittingListWidget, void());

  // enables or disables the fitting list widget
  MOCK_CONST_METHOD1(enableFittingListWidget, void(bool enable));

  // gets the previously used directory path by the user
  MOCK_CONST_METHOD0(getPreviousDir, std::string());

  // sets the previously used directory path
  MOCK_METHOD1(setPreviousDir, void(const std::string &path));

  // gets the path as string which required when browsing the file
  MOCK_METHOD1(getOpenFile, std::string(const std::string &prevPath));

  // gets the path as string which is required when saving the file
  MOCK_METHOD1(getSaveFile, std::string(const std::string &prevPath));

  // Gets the peak picker's center (d-spacing value)
  MOCK_CONST_METHOD0(getPeakCentre, double());

  // Checks whether peak picker widget is enabled or no
  MOCK_CONST_METHOD0(peakPickerEnabled, bool());

  // return idx of current selected row of list widget
  MOCK_CONST_METHOD0(getFittingListWidgetCurrentRow, int());

  // gets whether the list widget has a selected row
  MOCK_CONST_METHOD0(listWidgetHasSelectedRow, bool());

  // sets the current row of the fitting list widget
  MOCK_CONST_METHOD1(setFittingListWidgetCurrentRow, void(int idx));

  // gets current value of the fitting list widget
  MOCK_CONST_METHOD0(getFittingListWidgetCurrentValue,
                     boost::optional<std::string>());

  // sets the peak list according to the QString given
  MOCK_CONST_METHOD1(setPeakList, void(const std::string &peakList));

  // std::vector<std::string> logMsgs() const;
  MOCK_CONST_METHOD0(logMsgs, std::vector<std::string>());

  // gets the global vector in view containing focused file directory
  MOCK_METHOD0(getFittingRunNumVec, std::vector<std::string>());

  // sets the global vector in view containing focused file directory
  MOCK_METHOD1(setFittingRunNumVec, void(std::vector<std::string> assignVec));

  // sets the fitting run number according to path
  MOCK_METHOD1(setFocusedFileNames, void(const std::string &path));

  // To determine whether the current loop is multi-run or single to avoid
  // regenerating the list - view widget when not required
  MOCK_METHOD0(getFittingMultiRunMode, bool());

  // sets the fitting mode to multi-run to avoid regenerating
  // the list and widgets - view widget when not required
  MOCK_METHOD1(setFittingMultiRunMode, void(bool mode));

  // To determine whether the current loop is single-run in order
  // to avoid regenerating the list and widgets
  MOCK_METHOD0(getFittingSingleRunMode, bool());

  // sets the fitting mode to single-run to avoid regenerating
  // the list and widgets - view widget when not required
  MOCK_METHOD1(setFittingSingleRunMode, void(bool mode));

  // enable or disable the Fit All button
  MOCK_CONST_METHOD1(enableFitAllButton, void(bool enable));

  // void saveSettings() const;
  MOCK_CONST_METHOD0(saveSettings, void());

  // virtual void setDataVector
  MOCK_METHOD4(setDataVector,
               void(std::vector<boost::shared_ptr<QwtData>> &data, bool focused,
                    bool plotSinglePeaks, const std::string &xAxisLabel));

  // virtual void resetCanvas
  MOCK_METHOD0(resetCanvas, void());

  // virtual std::string getCurrentInstrument() const = 0;
  MOCK_CONST_METHOD0(getCurrentInstrument, std::string());

  // virtual void setCurrentInstrument(const std::string &newInstrument) = 0;
  MOCK_METHOD1(setCurrentInstrument, void(const std::string &newInstrument));

  MOCK_CONST_METHOD0(plotFittedPeaksEnabled, bool());

  // virtual void updateFittingListWidget(const std::vector<std::string> &rows)
  // = 0;
  MOCK_METHOD1(updateFittingListWidget,
               void(const std::vector<std::string> &rows));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGVIEWMOCK_H
