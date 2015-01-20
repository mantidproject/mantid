#ifndef ASYMMETRYCALCTEST_H_
#define ASYMMETRYCALCTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/AsymmetryCalc.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include <stdexcept>
#include <algorithm>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class AsymmetryCalcTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( asymCalc.name(), "AsymmetryCalc" )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( asymCalc.category(), "Muon" )
  }

  void testInit()
  {
    asymCalc.initialize();
    TS_ASSERT( asymCalc.isInitialized() )
  }

  void testLoadNexusAndSetProperties()
  {
    //Load the muon nexus file
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);

    asymCalc.setPropertyValue("InputWorkspace", "EMU6473");
    asymCalc.setPropertyValue("OutputWorkspace", "Result");
    asymCalc.setPropertyValue("Alpha", "1.0");
    asymCalc.setPropertyValue("ForwardSpectra", "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
    asymCalc.setPropertyValue("BackwardSpectra", "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32");
  }

  void testProperties()
  {
    //this test is badly written and requires the previous test to have succeeded
    TS_ASSERT_EQUALS( asymCalc.getPropertyValue("Alpha"), "1");
  }

  void testExecute()
  {
    //this test is badly written and requires the previous test to have succeeded
    try 
    {
      TS_ASSERT_EQUALS(asymCalc.execute(),true);
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Result");

    //Use a range as cxxtest seems to complain about the accuracy
    TS_ASSERT_DELTA(outputWS->dataY(0)[100],0.2965,0.005);
    TS_ASSERT( !outputWS->isHistogramData() );
  }

  void test_single_spectra()
  {
      auto ws = WorkspaceCreationHelper::Create2DWorkspace(3,10);
      for(size_t i = 0; i < ws->getNumberHistograms(); ++i)
      {
          auto &y = ws->dataY(i);
          std::fill(y.begin(),y.end(), static_cast<double>(i+1));
      }

      AsymmetryCalc alg;
      alg.initialize();
      alg.setProperty("InputWorkspace", ws);
      alg.setPropertyValue("OutputWorkspace", "Result");
      alg.setPropertyValue("ForwardSpectra", "1");
      alg.setPropertyValue("BackwardSpectra", "3");
      alg.execute();

      MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Result");
      TS_ASSERT_EQUALS( outputWS->readY(0)[0], -0.5 ); // == (1 - 3)/(1 + 3)
      TS_ASSERT_EQUALS( outputWS->readY(0)[6], -0.5 ); // == (1 - 3)/(1 + 3)
      TS_ASSERT_EQUALS( outputWS->readY(0)[9], -0.5 ); // == (1 - 3)/(1 + 3)
      TS_ASSERT( !outputWS->isHistogramData() );
  }

  void test_yUnitLabel()
  {
    const std::string outputWSName = "AsymmetryCalcTest_yUnitLabel_OutputWS";

    auto ws = WorkspaceCreationHelper::Create2DWorkspace(2,1);

    AsymmetryCalc alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("OutputWorkspace", outputWSName);
    alg.execute();

    auto result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWSName);

    TS_ASSERT(result);

    if( result )
    {
      TS_ASSERT_EQUALS( result->YUnitLabel(), "Asymmetry" );
    }

    AnalysisDataService::Instance().remove(outputWSName);
  }

private:
  AsymmetryCalc asymCalc;
  Mantid::DataHandling::LoadMuonNexus2 loader;
  Mantid::DataHandling::GroupDetectors group1;
  Mantid::DataHandling::GroupDetectors group2;

};

#endif /*ASYMMETRYCALCTEST_H_*/
