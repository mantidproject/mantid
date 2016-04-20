#ifndef MANTID_CUSTOMINTERFACES_REFLTABLEVIEWMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLTABLEVIEWMOCKOBJECTS_H

#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableView.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

// Clean column ids for use within tests
const int RunCol = ReflTableSchema::COL_RUNS;
const int ThetaCol = ReflTableSchema::COL_ANGLE;
const int TransCol = ReflTableSchema::COL_TRANSMISSION;
const int QMinCol = ReflTableSchema::COL_QMIN;
const int QMaxCol = ReflTableSchema::COL_QMAX;
const int DQQCol = ReflTableSchema::COL_DQQ;
const int ScaleCol = ReflTableSchema::COL_SCALE;
const int GroupCol = ReflTableSchema::COL_GROUP;
const int OptionsCol = ReflTableSchema::COL_OPTIONS;

class MockTableView : public ReflTableView {
public:
  MockTableView(){};
  ~MockTableView() override {}

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
  MOCK_METHOD0(getEnableNotebook, bool());
  MOCK_METHOD1(setSelection, void(const std::set<int> &rows));
  MOCK_METHOD1(setClipboard, void(const std::string &text));
  MOCK_METHOD1(setOptionsHintStrategy,
               void(MantidQt::MantidWidgets::HintStrategy *));
  MOCK_METHOD1(setModel, void(const std::string&));
  MOCK_METHOD1(setTableList, void(const std::set<std::string> &));
  MOCK_METHOD2(setInstrumentList,
               void(const std::vector<std::string> &, const std::string &));

  // Calls we don't care about
  void showTable(QReflTableModel_sptr) override{};
  void saveSettings(const std::map<std::string, QVariant> &) override{};
  void loadSettings(std::map<std::string, QVariant> &) override{};
  std::string getProcessInstrument() const override { return "FAKE"; }

  boost::shared_ptr<IReflTablePresenter> getTablePresenter() const override {
    return boost::shared_ptr<IReflTablePresenter>();
  }
};

#endif /*MANTID_CUSTOMINTERFACES_REFLTABLEVIEWMOCKOBJECTS_H*/
