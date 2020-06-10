// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentView.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
//#include "MantidTestHelpers/WorkspaceCreationHelper.h"
//#include "MantidAPI/NumericAxis.h"

//#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument.h"

#include <string>
#include <utility>
#include<iostream>
using namespace Mantid::API;
using Mantid::Geometry::Instrument;
using namespace MantidQt;
using namespace MantidQt::MantidWidgets;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW baseTest :  public BaseCustomInstrumentPresenter{
public:
    explicit baseTest(IBaseCustomInstrumentView *view, IBaseCustomInstrumentModel *model, IPlotFitAnalysisPanePresenter *analysis):BaseCustomInstrumentPresenter(view, model, analysis), m_add(0), m_load(0), m_layout(0),m_mockAdd(false), m_mockLoad(false), m_mockLayout(false){}; 
    ~baseTest() {}; 

// turn mocks on
    void setMockAdd(){m_mockAdd = true;};
    void setMockLoad(){m_mockLoad = true;};
    void setMockLayout(){m_mockLayout = true;};

    void initLayout(std::pair<instrumentSetUp, instrumentObserverOptions> *setup) override final{
    std::cout<<"initLayout "<<m_mockLayout<<" :P"<<std::endl;
    if(m_mockLayout == true){std::cout<<"true"<<std::endl; m_layout+=1;}
    else{std::cout<<"false"<<std::endl; BaseCustomInstrumentPresenter::initLayout(setup);}
    std::cout<<"sone"<<std::endl;
    };
    void loadRunNumber() override {
    std::cout<<"load "<<m_mockLoad<<" :P"<<std::endl;
    if(m_mockLoad){m_load+=1;}else{BaseCustomInstrumentPresenter::loadRunNumber();}
    };

    // get methods for mocks
    int getAddCount(){return m_add;};
    int getLayoutCount(){return m_layout;};
    int getLoadCount(){return m_load;};

    // allow tests to get at protected/private functions
    void loadAndAnalysis(const std::string &run) override {BaseCustomInstrumentPresenter::loadAndAnalysis(run);};
    void initInstrument(std::pair<instrumentSetUp, instrumentObserverOptions> *setUp) override {BaseCustomInstrumentPresenter::initInstrument(setUp);};
    void setUpInstrumentAnalysisSplitter() override {BaseCustomInstrumentPresenter::setUpInstrumentAnalysisSplitter();};
   std::pair<instrumentSetUp, instrumentObserverOptions> setupInstrument() override{ BaseCustomInstrumentPresenter::setupInstrument();};

private:
int m_add;
int m_load;
int m_layout;

bool m_mockAdd;
bool m_mockLoad;
bool m_mockLayout;
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW baseViewTest : public MantidQt::MantidWidgets::IBaseCustomInstrumentView{
public:
   explicit baseViewTest(const std::string &instrument, QWidget *parent=nullptr){};
   ~baseViewTest(){};

  MOCK_METHOD0(getFile, std::string());
  MOCK_METHOD1(setRunQuietly, void(const std::string &runNumber));
  MOCK_METHOD1(observeLoadRun, void(Observer *listener));
  MOCK_METHOD1(warningBox, void(const std::string &message));

  MOCK_METHOD1(setInstrumentWidget, void(InstrumentWidget *instrument));
  MOCK_METHOD0(getInstrumentView, MantidWidgets::InstrumentWidget*());
  MOCK_METHOD2(setUpInstrument, void(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &instrument));
  MOCK_METHOD1(addObserver, void(std::tuple<std::string, Observer *> &listener));
  MOCK_METHOD1(setupInstrumentAnalysisSplitters, void(QWidget *analysis));
  MOCK_METHOD0(setupHelp, void());
  // override getQWidget
  MOCK_METHOD0(getQWidget, QWidget*());
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW baseModelTest : public MantidQt::MantidWidgets::IBaseCustomInstrumentModel{
public:
  explicit baseModelTest(){};
  ~baseModelTest(){};

  MOCK_METHOD0(loadEmptyInstrument, void());
  MOCK_METHOD1(loadData, std::pair<int, std::string>(const std::string &name));
  MOCK_METHOD1(setCurrentRun, void(int &run));
  MOCK_METHOD0(getCurrentRun, int());
  MOCK_METHOD0(rename, void());
  MOCK_METHOD0(remove, void());
  MOCK_METHOD0(dataFileName, std::string());
  MOCK_METHOD0(currentRun, int());
  MOCK_METHOD1(isErrorCode, bool(const int run));
  MOCK_METHOD0(getInstrument, const std::string());
  MOCK_METHOD0(getTmpName, const std::string());
  MOCK_METHOD0(getWSName, const std::string());
};

