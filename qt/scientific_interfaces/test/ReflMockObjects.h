// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H

#include "../ISISReflectometry/ExperimentOptionDefaults.h"
#include "../ISISReflectometry/IReflAsciiSaver.h"
#include "../ISISReflectometry/IReflEventPresenter.h"
#include "../ISISReflectometry/IReflEventTabPresenter.h"
#include "../ISISReflectometry/IReflEventView.h"
#include "../ISISReflectometry/IReflMainWindowPresenter.h"
#include "../ISISReflectometry/IReflMainWindowView.h"
#include "../ISISReflectometry/IReflRunsTabPresenter.h"
#include "../ISISReflectometry/IReflRunsTabView.h"
#include "../ISISReflectometry/IReflSaveTabPresenter.h"
#include "../ISISReflectometry/IReflSaveTabView.h"
#include "../ISISReflectometry/IReflSettingsPresenter.h"
#include "../ISISReflectometry/IReflSettingsTabPresenter.h"
#include "../ISISReflectometry/IReflSettingsView.h"
#include "../ISISReflectometry/InstrumentOptionDefaults.h"
#include "../ISISReflectometry/ReflLegacyTransferStrategy.h"
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
      : ReflSearchModel(ReflLegacyTransferStrategy(), ITableWorkspace_sptr(),
                        std::string()) {}
  ~MockReflSearchModel() override {}
  MOCK_CONST_METHOD2(data, QVariant(const QModelIndex &, int role));
};

/**** Views ****/

class MockRunsTabView : public IReflRunsTabView {
public:
  // Gmock requires parameters and return values of mocked methods to be
  // copyable
  // We can't mock setTableCommands(std::vector<Command_uptr>)
  // because
  // of the vector of unique pointers
  // I will mock a proxy method, setTableCommandsProxy, I just want to test that
  // this method is invoked by the presenter's constructor
  virtual void setTableCommands(
      std::vector<MantidQt::MantidWidgets::DataProcessor::Command_uptr>)
      override {
    setTableCommandsProxy();
  }
  // The same happens for setRowCommands
  virtual void setRowCommands(
      std::vector<MantidQt::MantidWidgets::DataProcessor::Command_uptr>)
      override {
    setRowCommandsProxy();
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
  MOCK_METHOD2(setInstrumentList,
               void(const std::vector<std::string> &, const std::string &));
  MOCK_METHOD1(updateMenuEnabledState, void(bool));
  MOCK_METHOD1(setAutoreduceButtonEnabled, void(bool));
  MOCK_METHOD1(setAutoreducePauseButtonEnabled, void(bool));
  MOCK_METHOD1(setTransferButtonEnabled, void(bool));
  MOCK_METHOD1(setInstrumentComboEnabled, void(bool));
  MOCK_METHOD1(setTransferMethodComboEnabled, void(bool));
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
};

class MockSettingsView : public IReflSettingsView {
public:
  // Global options
  MOCK_CONST_METHOD0(getTransmissionOptions, std::string());
  MOCK_CONST_METHOD0(getStartOverlap, std::string());
  MOCK_CONST_METHOD0(getEndOverlap, std::string());
  MOCK_CONST_METHOD0(getReductionOptions, std::string());
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  MOCK_CONST_METHOD0(getAnalysisMode, std::string());
  MOCK_CONST_METHOD0(getDirectBeam, std::string());
  MOCK_CONST_METHOD0(getPolarisationCorrections, std::string());
  MOCK_CONST_METHOD0(getCRho, std::string());
  MOCK_CONST_METHOD0(getCAlpha, std::string());
  MOCK_CONST_METHOD0(getCAp, std::string());
  MOCK_CONST_METHOD0(getCPp, std::string());
  MOCK_CONST_METHOD0(getFloodCorrection, std::string());
  MOCK_CONST_METHOD0(getFloodWorkspace, std::string());
  MOCK_CONST_METHOD0(getIntMonCheck, std::string());
  MOCK_CONST_METHOD0(getMonitorIntegralMin, std::string());
  MOCK_CONST_METHOD0(getMonitorIntegralMax, std::string());
  MOCK_CONST_METHOD0(getMonitorBackgroundMin, std::string());
  MOCK_CONST_METHOD0(getMonitorBackgroundMax, std::string());
  MOCK_CONST_METHOD0(getLambdaMin, std::string());
  MOCK_CONST_METHOD0(getLambdaMax, std::string());
  MOCK_CONST_METHOD0(getI0MonitorIndex, std::string());
  MOCK_CONST_METHOD0(getSummationType, std::string());
  MOCK_CONST_METHOD0(getReductionType, std::string());
  MOCK_CONST_METHOD0(getDebugOption, bool());
  MOCK_CONST_METHOD0(getIncludePartialBins, bool());
  MOCK_CONST_METHOD0(getPerAngleOptions, std::map<std::string, OptionsQMap>());
  MOCK_CONST_METHOD1(setIsPolCorrEnabled, void(bool));
  MOCK_METHOD1(setReductionTypeEnabled, void(bool));
  MOCK_METHOD1(setIncludePartialBinsEnabled, void(bool));
  MOCK_METHOD1(setPolarisationOptionsEnabled, void(bool));
  MOCK_METHOD1(setDetectorCorrectionEnabled, void(bool));
  MOCK_METHOD1(setExpDefaults, void(ExperimentOptionDefaults));
  MOCK_METHOD1(setInstDefaults, void(InstrumentOptionDefaults));
  MOCK_METHOD0(disableAll, void());
  MOCK_METHOD0(enableAll, void());
  MOCK_CONST_METHOD0(getDetectorCorrectionType, std::string());
  MOCK_CONST_METHOD0(experimentSettingsEnabled, bool());
  MOCK_CONST_METHOD0(instrumentSettingsEnabled, bool());
  MOCK_METHOD2(showOptionLoadErrors,
               void(std::vector<InstrumentParameterTypeMissmatch> const &,
                    std::vector<MissingInstrumentParameterValue> const &));
  MOCK_CONST_METHOD0(detectorCorrectionEnabled, bool());
  // Calls we don't care about
  void createStitchHints(
      const std::vector<MantidQt::MantidWidgets::Hint> &hints) override {
    UNUSED_ARG(hints);
  };
  IReflSettingsPresenter *getPresenter() const override { return nullptr; }
};

class MockEventView : public IReflEventView {
public:
  // Global options
  MOCK_METHOD1(enableSliceType, void(SliceType));
  MOCK_METHOD1(disableSliceType, void(SliceType));
  MOCK_METHOD0(enableSliceTypeSelection, void());
  MOCK_METHOD0(disableSliceTypeSelection, void());
  MOCK_CONST_METHOD0(getLogValueTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getCustomTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getUniformTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getUniformEvenTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getLogValueTimeSlicingType, std::string());

