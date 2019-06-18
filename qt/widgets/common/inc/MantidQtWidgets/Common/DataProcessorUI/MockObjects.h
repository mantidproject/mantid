// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"

#include "MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QTwoLevelTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeManager.h"

#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;

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
const int HiddenOptionsCol = 9;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockDataProcessorView : public DataProcessorView {
public:
  MockDataProcessorView() {}
  ~MockDataProcessorView() override {}

  // Prompt
  MOCK_METHOD0(requestNotebookPath, QString());
  MOCK_METHOD3(askUserString,
               QString(const QString &, const QString &, const QString &));
  MOCK_METHOD2(askUserYesNo, bool(QString, QString));
  MOCK_METHOD2(giveUserWarning, void(QString, QString));
  MOCK_METHOD2(giveUserCritical, void(QString, QString));
  MOCK_METHOD1(runPythonAlgorithm, QString(const QString &));

  // IO
  MOCK_CONST_METHOD0(getWorkspaceToOpen, QString());
  MOCK_CONST_METHOD0(getSelectedChildren, std::map<int, std::set<int>>());
  MOCK_CONST_METHOD0(getSelectedParents, std::set<int>());
  MOCK_CONST_METHOD0(getClipboard, QString());
  MOCK_CONST_METHOD0(getProcessInstrument, QString());
  MOCK_METHOD0(getEnableNotebook, bool());
  MOCK_METHOD0(expandAll, void());
  MOCK_METHOD0(collapseAll, void());
  MOCK_METHOD0(selectAll, void());
  MOCK_METHOD1(updateMenuEnabledState, void(const bool));
  MOCK_METHOD1(setProcessButtonEnabled, void(const bool));
  MOCK_METHOD1(setInstrumentComboEnabled, void(const bool));
  MOCK_METHOD1(setTreeEnabled, void(const bool));
  MOCK_METHOD1(setOutputNotebookEnabled, void(const bool));
  MOCK_METHOD1(setSelection, void(const std::set<int> &rows));
  MOCK_METHOD1(setClipboard, void(const QString &text));

  MOCK_METHOD1(setModel, void(const QString &));
  MOCK_METHOD2(setInstrumentList, void(const QString &, const QString &));
  MOCK_METHOD2(setOptionsHintStrategy,
               void(MantidQt::MantidWidgets::HintStrategy *, int));
  MOCK_METHOD0(setItemDelegate, void());

  // Settings
  MOCK_METHOD1(loadSettings, void(std::map<QString, QVariant> &));
  MOCK_METHOD0(settingsChanged, void());

  // Processing options
  MOCK_METHOD1(setForcedReProcessing, void(bool));
  MOCK_METHOD0(skipProcessing, void());

  // Grouping options
  MOCK_METHOD0(enableGrouping, void());
  MOCK_METHOD0(disableGrouping, void());

  // Accessor
  MOCK_CONST_METHOD0(getCurrentInstrument, QString());

  // Actions/commands
  // Gmock requires parameters and return values of mocked methods to be
  // copyable which means we have to mock addActions() via a proxy method
  void addActions(std::vector<Command_uptr>) override { addActionsProxy(); }
  MOCK_METHOD0(addActionsProxy, void());

  // Calls we don't care about
  void showTable(boost::shared_ptr<
                 MantidQt::MantidWidgets::DataProcessor::AbstractTreeModel>)
      override{};
  void saveSettings(const std::map<QString, QVariant> &) override{};

  void emitProcessClicked() override{};
  void emitProcessingFinished() override{};

  DataProcessorPresenter *getPresenter() const override { return nullptr; }
};

class MockMainPresenter : public DataProcessorMainPresenter {

public:
  MockMainPresenter() {}
  ~MockMainPresenter() override {}

  // Notify
  MOCK_METHOD1(notifyADSChanged, void(const QSet<QString> &));

  // Prompt methods
  MOCK_METHOD3(askUserString,
               QString(const QString &, const QString &, const QString &));
  MOCK_METHOD2(askUserYesNo, bool(QString, QString));
  MOCK_METHOD2(giveUserWarning, void(QString, QString));
  MOCK_METHOD2(giveUserCritical, void(QString, QString));
  MOCK_METHOD1(runPythonAlgorithm, QString(const QString &));

  // Global options
  MOCK_CONST_METHOD0(getPreprocessingOptions, ColumnOptionsQMap());
  MOCK_CONST_METHOD0(getProcessingOptions, OptionsQMap());
  MOCK_CONST_METHOD0(getPostprocessingOptionsAsString, QString());
  MOCK_CONST_METHOD0(getTimeSlicingOptions, QString());

  // Event handling
  MOCK_CONST_METHOD0(getTimeSlicingValues, QString());
  MOCK_CONST_METHOD0(getTimeSlicingType, QString());

