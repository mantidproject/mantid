#ifndef MUONREMOVEEXPDECAYTEST_H_
#define MUONREMOVEEXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAlgorithms/RemoveExpDecay.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class RemoveExpDecayTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "RemoveExpDecay" )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( alg.category(), "Muon" )
  }

  void testInit()
  {
    alg.initialize();
    TS_ASSERT( alg.isInitialized() )
  }

  void testLoadNexusAndSetProperties()
  {
  //This test does not run on Windows64 as is does not support HDF4 files

    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);

    alg.setPropertyValue("InputWorkspace", "EMU6473");
    alg.setPropertyValue("OutputWorkspace", "Result");
    alg.setPropertyValue("Spectra", "0");
  }

  void testProperties()
  {
    //This test does not run on Windows64 as is does not support HDF4 files
    TS_ASSERT_EQUALS( alg.getPropertyValue("Spectra"), "0");
  }

  void testExecute()
  {
      //This test does not run on Windows64 as is does not support HDF4 files
    try 
    {
      TS_ASSERT_EQUALS(alg.execute(),true);
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

    Workspace_const_sptr outputWS = AnalysisDataService::Instance().retrieve("Result");
  }

  void testWhereOptional3rdArgNotSet()
  {
  //This test does not run on Windows64 as is does not support HDF4 files

    MuonRemoveExpDecay alg2;
    alg2.initialize();

    alg2.setPropertyValue("InputWorkspace", "EMU6473");
    alg2.setPropertyValue("OutputWorkspace", "MuonRemoveExpDecayResult");

    try 
    {
      TS_ASSERT_EQUALS(alg2.execute(),true);
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }
  }

  void test_yUnitLabel()
  {
    const std::string outputWSName = "RemoveExpDecayTest_yUnitLabel_OutputWS";

    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    MuonRemoveExpDecay alg;
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
  MuonRemoveExpDecay alg;
  Mantid::DataHandling::LoadMuonNexus2 loader;

};

#endif /*MUONREMOVEEXPDECAYTEST_H_*/
