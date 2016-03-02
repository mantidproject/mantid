#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H

#include <gmock/gmock.h>
#include "MantidQtCustomInterfaces/ProgressableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

//Clean column ids for use within tests
const int RunCol     = ReflTableSchema::COL_RUNS;
const int ThetaCol   = ReflTableSchema::COL_ANGLE;
const int TransCol   = ReflTableSchema::COL_TRANSMISSION;
const int QMinCol    = ReflTableSchema::COL_QMIN;
const int QMaxCol    = ReflTableSchema::COL_QMAX;
const int DQQCol     = ReflTableSchema::COL_DQQ;
const int ScaleCol   = ReflTableSchema::COL_SCALE;
const int GroupCol   = ReflTableSchema::COL_GROUP;
const int OptionsCol = ReflTableSchema::COL_OPTIONS;

class MockView : public ReflMainView
{
public:
  MockView(){};
  virtual ~MockView(){}

  //Prompts
  MOCK_METHOD3(askUserString, std::string(const std::string& prompt, const std::string& title, const std::string& defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_METHOD0(requestNotebookPath, std::string());

  MOCK_METHOD1(showAlgorithmDialog, void(const std::string&));
  MOCK_METHOD0(showImportDialog, void());
  MOCK_METHOD1(plotWorkspaces, void(const std::set<std::string>&));

  //IO
  MOCK_CONST_METHOD0(getWorkspaceToOpen, std::string());
  MOCK_METHOD1(setSelection, void(const std::set<int>& rows));
  MOCK_CONST_METHOD0(getSelectedRows, std::set<int>());
  MOCK_CONST_METHOD0(getSelectedSearchRows, std::set<int>());
  MOCK_METHOD1(setClipboard, void(const std::string& text));
  MOCK_CONST_METHOD0(getClipboard, std::string());
  MOCK_CONST_METHOD0(getSearchString, std::string());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_METHOD0(getEnableNotebook, bool());
  MOCK_CONST_METHOD0(getTransferMethod, std::string());
  MOCK_CONST_METHOD0(getAlgorithmRunner, boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_METHOD1(setTransferMethods, void(const std::set<std::string>&));

  //Calls we don't care about
  virtual void showTable(QReflTableModel_sptr) {};
  virtual void showSearch(ReflSearchModel_sptr) {};
  virtual void setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy*) {};
  virtual void setProgressRange(int,int) {};
  virtual void setProgress(int) {};
  virtual void setTableList(const std::set<std::string>&) {};
  virtual void setInstrumentList(const std::vector<std::string>&, const std::string&) {};
  virtual void saveSettings(const std::map<std::string,QVariant>&) {};
  virtual void loadSettings(std::map<std::string,QVariant>&) {};
  virtual std::string getProcessInstrument() const {return "FAKE";}
  virtual boost::shared_ptr<IReflPresenter> getPresenter() const {return boost::shared_ptr<IReflPresenter>();}
};

class MockProgressableView : public ProgressableView {
public:
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD0(clearProgress, void());
  ~MockProgressableView() {}
};

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
  ~MockProgressBase() {}
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
  virtual ~MockICatalogInfo() {}
};

#endif /*MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H*/