  // Calls we don't care about
  IReflEventPresenter *getPresenter() const override { return nullptr; }
};

class MockSaveTabView : public IReflSaveTabView {
public:
  MOCK_CONST_METHOD1(setSavePath, void(const std::string &path));
  MOCK_CONST_METHOD0(getSavePath, std::string());
  MOCK_CONST_METHOD0(getPrefix, std::string());
  MOCK_CONST_METHOD0(getFilter, std::string());
  MOCK_CONST_METHOD0(getRegexCheck, bool());
  MOCK_CONST_METHOD0(getCurrentWorkspaceName, std::string());
  MOCK_CONST_METHOD0(getSelectedWorkspaces, std::vector<std::string>());
  MOCK_CONST_METHOD0(getSelectedParameters, std::vector<std::string>());
  MOCK_CONST_METHOD0(getFileFormatIndex, int());
  MOCK_CONST_METHOD0(getTitleCheck, bool());
  MOCK_CONST_METHOD0(getQResolutionCheck, bool());
  MOCK_CONST_METHOD0(getSeparator, std::string());
  MOCK_CONST_METHOD0(clearWorkspaceList, void());
  MOCK_CONST_METHOD1(setWorkspaceList, void(const std::vector<std::string> &));
  MOCK_CONST_METHOD0(clearParametersList, void());
  MOCK_CONST_METHOD1(setParametersList, void(const std::vector<std::string> &));
  MOCK_CONST_METHOD0(getAutosavePrefixInput, std::string());
  MOCK_METHOD1(subscribe, void(IReflSaveTabPresenter *));
  MOCK_METHOD0(disallowAutosave, void());
  MOCK_METHOD0(disableAutosaveControls, void());
  MOCK_METHOD0(enableAutosaveControls, void());
  MOCK_METHOD0(enableFileFormatAndLocationControls, void());
  MOCK_METHOD0(disableFileFormatAndLocationControls, void());
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  virtual ~MockSaveTabView() = default;
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
  MOCK_CONST_METHOD1(isAutoreducing, bool(int));
  MOCK_METHOD1(settingsChanged, void(int));
  void notify(IReflRunsTabPresenter::Flag flag) override { UNUSED_ARG(flag); };
  void acceptMainPresenter(IReflMainWindowPresenter *presenter) override {
    UNUSED_ARG(presenter);
  }
  bool isProcessing(int) const override { return false; }
  bool isProcessing() const override { return false; }
  ~MockRunsTabPresenter() override{};
};

class MockEventPresenter : public IReflEventPresenter {
public:
  MOCK_CONST_METHOD0(getTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getTimeSlicingType, std::string());
  MOCK_METHOD1(acceptTabPresenter, void(IReflEventTabPresenter *));
  MOCK_METHOD0(onReductionPaused, void());
  MOCK_METHOD0(onReductionResumed, void());
  MOCK_METHOD1(notifySliceTypeChanged, void(SliceType));
  MOCK_METHOD0(notifySettingsChanged, void());
  ~MockEventPresenter() override{};
};

class MockEventTabPresenter : public IReflEventTabPresenter {
public:
  std::string getTimeSlicingValues(int) const override { return std::string(); }
  std::string getTimeSlicingType(int) const override { return std::string(); }
  MOCK_METHOD1(acceptMainPresenter, void(IReflMainWindowPresenter *));
  MOCK_METHOD1(settingsChanged, void(int));
  MOCK_METHOD1(onReductionPaused, void(int));
  MOCK_METHOD1(onReductionResumed, void(int));

