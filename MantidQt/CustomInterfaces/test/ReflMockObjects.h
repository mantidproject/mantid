#ifndef MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H

#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabView.h"
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
  MOCK_METHOD1(setTransferMethods, void(const std::set<std::string> &));
  MOCK_METHOD0(setTableCommandsProxy, void());
  MOCK_METHOD0(setRowCommandsProxy, void());
  MOCK_METHOD0(clearCommands, void());
  MOCK_METHOD2(setInstrumentList,
               void(const std::vector<std::string> &, const std::string &));

  // Calls we don't care about
  void showSearch(ReflSearchModel_sptr) override{};
  IReflRunsTabPresenter *getPresenter() const override { return nullptr; }
};

class MockSettingsTabView : public IReflSettingsTabView {
public:
  // Global options
  MOCK_CONST_METHOD0(getPlusOptions, std::string());
  MOCK_CONST_METHOD0(getTransmissionOptions, std::string());
  MOCK_CONST_METHOD0(getReductionOptions, std::string());
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  MOCK_CONST_METHOD0(getAnalysisMode, std::string());
  MOCK_CONST_METHOD0(getCRho, std::string());
  MOCK_CONST_METHOD0(getCAlpha, std::string());
  MOCK_CONST_METHOD0(getCAp, std::string());

  // Calls we don't care about
  void
  createPlusHints(const std::map<std::string, std::string> &hints) override {
    UNUSED_ARG(hints);
  };
  void createTransmissionHints(
      const std::map<std::string, std::string> &hints) override {
    UNUSED_ARG(hints);
  };
  void createReductionHints(
      const std::map<std::string, std::string> &hints) override {
    UNUSED_ARG(hints);
  };
  void
  createStitchHints(const std::map<std::string, std::string> &hints) override {
    UNUSED_ARG(hints);
  };
  IReflSettingsTabPresenter *getPresenter() const override { return nullptr; }
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

class MockSettingsTabPresenter : public IReflSettingsTabPresenter {
public:
  MOCK_CONST_METHOD0(getPlusOptions, std::string());
  MOCK_CONST_METHOD0(getTransmissionOptions, std::string());
  MOCK_CONST_METHOD0(getReductionOptions, std::string());
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  // Other calls we don't care about
  void acceptMainPresenter(IReflMainWindowPresenter *presenter) override {
    UNUSED_ARG(presenter);
  };
  ~MockSettingsTabPresenter() override{};
};

class MockMainWindowPresenter : public IReflMainWindowPresenter {
public:
  MOCK_CONST_METHOD0(getPlusOptions, std::string());
  MOCK_CONST_METHOD0(getTransmissionOptions, std::string());
  MOCK_CONST_METHOD0(getReductionOptions, std::string());
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
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
