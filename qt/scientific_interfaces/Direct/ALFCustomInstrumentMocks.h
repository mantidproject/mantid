// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentPresenter.h"
#include "ALFCustomInstrumentView.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/WarningSuppressions.h"

#include <string>
#include <utility>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class mockALFData {
public:
  mockALFData(const std::string &name, const std::string &instName, const int &run, const bool TOF) : m_name(name) {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 10, false, 0.1, 0.2, 0.01, 0.3);
    // set instrument
    std::shared_ptr<Instrument> inst = std::make_shared<Instrument>();
    inst->setName(instName);
    // set run
    ws->mutableRun().addProperty("run_number", run, true);
    // set units
    ws->setInstrument(inst);
    auto axis = ws->getAxis(0);
    if (TOF) {
      axis->setUnit("TOF");
    } else {
      axis->setUnit("dSpacing");
    }
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
  }
  ~mockALFData() { AnalysisDataService::Instance().remove(m_name); }

private:
  std::string m_name;
};

class MockALFCustomInstrumentModel : public IALFCustomInstrumentModel {
public:
  MockALFCustomInstrumentModel(){};
  ~MockALFCustomInstrumentModel(){};

  MOCK_METHOD1(loadAlg, void(const std::string &name));
  MOCK_METHOD0(transformData, void());
  MOCK_METHOD0(isDataValid, std::map<std::string, bool>());
  MOCK_METHOD1(storeSingleTube, void(const std::string &name));
  MOCK_METHOD0(averageTube, void());
  MOCK_METHOD1(hasTubeBeenExtracted, bool(const std::string &name));
  MOCK_METHOD1(extractTubeCondition, bool(std::map<std::string, bool> tabBools));
  MOCK_METHOD1(averageTubeCondition, bool(std::map<std::string, bool> tabBools));
  MOCK_METHOD0(extractSingleTube, void());
  MOCK_METHOD0(WSName, std::string());
  MOCK_METHOD0(getDefaultFunction, Mantid::API::CompositeFunction_sptr());
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

class MockALFCustomInstrumentView : public IALFCustomInstrumentView {
public:
  explicit MockALFCustomInstrumentView(const std::string &instrument, QWidget *parent = nullptr) {
    (void)instrument;
    (void)parent;
  };
  ~MockALFCustomInstrumentView(){};

  MOCK_METHOD1(observeExtractSingleTube, void(Observer *listner));
  MOCK_METHOD1(observeAverageTube, void(Observer *listner));
  MOCK_METHOD1(addSpectrum, void(const std::string &name));
  MOCK_METHOD1(setupAnalysisPane, void(IPlotFitAnalysisPaneView *analysis));
  MOCK_METHOD0(getFile, std::string());
  MOCK_METHOD1(setRunQuietly, void(const std::string &runNumber));
  MOCK_METHOD1(observeLoadRun, void(Observer *listener));
  MOCK_METHOD1(warningBox, void(const std::string &error));
  MOCK_METHOD1(setInstrumentWidget, void(InstrumentWidget *instrument));
  MOCK_METHOD0(getInstrumentView, InstrumentWidget *());
  MOCK_METHOD2(setUpInstrument, void(const std::string &fileName,
                                     std::vector<std::function<bool(std::map<std::string, bool>)>> &binders));
  MOCK_METHOD1(addObserver, void(std::tuple<std::string, Observer *> &listener));
  MOCK_METHOD1(setupInstrumentAnalysisSplitters, void(QWidget *analysis));
  MOCK_METHOD0(setupHelp, void());
};

// want a partial mock of the model - removes the functions that just run algs.
class PartMockALFCustomInstrumentModel : public ALFCustomInstrumentModel {
public:
  PartMockALFCustomInstrumentModel() : m_loadCount(0), m_transformCount(0){};
  virtual ~PartMockALFCustomInstrumentModel(){};
  void loadAlg(const std::string &name) override final {
    (void)name;
    m_loadCount += 1;
  };
  void transformData() override final { m_transformCount += 1; };
  int getLoadCount() { return m_loadCount; };
  int getTransformCount() { return m_transformCount; };

private:
  int m_loadCount;
  int m_transformCount;
};
