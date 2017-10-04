#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"

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
  MOCK_METHOD0(getEnableNotebook, bool());
  MOCK_METHOD0(expandAll, void());
  MOCK_METHOD0(collapseAll, void());
  MOCK_METHOD0(selectAll, void());
  MOCK_METHOD0(pause, void());
  MOCK_METHOD0(resume, void());
  MOCK_METHOD1(setSelection, void(const std::set<int> &rows));
  MOCK_METHOD1(setClipboard, void(const QString &text));

  MOCK_METHOD1(setModel, void(const QString &));
  MOCK_METHOD1(setTableList, void(const QSet<QString> &));
  MOCK_METHOD2(setInstrumentList, void(const QString &, const QString &));
  MOCK_METHOD2(setOptionsHintStrategy,
               void(MantidQt::MantidWidgets::HintStrategy *, int));

  // Settings
  MOCK_METHOD1(loadSettings, void(std::map<QString, QVariant> &));

  // Processing options
  MOCK_METHOD1(setForcedReProcessing, void(bool));
  MOCK_METHOD0(skipProcessing, void());

  // Accessor
  MOCK_CONST_METHOD0(getCurrentInstrument, QString());

  // Actions/commands
  // Gmock requires parameters and return values of mocked methods to be
  // copyable which means we have to mock addActions() via a proxy method
  void addActions(std::vector<Command_uptr>) override { addActionsProxy(); }
  MOCK_METHOD0(addActionsProxy, void());

  // Calls we don't care about
  void showTable(boost::shared_ptr<
      MantidQt::MantidWidgets::DataProcessor::AbstractTreeModel>) override{};
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
  MOCK_CONST_METHOD0(pause, void());
  MOCK_CONST_METHOD0(resume, void());

  // Calls we don't care about
  void confirmReductionPaused() const override{};
  void confirmReductionResumed() const override{};
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
  MOCK_METHOD1(setForcedReProcessing, void(bool));

private:
  // Calls we don't care about
  const std::map<QString, QVariant> &options() const override {
    return m_options;
  };

  std::vector<Command_uptr> publishCommands() override {
    std::vector<Command_uptr> commands;
    for (size_t i = 0; i < 31; i++)
      commands.push_back(Mantid::Kernel::make_unique<AppendRowCommand>(this));
    publishCommandsMocked();
    return commands;
  };
  std::set<QString> getTableList() const { return std::set<QString>(); };
  // Calls we don't care about
  void setOptions(const std::map<QString, QVariant> &) override {}
  void transfer(const std::vector<std::map<QString, QString>> &) override {}
  void setInstrumentList(const QStringList &, const QString &) override {}
  // void accept(WorkspaceReceiver *) {};
  void acceptViews(DataProcessorView *, ProgressableView *) override{};

  void setCell(int, int, int, int, const std::string &) override{};
  std::string getCell(int, int, int, int) override { return ""; };
  int getNumberOfRows() override { return 2; }

  void clearTable() override {}

  std::map<QString, QVariant> m_options;
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_MANTIDWIDGETS_DATAPROCESSORVIEWMOCKOBJECTS_H*/
