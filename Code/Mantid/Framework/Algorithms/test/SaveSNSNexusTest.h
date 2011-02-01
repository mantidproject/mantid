#ifndef SAVESNSNEXUSTEST_H_
#define SAVESNSNEXUSTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidNexus/SaveSNSNexus.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidNexus/LoadNeXus.h"
#include "Poco/Path.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

class SaveSNSNexusTest : public CxxTest::TestSuite
{
public: 
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }
  
  void xtestExec()
  {
    IAlgorithm_sptr alg( AlgorithmFactory::Instance().create("LoadSNSEventNexus", 1) );
    alg->initialize();
    alg->setProperty("Filename", "/home/8oz/data/TOPAZ_1786_event.nxs");
    alg->setProperty("OutputWorkspace", "savesnsnexus_workspace");
    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    IAlgorithm_sptr rebin( AlgorithmFactory::Instance().create("Rebin", 1) );
    rebin->initialize();
    rebin->setProperty("InputWorkspace", "savesnsnexus_workspace");
    rebin->setProperty("Params", "400,-0.004,44988.2,11.8,45000");
    //rebin->setProperty("Params", "0, 1, 300");
    rebin->setProperty("OutputWorkspace", "savesnsnexus_workspace");
    rebin->execute();
    TS_ASSERT( rebin->isExecuted() );

    IAlgorithm_sptr save( AlgorithmFactory::Instance().create("SaveSNSNexus", 1) );
    save->initialize();
    save->setProperty("InputFilename", "/home/8oz/data/TOPAZ_1786.nxs");
    save->setProperty("InputWorkspace", "savesnsnexus_workspace");
    save->setProperty("OutputFilename", "/home/8oz/data/TOPAZ_1786_mantid.nxs");
    save->setProperty("Compress", "1");
    save->execute();
    TS_ASSERT( save->isExecuted() );
  }

  
private:
  SaveSNSNexus algToBeTested;
  std::string outputFile;
  std::string title;
  int entryNumber;
  
};
#endif /*SaveSNSNexusTEST_H_*/
