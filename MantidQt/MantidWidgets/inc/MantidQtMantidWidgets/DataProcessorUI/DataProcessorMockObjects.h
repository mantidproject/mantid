#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"

#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;

// Clean column ids for use within tests (they refer to the table workspace
// only)
const int GroupCol = 0;
const int RunCol = 1;
const int ThetaCol = 2;
const int TransCol = 3;
const int QMinCol = 4;
const int QMaxCol = 5;
const int DQQCol = 6;
const int ScaleCol = 7;
const int OptionsCol = 8;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockDataProcessorView : public DataProcessorView {
public:
  MockDataProcessorView(){};
  ~MockDataProcessorView() override {}

  // Prompt
  MOCK_METHOD0(requestNotebookPath, std::string());

  // IO
  MOCK_CONST_METHOD0(getWorkspaceToOpen, std::string());
  MOCK_CONST_METHOD0(getSelectedChildren, std::map<int, std::set<int>>());
  MOCK_CONST_METHOD0(getSelectedParents, std::set<int>());
  MOCK_CONST_METHOD0(getClipboard, std::string());
  MOCK_CONST_METHOD0(getProcessInstrument, std::string());
  MOCK_METHOD0(getEnableNotebook, bool());
  MOCK_METHOD0(expandAll, void());
  MOCK_METHOD0(collapseAll, void());
  MOCK_METHOD1(setSelection, void(const std::set<int> &rows));
  MOCK_METHOD1(setClipboard, void(const std::string &text));

  MOCK_METHOD1(setModel, void(const std::string &));
  MOCK_METHOD1(setTableList, void(const std::set<std::string> &));
  MOCK_METHOD2(setInstrumentList,
               void(const std::vector<std::string> &, const std::string &));
  MOCK_METHOD2(setOptionsHintStrategy,
               void(MantidQt::MantidWidgets::HintStrategy *, int));

  // Settings
  MOCK_METHOD1(loadSettings, void(std::map<std::string, QVariant> &));

  // Actions/commands
  // Gmock requires parameters and return values of mocked methods to be
  // copyable which means we have to mock addActions() via a proxy method
  void addActions(std::vector<DataProcessorCommand_uptr>) override {
    addActionsProxy();
  }
  MOCK_METHOD0(addActionsProxy, void());

  // Calls we don't care about
  void showTable(boost::shared_ptr<QAbstractItemModel>) override{};
  void saveSettings(const std::map<std::string, QVariant> &) override{};

  DataProcessorPresenter *getPresenter() const override { return nullptr; }
};

class MockMainPresenter : public DataProcessorMainPresenter {

public:
  MockMainPresenter(){};
  ~MockMainPresenter() override {}

  // Notify
  MOCK_METHOD1(notify, void(DataProcessorMainPresenter::Flag));

  // Prompt methods
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  MOCK_CONST_METHOD0(getPreprocessingValues,
                     std::map<std::string, std::string>());
  MOCK_CONST_METHOD0(getPreprocessingProperties,
                     std::map<std::string, std::set<std::string>>());

  // Global options
  MOCK_CONST_METHOD0(getPreprocessingOptions,
                     std::map<std::string, std::string>());
  MOCK_CONST_METHOD0(getProcessingOptions, std::string());
  MOCK_CONST_METHOD0(getPostprocessingOptions, std::string());

  // Event handling
  MOCK_CONST_METHOD0(getTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getTimeSlicingType, std::string());
};

class MockDataProcessorPresenter : public DataProcessorPresenter {

public:
  MockDataProcessorPresenter(){};
  ~MockDataProcessorPresenter() override {}

  MOCK_METHOD1(notify, void(DataProcessorPresenter::Flag));
  MOCK_METHOD1(setModel, void(std::string name));
  MOCK_METHOD1(accept, void(DataProcessorMainPresenter *));
  MOCK_CONST_METHOD0(selectedParents, std::set<int>());
  MOCK_CONST_METHOD0(selectedChildren, std::map<int, std::set<int>>());
  MOCK_CONST_METHOD2(askUserYesNo,
                     bool(const std::string &prompt, const std::string &title));
  MOCK_CONST_METHOD2(giveUserWarning,
                     void(const std::string &prompt, const std::string &title));
  MOCK_METHOD0(publishCommandsMocked, void());

private:
  // Calls we don't care about
  const std::map<std::string, QVariant> &options() const override {
    return m_options;
  };

  std::vector<DataProcessorCommand_uptr> publishCommands() override {
    std::vector<DataProcessorCommand_uptr> commands;
    for (size_t i = 0; i < 29; i++)
      commands.push_back(
          Mantid::Kernel::make_unique<DataProcessorAppendRowCommand>(this));
    publishCommandsMocked();
    return commands;
  };
  std::set<std::string> getTableList() const {
    return std::set<std::string>();
  };
  // Calls we don't care about
  void setOptions(const std::map<std::string, QVariant> &) override{};
  void
  transfer(const std::vector<std::map<std::string, std::string>> &) override{};
  void setInstrumentList(const std::vector<std::string> &,
                         const std::string &) override{};
  // void accept(WorkspaceReceiver *) {};
  void acceptViews(DataProcessorView *, ProgressableView *) override{};

  std::map<std::string, QVariant> m_options;
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H*/
