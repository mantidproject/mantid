// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFCustomInstrumentMocks.h"
#include "ALFCustomInstrumentModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/Instrument.h"

#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;
using Mantid::Geometry::Instrument;

namespace {
const std::string notALFFile = "ZOOM00006113.nxs";
}

class ALFCustomInstrumentModelTest : public CxxTest::TestSuite {
public:
  ALFCustomInstrumentModelTest() { FrameworkManager::Instance(); }

  static ALFCustomInstrumentModelTest *createSuite() { return new ALFCustomInstrumentModelTest(); }

  static void destroySuite(ALFCustomInstrumentModelTest *suite) { delete suite; }

  void setUp() override { m_model = new PartMockALFCustomInstrumentModel(); }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_model;
  }

  void test_loadData() {
    auto data = mockALFData("ALF_tmp", "ALF", 6113, true);
    TS_ASSERT_EQUALS(m_model->getLoadCount(), 0);

    std::pair<int, std::string> loadResult = m_model->loadData(notALFFile);
    TS_ASSERT_EQUALS(m_model->getLoadCount(), 1);
    TS_ASSERT_EQUALS(m_model->getTransformCount(), 1);
    TS_ASSERT_EQUALS(loadResult.first, 6113);
    TS_ASSERT_EQUALS(loadResult.second, "success");
  }

  void test_loadDataNotALF() {
    auto data = mockALFData("ALF_tmp", "EMU", 6113, true);
    TS_ASSERT_EQUALS(m_model->getLoadCount(), 0);

    std::pair<int, std::string> loadResult = m_model->loadData(notALFFile);
    TS_ASSERT_EQUALS(m_model->getLoadCount(), 1);
    TS_ASSERT_EQUALS(m_model->getTransformCount(), 0);
    TS_ASSERT_EQUALS(loadResult.first, 6113);
    TS_ASSERT_EQUALS(loadResult.second, "Not the correct instrument, expected ALF");
  }

  void test_loadDataDSpace() {
    auto data = mockALFData("ALF_tmp", "ALF", 6113, false);
    TS_ASSERT_EQUALS(m_model->getLoadCount(), 0);

    std::pair<int, std::string> loadResult = m_model->loadData(notALFFile);
    TS_ASSERT_EQUALS(m_model->getLoadCount(), 1);
    TS_ASSERT_EQUALS(m_model->getTransformCount(), 0);
    TS_ASSERT_EQUALS(loadResult.first, 6113);
    TS_ASSERT_EQUALS(loadResult.second, "success");
  }

  void test_isDataValid() {
    auto data = mockALFData("ALF_tmp", "ALF", 6113, true);
    std::map<std::string, bool> isDataValid = m_model->isDataValid();

    TS_ASSERT(isDataValid["IsValidInstrument"])
    TS_ASSERT(!isDataValid["IsItDSpace"])
  }

  void test_isDataValidNotALF() {
    auto data = mockALFData("ALF_tmp", "EMU", 6113, true);
    std::map<std::string, bool> isDataValid = m_model->isDataValid();
    TS_ASSERT(!isDataValid["IsValidInstrument"])
    TS_ASSERT(!isDataValid["IsItDSpace"])
  }

  void test_isDataValidDSpace() {
    auto data = mockALFData("ALF_tmp", "ALF", 6113, false);
    std::map<std::string, bool> isDataValid = m_model->isDataValid();

    TS_ASSERT(isDataValid["IsValidInstrument"]);
    TS_ASSERT(isDataValid["IsItDSpace"]);
  }

  void test_storeSingleTube() {
    auto data = mockALFData("CURVES", "ALF", 6113, false);

    m_model->storeSingleTube("test");

    auto outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("extractedTubes_test");
    TS_ASSERT_DELTA(outputWS->readX(0)[0], -22.9, 0.1);
    TS_ASSERT_EQUALS(outputWS->readY(0)[0], 0.2);
    TS_ASSERT_DELTA(outputWS->readX(0)[9], 492.7, 0.1);
    TS_ASSERT_EQUALS(outputWS->readY(0)[9], 0.2);

    AnalysisDataService::Instance().remove("extractedTubes_test");
  }

  void test_averageTube() {
    int run = 6113;
    auto data = mockALFData("CURVES", "ALF", run, false);
    m_model->setCurrentRun(run);
    m_model->extractSingleTube();

    // check original y values
    auto tmpWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("extractedTubes_ALF6113");
    TS_ASSERT_EQUALS(tmpWS->readY(0)[1], 0.2);
    TS_ASSERT_EQUALS(tmpWS->readY(0)[9], 0.2);

    // create another ws to add
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 10, false, 1.1, 2.2, 0.01, 0.3);
    std::shared_ptr<Instrument> inst = std::make_shared<Instrument>();
    inst->setName("ALF");
    ws->mutableRun().addProperty("run_number", run, true);
    ws->setInstrument(inst);
    auto axis = ws->getAxis(0);
    axis->setUnit("dSpacing");
    AnalysisDataService::Instance().addOrReplace("CURVES", ws);

    // check second WS y values
    TS_ASSERT_DELTA(ws->readY(0)[1], 2.2, 0.001);
    TS_ASSERT_DELTA(ws->readY(0)[9], 2.2, 0.001);

    m_model->averageTube();

    // check averages: (2.2+0.2)/2 = 2.4/2 = 1.2
    auto outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("extractedTubes_ALF6113");
    TS_ASSERT_DELTA(outputWS->readX(0)[1], 34.4, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[1], 1.2, 0.01);
    TS_ASSERT_DELTA(outputWS->readX(0)[9], 492.7, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[9], 1.2, 0.01);

    // create another ws to add
    auto ws3 = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 10, false, 1.1, 3.2, 0.01, 0.3);
    inst = std::make_shared<Instrument>();
    inst->setName("ALF");
    ws3->mutableRun().addProperty("run_number", run, true);
    ws3->setInstrument(inst);
    axis = ws3->getAxis(0);
    axis->setUnit("dSpacing");
    AnalysisDataService::Instance().addOrReplace("CURVES", ws3);

    // check second WS y values
    TS_ASSERT_DELTA(ws3->readY(0)[1], 3.2, 0.001);
    TS_ASSERT_DELTA(ws3->readY(0)[9], 3.2, 0.001);

    m_model->averageTube();

    // check averages: (2.2+0.2+3.2)/2 = 5.6/3 = 1.8666
    outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("extractedTubes_ALF6113");
    TS_ASSERT_DELTA(outputWS->readX(0)[1], 34.4, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[1], 1.86, 0.01);
    TS_ASSERT_DELTA(outputWS->readX(0)[9], 492.7, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[9], 1.86, 0.01);

    AnalysisDataService::Instance().remove("extractedTubes_ALF6113");
  }

  void test_hasTubeBeenExtracted() {
    const std::string name = "test";
    // not extracted -> false
    TS_ASSERT(!m_model->hasTubeBeenExtracted(name));
    // create data to store
    auto data = mockALFData("CURVES", "ALF", 6113, false);
    m_model->storeSingleTube(name);
    // stored data -> true
    TS_ASSERT(m_model->hasTubeBeenExtracted(name));
  }

  void test_extractTubeCondition() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", true}};
    TS_ASSERT(m_model->extractTubeCondition(conditions));
  }
  void test_extractTubeConditionNotTube() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", false}};
    TS_ASSERT(!m_model->extractTubeCondition(conditions));
  }

  void test_extractTubeConditionNoPlot() {
    std::map<std::string, bool> conditions = {{"plotStored", false}, {"hasCurve", true}, {"isTube", true}};
    TS_ASSERT(m_model->extractTubeCondition(conditions));
  }
  void test_extractTubeConditionNoCurve() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", false}, {"isTube", true}};
    TS_ASSERT(m_model->extractTubeCondition(conditions));
  }
  void test_extractTubeConditionNoPlotOrCurve() {
    std::map<std::string, bool> conditions = {{"plotStored", false}, {"hasCurve", false}, {"isTube", true}};
    TS_ASSERT(!m_model->extractTubeCondition(conditions));
  }

  void test_averageTubeCondition() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", true}};
    int run = 6113;
    auto data = mockALFData("CURVES", "ALF", run, false);
    m_model->setCurrentRun(run);
    m_model->extractSingleTube();

    TS_ASSERT(m_model->averageTubeCondition(conditions));
    AnalysisDataService::Instance().remove("extractedTubes_ALF6113");
  }
  void test_averageTubeConditionNotTube() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", false}};
    int run = 6113;
    auto data = mockALFData("CURVES", "ALF", run, false);
    m_model->setCurrentRun(run);
    m_model->extractSingleTube();

    TS_ASSERT(!m_model->averageTubeCondition(conditions));
    AnalysisDataService::Instance().remove("extractedTubes_ALF6113");
  }

  void test_averageTubeConditionNoPlot() {
    std::map<std::string, bool> conditions = {{"plotStored", false}, {"hasCurve", false}, {"isTube", true}};
    int run = 6113;
    auto data = mockALFData("CURVES", "ALF", run, false);
    m_model->setCurrentRun(run);
    m_model->extractSingleTube();

    TS_ASSERT(!m_model->averageTubeCondition(conditions));
    AnalysisDataService::Instance().remove("extractedTubes_ALF6113");
  }

  void test_averageTubeConditionNothingToAverage() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", true}};
    int run = 6113;
    // the extraced ws will exist but average will be 0 -> change runs
    auto data = mockALFData("extractedTubes_ALF6113", "ALF", run, false);
    m_model->setCurrentRun(run);

    TS_ASSERT(!m_model->averageTubeCondition(conditions));
  }

  void test_averageTubeConditionNoWSToAverage() {
    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", true}};
    int run = 6113;
    auto data = mockALFData("CURVES", "ALF", run, false);
    m_model->setCurrentRun(run);
    m_model->extractSingleTube();

    // the extraced ws will not exist but average will be 1
    AnalysisDataService::Instance().remove("extractedTubes_ALF6113");
    TS_ASSERT(!m_model->averageTubeCondition(conditions));
  }

  void test_defaultFunction() {
    auto function = m_model->getDefaultFunction();
    TS_ASSERT_DELTA(function->getParameter("f0.A0"), 0.0, 0.01);
    TS_ASSERT_DELTA(function->getParameter("f1.Height"), 3.0, 0.01);
    TS_ASSERT_DELTA(function->getParameter("f1.PeakCentre"), 0.0, 0.01);
    TS_ASSERT_DELTA(function->getParameter("f1.Sigma"), 1.0, 0.01);
  }

private:
  PartMockALFCustomInstrumentModel *m_model;
};
