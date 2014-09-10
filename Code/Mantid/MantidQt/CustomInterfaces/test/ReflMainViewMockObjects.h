#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H

#include <gmock/gmock.h>
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/TableRow.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

//Clean flag aliases for use within tests.
const ReflMainView::Flag saveAsFlag = ReflMainView::SaveAsFlag;
const ReflMainView::Flag saveFlag = ReflMainView::SaveFlag;
const ReflMainView::Flag processFlag = ReflMainView::ProcessFlag;
const ReflMainView::Flag addRowFlag = ReflMainView::AddRowFlag;
const ReflMainView::Flag deleteRowFlag = ReflMainView::DeleteRowFlag;

class ConstructView : public ReflMainView
{
public:
  ConstructView(){};
  MOCK_METHOD1(showTable, void(Mantid::API::ITableWorkspace_sptr));
  MOCK_METHOD3(askUserString, bool(const std::string& prompt, const std::string& title, const std::string& defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_CONST_METHOD0(getUserString, std::string());
  MOCK_METHOD0(getFlag, Flag());
  MOCK_CONST_METHOD0(flagSet, bool());
  MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
  virtual ~ConstructView(){}
};

class MockView : public ReflMainView
{
public:
  MockView(){};
  virtual void showTable(Mantid::API::ITableWorkspace_sptr model){(void)model;}
  MOCK_METHOD3(askUserString, bool(const std::string& prompt, const std::string& title, const std::string& defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_CONST_METHOD0(getUserString, std::string());
  MOCK_METHOD0(getFlag, Flag());
  MOCK_CONST_METHOD0(flagSet, bool());
  MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
  virtual ~MockView(){}
};

class FakeView : public ReflMainView
{
public:
  FakeView(){};
  virtual void showTable(Mantid::API::ITableWorkspace_sptr model)
  {
    TableRow row = model->appendRow();
    row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
    row = model->appendRow();
    row << "13462" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
    row = model->appendRow();
    row << "13469" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
    row = model->appendRow();
    row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
  }
  MOCK_METHOD3(askUserString, bool(const std::string& prompt, const std::string& title, const std::string& defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_CONST_METHOD0(getUserString, std::string());
  MOCK_METHOD0(getFlag, Flag());
  MOCK_CONST_METHOD0(flagSet, bool());
  MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
  virtual ~FakeView(){}
};

class AddDelProcView : public ReflMainView
{
public:
  AddDelProcView(){};
  virtual void showTable(Mantid::API::ITableWorkspace_sptr model)
  {
    m_model = model;
  }
  MOCK_METHOD3(askUserString, bool(const std::string& prompt, const std::string& title, const std::string& defaultValue));
  MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
  MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
  MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
  MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
  MOCK_CONST_METHOD0(getUserString, std::string());
  MOCK_METHOD0(getFlag, Flag());
  MOCK_CONST_METHOD0(flagSet, bool());
  MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
  virtual ~AddDelProcView(){}
  void addDataForTest()
  {
    TableRow row = m_model->appendRow();
    row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
    row = m_model->appendRow();
    row << "13462" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
    row = m_model->appendRow();
    row << "13469" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 1;
    row = m_model->appendRow();
    row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 1;
    m_model->removeRow(0);
  }
private:
  Mantid::API::ITableWorkspace_sptr m_model;
};

#endif /*MANTID_CUSTOMINTERFACES_REFLMAINVIEWMOCKOBJECTS_H*/
