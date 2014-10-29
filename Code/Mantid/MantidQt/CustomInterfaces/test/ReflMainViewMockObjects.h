#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H

#include <gmock/gmock.h>
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidAPI/TableRow.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

//Clean flag aliases for use within tests.
const int NewTableFlag   = ReflMainView::NewTableFlag;
const int OpenTableFlag  = ReflMainView::OpenTableFlag;
const int SaveAsFlag     = ReflMainView::SaveAsFlag;
const int SaveFlag       = ReflMainView::SaveFlag;
const int ProcessFlag    = ReflMainView::ProcessFlag;
const int AppendRowFlag  = ReflMainView::AppendRowFlag;
const int PrependRowFlag = ReflMainView::PrependRowFlag;
const int DeleteRowFlag  = ReflMainView::DeleteRowFlag;
const int GroupRowsFlag  = ReflMainView::GroupRowsFlag;
const int ExpandSelectionFlag = ReflMainView::ExpandSelectionFlag;

//Clean column ids for use within tests
const int RunCol     = ReflMainViewPresenter::COL_RUNS;
const int ThetaCol   = ReflMainViewPresenter::COL_ANGLE;
const int TransCol   = ReflMainViewPresenter::COL_TRANSMISSION;
const int QMinCol    = ReflMainViewPresenter::COL_QMIN;
const int QMaxCol    = ReflMainViewPresenter::COL_QMAX;
const int DQQCol     = ReflMainViewPresenter::COL_DQQ;
const int ScaleCol   = ReflMainViewPresenter::COL_SCALE;
const int GroupCol   = ReflMainViewPresenter::COL_GROUP;
const int OptionsCol = ReflMainViewPresenter::COL_OPTIONS;

class MockView : public ReflMainView
{
public:
  MockView(){};
  virtual ~MockView(){}
  virtual void showTable(QReflTableModel_sptr model){ m_model = model;}
  virtual void setOptionsHintStrategy(HintStrategy*) {};
  MOCK_METHOD3(askUserString, std::string(const std::string& prompt, const std::string& title, const std::string& defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD1(setTableList, void(const std::set<std::string>& tableList));
  MOCK_METHOD2(setInstrumentList, void(const std::vector<std::string>& instruments, const std::string& defaultInstrument));
  MOCK_METHOD1(setInstrument, void(const std::string&));
  MOCK_METHOD1(setSelection, void(const std::set<int>& rows));
  MOCK_CONST_METHOD0(getSelectedRows, std::set<int>());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_CONST_METHOD0(getProcessInstrument, std::string());
  MOCK_CONST_METHOD0(getWorkspaceToOpen, std::string());
  MOCK_CONST_METHOD0(getPresenter, boost::shared_ptr<IReflPresenter>());
private:
  QReflTableModel_sptr m_model;
};

#endif /*MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H*/