  ~MockEventTabPresenter() override{};
};

class MockSettingsPresenter : public IReflSettingsPresenter {
public:
  MOCK_CONST_METHOD1(getOptionsForAngle, OptionsQMap(const double));
  MOCK_CONST_METHOD0(hasPerAngleOptions, bool());
  MOCK_CONST_METHOD0(getTransmissionOptions, OptionsQMap());
  MOCK_CONST_METHOD0(getReductionOptions, OptionsQMap());
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  MOCK_METHOD0(onReductionPaused, void());
  MOCK_METHOD0(onReductionResumed, void());
  MOCK_METHOD1(acceptTabPresenter, void(IReflSettingsTabPresenter *));
  MOCK_METHOD1(setInstrumentName, void(const std::string &));
  void notify(IReflSettingsPresenter::Flag flag) override { UNUSED_ARG(flag); }
  IAlgorithm_sptr createReductionAlg() override {
    return AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
  }
  ~MockSettingsPresenter() override{};
};

class MockSettingsTabPresenter : public IReflSettingsTabPresenter {
public:
  MOCK_CONST_METHOD2(getOptionsForAngle, OptionsQMap(int, const double));
  MOCK_CONST_METHOD1(hasPerAngleOptions, bool(int));
  MOCK_CONST_METHOD0(getTransmissionOptions, OptionsQMap());
  MOCK_CONST_METHOD1(getTransmissionOptions, OptionsQMap(int));
  MOCK_CONST_METHOD1(getReductionOptions, OptionsQMap(int));
  MOCK_CONST_METHOD1(getStitchOptions, std::string(int));
  MOCK_METHOD1(acceptMainPresenter, void(IReflMainWindowPresenter *));
  MOCK_METHOD1(settingsChanged, void(int));
  void setInstrumentName(const std::string &instName) override {
    UNUSED_ARG(instName);
  };
  MOCK_METHOD1(onReductionPaused, void(int));
  MOCK_METHOD1(onReductionResumed, void(int));
  ~MockSettingsTabPresenter() override{};
};

class MockSaveTabPresenter : public IReflSaveTabPresenter {
public:
  MOCK_METHOD2(completedRowReductionSuccessfully,
               void(MantidQt::MantidWidgets::DataProcessor::GroupData const &,
                    std::string const &));
  MOCK_METHOD2(completedGroupReductionSuccessfully,
               void(MantidQt::MantidWidgets::DataProcessor::GroupData const &,
                    std::string const &));
  void notify(IReflSaveTabPresenter::Flag flag) override { UNUSED_ARG(flag); };
  void acceptMainPresenter(IReflMainWindowPresenter *presenter) override {
    UNUSED_ARG(presenter);
  };

  MOCK_METHOD0(onAnyReductionPaused, void());
  MOCK_METHOD0(onAnyReductionResumed, void());
  ~MockSaveTabPresenter() override{};
};

class MockMainWindowPresenter : public IReflMainWindowPresenter {
public:
  MOCK_CONST_METHOD2(getOptionsForAngle, OptionsQMap(int, const double));
  MOCK_CONST_METHOD1(hasPerAngleOptions, bool(int));
  MOCK_CONST_METHOD1(getTransmissionOptions, OptionsQMap(int));
  MOCK_CONST_METHOD1(getReductionOptions, OptionsQMap(int));
  MOCK_CONST_METHOD1(getStitchOptions, std::string(int));
  MOCK_CONST_METHOD1(setInstrumentName, void(const std::string &instName));
  MOCK_CONST_METHOD0(getInstrumentName, std::string());
  MOCK_METHOD2(completedRowReductionSuccessfully,
               void(MantidQt::MantidWidgets::DataProcessor::GroupData const &,
                    std::string const &));
  MOCK_METHOD2(completedGroupReductionSuccessfully,
               void(MantidQt::MantidWidgets::DataProcessor::GroupData const &,
                    std::string const &));
  MOCK_METHOD1(notify, void(IReflMainWindowPresenter::Flag));
  MOCK_METHOD1(notifyReductionPaused, void(int));
  MOCK_METHOD1(notifyReductionResumed, void(int));
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  MOCK_METHOD1(settingsChanged, void(int));
  // Other calls we don't care about
  std::string getTimeSlicingValues(int group) const override {
    UNUSED_ARG(group);
    return std::string();
  }
  std::string getTimeSlicingType(int group) const override {
    UNUSED_ARG(group);
    return std::string();
  }
  bool isProcessing() const override { return false; }
  bool isProcessing(int) const override { return false; }

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
