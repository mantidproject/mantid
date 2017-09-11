#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CommandProviderFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCommandProvider.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"

#include <gmock/gmock.h>
#include <cassert>

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
const int HiddenOptionsCol = 9;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

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
  MOCK_METHOD0(isNotebookEnabled, bool());
  MOCK_METHOD0(expandAll, void());
  MOCK_METHOD0(collapseAll, void());
  MOCK_METHOD0(selectAll, void());
  MOCK_METHOD1(setSelection, void(const std::set<int> &rows));
  MOCK_METHOD1(setClipboard, void(const QString &text));
  MOCK_METHOD0(disableProcessButton, void());
  MOCK_METHOD0(enableProcessButton, void());
  MOCK_METHOD1(enableAction, void(int));
  MOCK_METHOD1(disableAction, void(int));
  MOCK_METHOD0(disableSelectionAndEditing, void());
  MOCK_METHOD0(enableSelectionAndEditing, void());

  MOCK_METHOD1(setModel, void(const QString &));
  MOCK_METHOD1(setTableList, void(const QSet<QString> &));
  MOCK_METHOD2(setInstrumentList, void(const QString &, const QString &));
  MOCK_METHOD2(setOptionsHintStrategy,
               void(MantidQt::MantidWidgets::HintStrategy *, int));

  // Settings
  MOCK_METHOD1(loadSettings, void(std::map<QString, QVariant> &));

  // Processing options
  MOCK_METHOD1(setForcedReProcessing, void(bool));

  // Accessor
  MOCK_CONST_METHOD0(getCurrentInstrument, QString());

  // Actions/commands
  // Gmock requires parameters and return values of mocked methods to be
  // copyable which means we have to mock addEditActions() via a proxy method
  void addEditActions(const std::vector<DataProcessorCommand_uptr> &) override {
    addEditActionsProxy();
  }
  MOCK_METHOD0(addEditActionsProxy, void());

  // Calls we don't care about
  void showTable(
      boost::shared_ptr<
          MantidQt::MantidWidgets::AbstractDataProcessorTreeModel>) override{};
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
  MOCK_CONST_METHOD0(getPreprocessingProperties, QString());

  // Global options
  MOCK_CONST_METHOD0(getPreprocessingOptionsAsString, QString());
  MOCK_CONST_METHOD0(getProcessingOptions, QString());
  MOCK_CONST_METHOD0(getPostprocessingOptions, QString());
  MOCK_CONST_METHOD0(getTimeSlicingOptions, QString());

  // Event handling
  MOCK_CONST_METHOD0(getTimeSlicingValues, QString());
  MOCK_CONST_METHOD0(getTimeSlicingType, QString());

  // Data reduction paused/resumed handling
  MOCK_METHOD0(pause, void());
  MOCK_METHOD0(resume, void());
  MOCK_METHOD0(confirmReductionPaused, void());
  MOCK_METHOD0(confirmReductionResumed, void());
};

class MockDataProcessorPresenter : public DataProcessorPresenter {

public:
  MockDataProcessorPresenter() : m_commands(){};
  ~MockDataProcessorPresenter() override = default;

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
  MOCK_METHOD0(getTableCommandsMocked, void());
  MOCK_METHOD1(indexOfCommand, int(TableAction));
  MOCK_METHOD1(indexOfCommand, int(EditAction));
  MOCK_METHOD0(getEditCommandsMocked, void());
  MOCK_METHOD1(setForcedReProcessing, void(bool));

private:
  // Calls we don't care about
  const std::map<QString, QVariant> &options() const override {
    return m_options;
  };

  std::vector<DataProcessorCommand_uptr> &getEditCommands() override {
    getEditCommandsMocked();
    return m_commands;
  };

  std::vector<DataProcessorCommand_uptr> &getTableCommands() override {
    getTableCommandsMocked();
    return m_commands;
  };

  std::set<QString> getTableList() const { return std::set<QString>(); };
  // Calls we don't care about
  void setOptions(const std::map<QString, QVariant> &) override {}
  void transfer(const std::vector<std::map<QString, QString>> &) override {}
  void setInstrumentList(const QStringList &, const QString &) override {}
  // void accept(WorkspaceReceiver *) {};
  void acceptViews(DataProcessorView *, ProgressableView *) override {}
  std::vector<DataProcessorCommand_uptr> m_commands;

  void setCell(int, int, int, int, const std::string &) override{};
  std::string getCell(int, int, int, int) override { return ""; };
  int getNumberOfRows() override { return 2; }

  void clearTable() override {}

  std::map<QString, QVariant> m_options;
};

class MockDataProcessorCommandProvider : public DataProcessorCommandProvider {
public:
  using CommandVector = typename DataProcessorCommandProvider::CommandVector;
  using CommandIndex = typename DataProcessorCommandProvider::CommandIndex;
  using CommandIndices = typename DataProcessorCommandProvider::CommandIndices;
  MOCK_CONST_METHOD0(getTableCommands, const CommandVector &());
  MOCK_CONST_METHOD1(indexOfCommand, CommandIndex(TableAction));

  MOCK_CONST_METHOD0(getEditCommands, const CommandVector &());
  MOCK_CONST_METHOD0(getModifyingTableCommands, CommandIndices());
  MOCK_CONST_METHOD0(getModifyingEditCommands, CommandIndices());
  MOCK_CONST_METHOD0(getPausingEditCommands, CommandIndices());
  MOCK_CONST_METHOD0(getProcessingEditCommands, CommandIndices());
  MOCK_CONST_METHOD1(indexOfCommand, CommandIndex(EditAction));
};

class MockDataProcessorCommandProviderFactory : public CommandProviderFactory {
public:
  MockDataProcessorCommandProviderFactory(
      std::unique_ptr<DataProcessorCommandProvider> mockProvider)
      : m_mockProvider(std::move(mockProvider)) {
        ON_CALL(*this, fromPostprocessorName(::testing::_, ::testing::_))
          .WillByDefault(::testing::Invoke([&m_mockProvider]
                (const QString&, GenericDataProcessorPresenter&) 
                  -> std::unique_ptr<DataProcessorCommandProvider> {
                  assert(m_mockProvider != nullptr);
                  return std::move(m_mockProvider);
                });
      }
  MOCK_CONST_METHOD2(fromPostprocessorName,
                     std::unique_ptr<DataProcessorCommandProvider>(
                         const QString &, GenericDataProcessorPresenter &));

private:
  std::unique_ptr<DataProcessorCommandProvider> m_mockProvider;
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H*/
