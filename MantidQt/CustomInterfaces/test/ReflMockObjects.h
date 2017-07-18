#ifndef MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H

#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

/**** Views ****/

class MockRunsTabView : public IReflRunsTabView {
public:
  // Gmock requires parameters and return values of mocked methods to be
  // copyable
  // We can't mock setTableCommands(std::vector<DataProcessorCommand_uptr>)
  // because
  // of the vector of unique pointers
  // I will mock a proxy method, setTableCommandsProxy, I just want to test that
  // this method is invoked by the presenter's constructor
  virtual void setTableCommands(
      std::vector<MantidQt::MantidWidgets::DataProcessorCommand_uptr>)
      override {
    setTableCommandsProxy();
  }
  // The same happens for setRowCommands
  virtual void setRowCommands(
      std::vector<MantidQt::MantidWidgets::DataProcessorCommand_uptr>)
      override {
    setRowCommandsProxy();
  }

  // IO
  MOCK_CONST_METHOD0(getSelectedSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getSearchString, std::string());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_CONST_METHOD0(getTransferMethod, std::string());
  MOCK_CONST_METHOD0(getAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_CONST_METHOD0(getSelectedGroup, int());
  MOCK_METHOD1(setTransferMethods, void(const std::set<std::string> &));
  MOCK_METHOD0(setTableCommandsProxy, void());
  MOCK_METHOD0(setRowCommandsProxy, void());
  MOCK_METHOD0(clearCommands, void());
  MOCK_METHOD2(setInstrumentList,
               void(const std::vector<std::string> &, const std::string &));
  MOCK_METHOD2(setRowActionEnabled, void(int, bool));

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
  MOCK_CONST_METHOD0(getMomentumTransferStep, std::string());
  MOCK_CONST_METHOD0(getScaleFactor, std::string());
  MOCK_CONST_METHOD0(getIntMonCheck, std::string());
  MOCK_CONST_METHOD0(getMonitorIntegralMin, std::string());
  MOCK_CONST_METHOD0(getMonitorIntegralMax, std::string());
  MOCK_CONST_METHOD0(getMonitorBackgroundMin, std::string());
  MOCK_CONST_METHOD0(getMonitorBackgroundMax, std::string());
  MOCK_CONST_METHOD0(getLambdaMin, std::string());
  MOCK_CONST_METHOD0(getLambdaMax, std::string());
  MOCK_CONST_METHOD0(getI0MonitorIndex, std::string());
  MOCK_CONST_METHOD0(getProcessingInstructions, std::string());
  MOCK_CONST_METHOD0(getTransmissionRuns, std::string());
  MOCK_CONST_METHOD1(setIsPolCorrEnabled, void(bool));
  MOCK_CONST_METHOD1(setPolarisationOptionsEnabled, void(bool));
  MOCK_CONST_METHOD1(setExpDefaults, void(const std::vector<std::string> &));
  MOCK_CONST_METHOD2(setInstDefaults, void(const std::vector<double> &,
                                           const std::vector<std::string> &));
  MOCK_CONST_METHOD0(getDetectorCorrectionType, std::string());
  MOCK_CONST_METHOD0(experimentSettingsEnabled, bool());
  MOCK_CONST_METHOD0(instrumentSettingsEnabled, bool());
  // Calls we don't care about
  void
  createStitchHints(const std::map<std::string, std::string> &hints) override {
    UNUSED_ARG(hints);
  };
  IReflSettingsPresenter *getPresenter() const override { return nullptr; }
};

class MockEventView : public IReflEventView {
public:
  // Global options
  MOCK_CONST_METHOD0(getTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getTimeSlicingType, std::string());

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

  // Calls we don't care about
  IReflSaveTabPresenter *getPresenter() const override { return nullptr; }
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
  void notify(IReflRunsTabPresenter::Flag flag) override { UNUSED_ARG(flag); };
  void acceptMainPresenter(IReflMainWindowPresenter *presenter) override {
    UNUSED_ARG(presenter);
  };
  ~MockRunsTabPresenter() override{};
};

class MockEventPresenter : public IReflEventPresenter {
public:
  MOCK_CONST_METHOD0(getTimeSlicingValues, std::string());
  MOCK_CONST_METHOD0(getTimeSlicingType, std::string());
  ~MockEventPresenter() override{};
};

class MockEventTabPresenter : public IReflEventTabPresenter {
public:
  std::string getTimeSlicingValues(int group) const override {
    UNUSED_ARG(group)
    return std::string();
  };
  std::string getTimeSlicingType(int group) const override {
    UNUSED_ARG(group)
    return std::string();
  };
  ~MockEventTabPresenter() override{};
};

class MockSettingsPresenter : public IReflSettingsPresenter {
public:
  MOCK_CONST_METHOD1(getTransmissionRuns, std::string(bool));
  MOCK_CONST_METHOD0(getTransmissionOptions, std::string());
  MOCK_CONST_METHOD0(getReductionOptions, std::string());
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  MOCK_METHOD1(setInstrumentName, void(const std::string &));
  void notify(IReflSettingsPresenter::Flag flag) override { UNUSED_ARG(flag); }
  ~MockSettingsPresenter() override{};
};

class MockSettingsTabPresenter : public IReflSettingsTabPresenter {
public:
  MOCK_CONST_METHOD2(getTransmissionRuns, std::string(int, bool));
  MOCK_CONST_METHOD1(getTransmissionOptions, std::string(int));
  MOCK_CONST_METHOD1(getReductionOptions, std::string(int));
  MOCK_CONST_METHOD1(getStitchOptions, std::string(int));
  void setInstrumentName(const std::string &instName) override {
    UNUSED_ARG(instName);
  };
  ~MockSettingsTabPresenter() override{};
};

class MockSaveTabPresenter : public IReflSaveTabPresenter {
public:
  void notify(IReflSaveTabPresenter::Flag flag) override { UNUSED_ARG(flag); };
  void acceptMainPresenter(IReflMainWindowPresenter *presenter) override {
    UNUSED_ARG(presenter);
  };
  ~MockSaveTabPresenter() override{};
};

class MockMainWindowPresenter : public IReflMainWindowPresenter {
public:
  MOCK_CONST_METHOD1(getTransmissionRuns, std::string(int));
  MOCK_CONST_METHOD1(getTransmissionOptions, std::string(int));
  MOCK_CONST_METHOD1(getReductionOptions, std::string(int));
  MOCK_CONST_METHOD1(getStitchOptions, std::string(int));
  MOCK_CONST_METHOD1(setInstrumentName, void(const std::string &instName));
  MOCK_CONST_METHOD0(getInstrumentName, std::string());
  MOCK_METHOD1(notify, void(Flag));
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  // Other calls we don't care about
  std::string getTimeSlicingValues(int group) const override {
    UNUSED_ARG(group);
    return std::string();
  }
  std::string getTimeSlicingType(int group) const override {
    UNUSED_ARG(group);
    return std::string();
  }
  bool checkIfProcessing() const override { return false; }

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

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H*/