  // Data reduction paused/resumed handling
  MOCK_METHOD0(pause, void());
  MOCK_CONST_METHOD0(resume, void());

  // Calls we don't care about
  MOCK_METHOD0(confirmReductionCompleted, void());
  MOCK_METHOD0(confirmReductionPaused, void());
  MOCK_METHOD0(confirmReductionResumed, void());
};

class MockDataProcessorPresenter : public DataProcessorPresenter {

public:
  MockDataProcessorPresenter(){};
  ~MockDataProcessorPresenter() override {}

  MOCK_METHOD1(notify, void(DataProcessorPresenter::Flag));
  MOCK_METHOD1(setModel, void(QString const &name));
  MOCK_METHOD1(accept, void(DataProcessorMainPresenter *));
  MOCK_CONST_METHOD0(selectedParents, std::set<int>());
  MOCK_CONST_METHOD0(selectedChildren, std::map<int, std::set<int>>());
  MOCK_CONST_METHOD0(isProcessing, bool());
  MOCK_CONST_METHOD2(askUserYesNo,
                     bool(const QString &prompt, const QString &title));
  MOCK_CONST_METHOD2(giveUserWarning,
                     void(const QString &prompt, const QString &title));
  MOCK_METHOD0(publishCommandsMocked, void());
  MOCK_METHOD0(skipProcessing, void());
  MOCK_METHOD1(setPromptUser, void(const bool));
  MOCK_METHOD1(setForcedReProcessing, void(bool));
  MOCK_METHOD0(settingsChanged, void());
  MOCK_METHOD1(transfer, void(const std::vector<std::map<QString, QString>> &));

private:
  // Calls we don't care about
  const std::map<QString, QVariant> &options() const override {
    return m_options;
  };

  std::vector<Command_uptr> publishCommands() override {
    std::vector<Command_uptr> commands;
    for (size_t i = 0; i < 31; i++)
      commands.push_back(std::make_unique<AppendRowCommand>(this));
    publishCommandsMocked();
    return commands;
  };
  std::set<QString> getTableList() const { return std::set<QString>(); };
  // Calls we don't care about
  void setOptions(const std::map<QString, QVariant> &) override {}
  void setInstrumentList(const QStringList &, const QString &) override {}
  // void accept(WorkspaceReceiver *) {};
  void acceptViews(DataProcessorView *, ProgressableView *) override{};

  void setCell(int, int, int, int, const std::string &) override{};
  std::string getCell(int, int, int, int) override { return ""; };
  int getNumberOfRows() override { return 2; }

  void clearTable() override {}

  std::map<QString, QVariant> m_options;
};

class MockTreeManager : public TreeManager {
public:
  MockTreeManager(){};
  ~MockTreeManager() override{};
  MOCK_METHOD1(selectedData, TreeData(bool));
  MOCK_METHOD1(allData, TreeData(bool));
  // Calls we don't care about
  std::vector<std::unique_ptr<Command>> publishCommands() override {
    return std::vector<std::unique_ptr<Command>>();
  };
  bool isMultiLevel() const override { return false; }
  void appendRow() override{};
  void appendGroup() override{};
  void deleteRow() override{};
  void deleteGroup() override{};
  void deleteAll() override{};
  void groupRows() override{};
  std::set<int> expandSelection() override { return std::set<int>(); };
  void clearSelected() override{};
  QString copySelected() override { return QString(); };
  void pasteSelected(const QString &) override{};
  void newTable(const WhiteList &) override{};
  void newTable(Mantid::API::ITableWorkspace_sptr,
                const WhiteList &) override{};
  void transfer(const std::vector<std::map<QString, QString>> &) override{};
  void update(int, int, const QStringList &) override{};
  int rowCount() const override { return 0; };
  int rowCount(int) const override { return 0; };
  bool isProcessed(int) const override { return false; };
  bool isProcessed(int, int) const override { return false; };
  void setProcessed(bool, int) override{};
  void setProcessed(bool, int, int) override{};
  bool reductionFailed(int) const override { return false; };
  bool reductionFailed(int, int) const override { return false; };
  void setError(const std::string &, int) override{};
  void setError(const std::string &, int, int) override{};
  void invalidateAllProcessed() override{};
  void setCell(int, int, int, int, const std::string &) override{};
  std::string getCell(int, int, int, int) const override {
    return std::string();
  };
  int getNumberOfRows() override { return 0; };
  bool isValidModel(Mantid::API::Workspace_sptr, size_t) const override {
    return false;
  };
  boost::shared_ptr<AbstractTreeModel> getModel() override {
    return boost::shared_ptr<QTwoLevelTreeModel>();
  };
  Mantid::API::ITableWorkspace_sptr getTableWorkspace() override {
    return Mantid::API::ITableWorkspace_sptr();
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H*/
