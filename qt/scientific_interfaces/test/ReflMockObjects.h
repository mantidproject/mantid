// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H

#include "../ISISReflectometry/GUI/RunsTable/IRunsTableView.h"
#include "../ISISReflectometry/IReflAsciiSaver.h"
#include "../ISISReflectometry/IReflBatchPresenter.h"
#include "../ISISReflectometry/IReflMainWindowPresenter.h"
#include "../ISISReflectometry/IReflMainWindowView.h"
#include "../ISISReflectometry/IReflRunsTabPresenter.h"
#include "../ISISReflectometry/IReflRunsTabView.h"
#include "../ISISReflectometry/InstrumentOptionDefaults.h"
#include "../ISISReflectometry/ReflSearchModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/Hint.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::DataProcessor;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/**** Models ****/

class MockReflSearchModel : public ReflSearchModel {
public:
  MockReflSearchModel()
      : ReflSearchModel(ITableWorkspace_sptr(), std::string()) {}
  ~MockReflSearchModel() override {}
  MOCK_CONST_METHOD2(data, QVariant(const QModelIndex &, int role));
};

/**** Views ****/

class MockRunsTabView : public IReflRunsTabView {
public:
  MockRunsTabView() {
    ON_CALL(*this, table()).WillByDefault(testing::Return(m_tableView));
  }

  // IO
  MOCK_CONST_METHOD0(getSelectedSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getAllSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getSearchString, std::string());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_CONST_METHOD0(getTransferMethod, std::string());
  MOCK_CONST_METHOD0(getAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_CONST_METHOD0(getMonitorAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_CONST_METHOD0(getSelectedGroup, int());
  MOCK_METHOD1(setTransferMethods, void(const std::set<std::string> &));
  MOCK_METHOD0(setTableCommandsProxy, void());
  MOCK_METHOD0(setRowCommandsProxy, void());
  MOCK_METHOD0(clearCommands, void());
  MOCK_METHOD2(setInstrumentList, void(const std::vector<std::string> &, int));
  MOCK_METHOD1(updateMenuEnabledState, void(bool));
  MOCK_METHOD1(setAutoreduceButtonEnabled, void(bool));
  MOCK_METHOD1(setAutoreducePauseButtonEnabled, void(bool));
  MOCK_METHOD1(setTransferButtonEnabled, void(bool));
  MOCK_METHOD1(setInstrumentComboEnabled, void(bool));
  MOCK_METHOD1(subscribe, void(IReflRunsTabPresenter *));
  MOCK_CONST_METHOD0(table, IRunsTableView *());
  MOCK_METHOD1(setSearchTextEntryEnabled, void(bool));
  MOCK_METHOD1(setSearchButtonEnabled, void(bool));
  MOCK_METHOD1(setStartMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(setStopMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(startTimer, void(const int));
  MOCK_METHOD0(stopTimer, void());
  MOCK_METHOD0(startIcatSearch, void());
  MOCK_METHOD0(startMonitor, void());
  MOCK_METHOD0(stopMonitor, void());
  MOCK_METHOD0(updateMonitorRunning, void());
  MOCK_METHOD0(updateMonitorStopped, void());

  // Calls we don't care about
  void showSearch(ReflSearchModel_sptr) override{};
  IReflRunsTabPresenter *getPresenter() const override { return nullptr; };

private:
  IRunsTableView *m_tableView;
};

class MockMainWindowView : public IReflMainWindowView {
public:
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  ~MockMainWindowView() override{};
};

/**** Presenters ****/

class MockRunsTabPresenter : public IReflRunsTabPresenter {
public:
  MOCK_CONST_METHOD0(isAutoreducing, bool());
  MOCK_METHOD0(settingsChanged, void());
  void notify(IReflRunsTabPresenter::Flag flag) override { UNUSED_ARG(flag); };
  void acceptMainPresenter(IReflBatchPresenter *presenter) override {
    UNUSED_ARG(presenter);
  }
  bool isProcessing() const override { return false; }
  ~MockRunsTabPresenter() override{};
};

class MockMainWindowPresenter : public IReflMainWindowPresenter {
public:
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  MOCK_METHOD1(settingsChanged, void(int));
  bool isProcessing() const override { return false; }

  ~MockMainWindowPresenter() override{};
};

/**** Progress ****/

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
  ~MockProgressBase() override {}
};

/**** Catalog ****/

class MockICatalogInfo : public Mantid::Kernel::ICatalogInfo {
public:
  MOCK_CONST_METHOD0(catalogName, const std::string());
  MOCK_CONST_METHOD0(soapEndPoint, const std::string());
  MOCK_CONST_METHOD0(externalDownloadURL, const std::string());
  MOCK_CONST_METHOD0(catalogPrefix, const std::string());
  MOCK_CONST_METHOD0(windowsPrefix, const std::string());
  MOCK_CONST_METHOD0(macPrefix, const std::string());
  MOCK_CONST_METHOD0(linuxPrefix, const std::string());
  MOCK_CONST_METHOD0(clone, ICatalogInfo *());
  MOCK_CONST_METHOD1(transformArchivePath, std::string(const std::string &));
  ~MockICatalogInfo() override {}
};

class MockReflAsciiSaver : public IReflAsciiSaver {
public:
  MOCK_CONST_METHOD1(isValidSaveDirectory, bool(std::string const &));
  MOCK_CONST_METHOD4(save,
                     void(std::string const &, std::vector<std::string> const &,
                          std::vector<std::string> const &,
                          FileFormatOptions const &));
  virtual ~MockReflAsciiSaver() = default;
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H*/
