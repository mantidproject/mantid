#ifndef MDALGORITHMS_PREPROCESS_DETECTORS2MD_TEST_H_
#define MDALGORITHMS_PREPROCESS_DETECTORS2MD_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;

class PrepcocessDetectorsToMDTestHelper : public PreprocessDetectorsToMD
{
public:
      boost::shared_ptr<DataObjects::TableWorkspace> createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS)
      {
        return PreprocessDetectorsToMD::createTableWorkspace(inputWS);
      }
      void processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,DataObjects::TableWorkspace_sptr &targWS)
      {
         PreprocessDetectorsToMD::processDetectorsPositions(inputWS,targWS);
      }
      void buildFakeDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,DataObjects::TableWorkspace_sptr &targWS)
      {
          PreprocessDetectorsToMD::buildFakeDetectorsPositions(inputWS,targWS);
      }
};


// Test is transformed from ConvetToQ3DdE but actually tests some aspects of ConvertToMD algorithm. 
class PreprocessDetectorsToMDTest : public CxxTest::TestSuite
{
 std::auto_ptr<PrepcocessDetectorsToMDTestHelper> pAlg;
 API::MatrixWorkspace_sptr ws2D;
 boost::shared_ptr<DataObjects::TableWorkspace> tws;

public:
static PreprocessDetectorsToMDTest *createSuite() { return new PreprocessDetectorsToMDTest(); }
static void destroySuite(PreprocessDetectorsToMDTest * suite) { delete suite; }  

void testCreateTarget()
{
  TS_ASSERT_THROWS_NOTHING(tws = pAlg->createTableWorkspace(ws2D));

  TS_ASSERT(tws);

  TS_ASSERT_EQUALS(4,tws->rowCount());
  TS_ASSERT_EQUALS(7,tws->columnCount());
}


void testPreprocessDetectors()
{
 
  TS_ASSERT_THROWS_NOTHING(pAlg->processDetectorsPositions(ws2D,tws));

  size_t nVal = tws->rowCount();

  const std::vector<size_t> & spec2detMap = tws->getColVector<size_t>("spec2detMap");
  for(size_t i=0;i<nVal;i++)
  {
    TS_ASSERT_EQUALS(i,spec2detMap[i]);
  }

  double L1(0);
  uint32_t nDet(0); 
  std::string InstrName;
  bool fakeDetectrors(false);
  TS_ASSERT_THROWS_NOTHING(nDet = tws->getProperty("ActualDetectorsNum"));
  TS_ASSERT_THROWS_NOTHING(L1  = tws->getProperty("L1"));
  TS_ASSERT_THROWS_NOTHING(InstrName =std::string(tws->getProperty("InstrumentName")));  
  TS_ASSERT_THROWS_NOTHING(fakeDetectrors=tws->getProperty("FakeDetectors"));  

  TS_ASSERT_DELTA(10,L1,1.e-11);
  TS_ASSERT_EQUALS(4,nDet);
  TS_ASSERT_EQUALS("basic",InstrName);
  TS_ASSERT(!fakeDetectrors);
}


void testFakeDetectors()
{
 
  TS_ASSERT_THROWS_NOTHING(pAlg->buildFakeDetectorsPositions(ws2D,tws));

  size_t nVal = tws->rowCount();

  auto & spec2detMap = tws->getColVector<size_t>("spec2detMap");
  auto & detId       = tws->getColVector<int32_t>("DetectorID");
  auto &detIDMap     = tws->getColVector<size_t>("detIDMap");
  auto &L2         = tws->getColVector<double>("L2");
  auto &TwoTheta   = tws->getColVector<double>("TwoTheta");
  auto &Azimuthal  = tws->getColVector<double>("Azimuthal");
  auto &detDir     = tws->getColVector<Kernel::V3D>("DetDirections"); 

  TS_ASSERT(&detDir[0]);
  TS_ASSERT_EQUALS(nVal,detDir.size());

  for(size_t i=0;i<nVal;i++)
  {
    TS_ASSERT_EQUALS(i,spec2detMap[i]);
    TS_ASSERT_EQUALS(i,detId[i]);
    TS_ASSERT_EQUALS(i,detIDMap[i]);
    TS_ASSERT_DELTA(1,L2[i],1.e-11);
    TS_ASSERT_DELTA(0,TwoTheta[i],1.e-11);
    TS_ASSERT_DELTA(0,Azimuthal[i],1.e-11);
  }

  double L1(0);
  uint32_t nDet(0); 
  std::string InstrName;
  bool fakeDetectrors(false);
  TS_ASSERT_THROWS_NOTHING(nDet = tws->getProperty("ActualDetectorsNum"));
  TS_ASSERT_THROWS_NOTHING(L1  = tws->getProperty("L1"));
  TS_ASSERT_THROWS_NOTHING(InstrName =std::string(tws->getProperty("InstrumentName")));  
  TS_ASSERT_THROWS_NOTHING(fakeDetectrors=tws->getProperty("FakeDetectors"));  

  TS_ASSERT_DELTA(1,L1,1.e-11);
  TS_ASSERT_EQUALS(4,nDet);
  TS_ASSERT_EQUALS("FakeInstrument",InstrName);
  TS_ASSERT(fakeDetectrors);
}

void testTheAlg()
{
    auto pAlg = std::auto_ptr<PrepcocessDetectorsToMDTestHelper>(new PrepcocessDetectorsToMDTestHelper());
    TS_ASSERT_THROWS_NOTHING(pAlg->initialize());

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace","testMatrWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace","PreprocessedDetectors"));

    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());

    API::Workspace_sptr wsOut =  API::AnalysisDataService::Instance().retrieve("PreprocessedDetectors");
    TSM_ASSERT("can not retrieve table worksapce from analysis data service ",wsOut);
    DataObjects::TableWorkspace_sptr tws = boost::dynamic_pointer_cast< DataObjects::TableWorkspace>(wsOut);
    TSM_ASSERT("can not interpet the workspace as table workspace",tws);

   double L1(0);
  uint32_t nDet(0); 
  std::string InstrName;
  bool fakeDetectrors(false);
  TS_ASSERT_THROWS_NOTHING(nDet = tws->getProperty("ActualDetectorsNum"));
  TS_ASSERT_THROWS_NOTHING(L1  = tws->getProperty("L1"));
  TS_ASSERT_THROWS_NOTHING(InstrName =std::string(tws->getProperty("InstrumentName")));  
  TS_ASSERT_THROWS_NOTHING(fakeDetectrors=tws->getProperty("FakeDetectors"));  

  TS_ASSERT_DELTA(10,L1,1.e-11);
  TS_ASSERT_EQUALS(4,nDet);
  TS_ASSERT_EQUALS("basic",InstrName);
  TS_ASSERT(!fakeDetectrors);

}


PreprocessDetectorsToMDTest()
{
     pAlg = std::auto_ptr<PrepcocessDetectorsToMDTestHelper>(new PrepcocessDetectorsToMDTestHelper());
     ws2D = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);

     API::AnalysisDataService::Instance().addOrReplace("testMatrWS", ws2D);


}

};

#endif