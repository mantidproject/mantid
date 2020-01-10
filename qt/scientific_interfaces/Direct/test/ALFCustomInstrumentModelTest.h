// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ALFCUSTOMINSTRUMENTMODELTEST_H_
#define MANTIDQT_ALFCUSTOMINSTRUMENTMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "ALFCustomInstrumentModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace{
  const std::string notALFFile = "ZOOM00006113.nxs";
}

class ALFCustomInstrumentModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ALFCustomInstrumentModelTest() { FrameworkManager::Instance(); }

  static ALFCustomInstrumentModelTest *createSuite() { return new ALFCustomInstrumentModelTest(); }

  static void destroySuite(ALFCustomInstrumentModelTest *suite) { delete suite; }

  void setUp() override {
    //m_workspace = createWorkspace(4, 3);
    //m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
  m_model = new ALFCustomInstrumentModel();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_model;
    //m_ads.reset();
    //m_workspace.reset();
    //m_model.reset();
  }

  void test_loadData() {
    return;
    //auto loadResult = m_model->loadData("ALF");
    //TS_ASSERT_EQUALS(loadResult.first, 1);
    //TS_ASSERT_EQUALS(loadResult.second, "success");
  }

  void test_loadDataNotALF(){
   std::pair<int, std::string> loadResult = m_model->loadData(notALFFile);
    TS_ASSERT_EQUALS(loadResult.first, 6113);
    TS_ASSERT_EQUALS(loadResult.second, "Not the correct instrument, expected ALF");
  }

  void test_loadDataDSpace(){
    //auto loadResult = m_model->loadData("ALF");
    //TS_ASSERT_EQUALS(loadResult.first, 1);
    //TS_ASSERT_EQUALS(loadResult.second, "success");
  }

  void test_isDataValid(){
    return;
  }

  void test_isDataValidNotALF(){
    auto alg = AlgorithmManager::Instance().create("Load");
    alg->initialize();
    alg->setProperty("Filename",notALFFile);
    alg->setProperty("OutputWorkspace","ALF_tmp");
    alg->execute();
    
    std::map<std::string,bool> result = m_model->isDataValid();
    TS_ASSERT(!result["IsValidInstrument"])
    TS_ASSERT(!result["IsItDspace"])
    // clean up
    AnalysisDataService::Instance().remove("ALF_tmp");

  }

  void test_isDataValidDSpace(){
    return;
  }

  void test_transformData(){
    return;
  }

  void test_storeSingleTube(){
   return;
  }
 
  void test_averageTube(){
   return;
  }

  void test_hasTubeBeenExtracted(){
   return;
  }

  void test_extractTubeCondition(){
   return;
  }
  void test_extractTubeConditionNotTube(){
   return;
  }

 void test_extractTubeConditionNoPlot(){
   return;
  }

 void test_averageTubeCondition(){
  return;
 }
 void test_averageTubeConditionNotTube(){
  return;
 }

 void test_averageTubeConditionNoPlot(){
  return;
 }
 void test_averageTubeConditionNothingToAverage(){
  return;
 }
 
 void test_defaultFunction(){
  return;
}



private:
  ALFCustomInstrumentModel *m_model;
};

#endif /* MANTIDQT_ALFCUSTOMINSTRUMENTMODELTEST_H_ */
