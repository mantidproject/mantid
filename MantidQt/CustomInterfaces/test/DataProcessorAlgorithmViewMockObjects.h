#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORVIEWMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORVIEWMOCKOBJECTS_H

#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QDataProcessorTableModel.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

// Clean column ids for use within tests
const int RunCol = 0;
const int ThetaCol = 1;
const int TransCol = 2;
const int QMinCol = 3;
const int QMaxCol = 4;
const int DQQCol = 5;
const int ScaleCol = 6;
const int GroupCol = 7;
const int OptionsCol = 8;

class MockDataProcessorView : public DataProcessorView {
public:
  MockDataProcessorView(){};
  ~MockDataProcessorView() override {}

  // Prompts
  MOCK_METHOD3(askUserString,
               std::string(const std::string &prompt, const std::string &title,
                           const std::string &defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_METHOD0(requestNotebookPath, std::string());
  MOCK_METHOD0(showImportDialog, void());
  MOCK_METHOD1(showAlgorithmDialog, void(const std::string &));

  MOCK_METHOD1(plotWorkspaces, void(const std::set<std::string> &));

  // IO
  MOCK_CONST_METHOD0(getWorkspaceToOpen, std::string());
  MOCK_CONST_METHOD0(getSelectedRows, std::set<int>());
  MOCK_CONST_METHOD0(getClipboard, std::string());
  MOCK_CONST_METHOD1(getProcessingOptions, std::string(const std::string &));
  MOCK_METHOD0(getEnableNotebook, bool());
  MOCK_METHOD1(setSelection, void(const std::set<int> &rows));
  MOCK_METHOD1(setClipboard, void(const std::string &text));

  MOCK_METHOD1(setModel, void(const std::string &));
  MOCK_METHOD1(setTableList, void(const std::set<std::string> &));
  MOCK_METHOD2(setInstrumentList,
               void(const std::vector<std::string> &, const std::string &));
  MOCK_METHOD2(setOptionsHintStrategy,
               void(MantidQt::MantidWidgets::HintStrategy *, int));

  MOCK_METHOD3(addHintingLineEdit,
               void(const std::string &, const std::string &,
                    const std::map<std::string, std::string> &));

  // Settings
  MOCK_METHOD1(loadSettings, void(std::map<std::string, QVariant> &));

  // Calls we don't care about
  void showTable(QDataProcessorTableModel_sptr) override{};
  void saveSettings(const std::map<std::string, QVariant> &) override{};
  std::string getProcessInstrument() const override { return "FAKE"; }

  boost::shared_ptr<DataProcessorPresenter> getTablePresenter() const override {
    return boost::shared_ptr<DataProcessorPresenter>();
  }
};

#endif /*MANTID_CUSTOMINTERFACES_DATAPROCESSORVIEWMOCKOBJECTS_H*/
