#ifndef MUONALPHACALCTEST_H_
#define MUONALPHACALCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/MuonAlphaCalc.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class MuonAlphaCalcTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( alphaCalc.name(), "AlphaCalc" )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( alphaCalc.category(), "Muon" )
  }

  void testInit()
  {
    alphaCalc.initialize();
    TS_ASSERT( alphaCalc.isInitialized() )
  }

  void testCalAlphaManySpectra()
  {
    //This test does not compile on Windows64 as is does not support HDF4 files
    #ifndef _WIN64

    //Load the muon nexus file
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);


    alphaCalc.setPropertyValue("InputWorkspace", "EMU6473");
    alphaCalc.setPropertyValue("ForwardSpectra", "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
    alphaCalc.setPropertyValue("BackwardSpectra", "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32");
    alphaCalc.setPropertyValue("FirstGoodValue", "0.3");

    try 
    {
      TS_ASSERT_EQUALS(alphaCalc.execute(),true);
    }
    catch(std::runtime_error e)
    {
      TS_FAIL(e.what());
    }
    double alpha = alphaCalc.getProperty("Alpha");
    TS_ASSERT_DELTA(alpha,1.7875,0.0001);

    #endif /*_WIN64*/
  }

  void testCalAlphaTwoSpectra()
  {
    //This test does not compile on Windows64 as is does not support HDF4 files
    #ifndef _WIN64

    //Load the muon nexus file
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);


    alphaCalc.setPropertyValue("InputWorkspace", "EMU6473");
    alphaCalc.setPropertyValue("ForwardSpectra", "1");
    alphaCalc.setPropertyValue("BackwardSpectra", "17");
    alphaCalc.setPropertyValue("FirstGoodValue", "0.3");

    try 
    {
      TS_ASSERT_EQUALS(alphaCalc.execute(),true);
    }
    catch(std::runtime_error e)
    {
      TS_FAIL(e.what());
    }
    double alpha = alphaCalc.getProperty("Alpha");
    TS_ASSERT_DELTA(alpha,1.6880,0.0001);

    #endif /*_WIN64*/
  }


private:
  MuonAlphaCalc alphaCalc;
  Mantid::DataHandling::LoadMuonNexus loader;

};

#endif /*MUONALPHACALCTEST_H_*/
