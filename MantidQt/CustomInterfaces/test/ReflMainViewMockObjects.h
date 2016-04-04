#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H

#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

class MockView : public ReflMainView {
public:
  MockView(){};
  ~MockView() override {}

  // Gmock requires parameters and return values of mocked methods to be
  // copyable
  // We can't mock setTableCommands(std::vector<ReflCommand_uptr>) because
  // of the vector of unique pointers
  // I will mock a proxy method, setTableCommandsProxy, I just want to test that
  // this method is invoked by the presenter's constructor
  virtual void setTableCommands(std::vector<ReflCommand_uptr>) override {
    setTableCommandsProxy();
  }
  // The same happens for setRowCommands
  virtual void setRowCommands(std::vector<ReflCommand_uptr>) override {
    setRowCommandsProxy();
  }

  // Prompts
  MOCK_METHOD3(askUserString,
               std::string(const std::string &prompt, const std::string &title,
                           const std::string &defaultValue));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD1(showAlgorithmDialog, void(const std::string &));

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
  virtual void setProgressRange(int, int){};
  virtual void setProgress(int){};
  boost::shared_ptr<IReflPresenter> getPresenter() const override {
    return boost::shared_ptr<IReflPresenter>();
  }
};

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
  ~MockProgressBase() override {}
};

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

#endif /*MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H*/
