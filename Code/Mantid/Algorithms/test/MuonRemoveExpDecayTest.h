#ifndef MUONREMOVEEXPDECAYTEST_H_
#define MUONREMOVEEXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonNexus.h"
#include "MantidAlgorithms/MuonRemoveExpDecay.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class MuonRemoveExpDecayTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "MuonRemoveExpDecay" )
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
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/Nexus/emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);

    alg.setPropertyValue("InputWorkspace", "EMU6473");
    alg.setPropertyValue("OutputWorkspace", "Result");
    alg.setPropertyValue("Spectra", "0");
  }

  void testProperties()
  {
    TS_ASSERT_EQUALS( alg.getPropertyValue("Spectra"), "0");
  }

  void testExecute()
  {
    try 
    {
      TS_ASSERT_EQUALS(alg.execute(),true);
    }
    catch(std::runtime_error e)
    {
      TS_FAIL(e.what());
    }

    Workspace_const_sptr outputWS = AnalysisDataService::Instance().retrieve("Result");
  }

private:
  MuonRemoveExpDecay alg;
  Mantid::NeXus::LoadMuonNexus loader;

};

#endif /*MUONREMOVEEXPDECAYTEST_H_*/
