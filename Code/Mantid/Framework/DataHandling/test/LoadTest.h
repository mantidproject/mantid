#ifndef LOADTEST_H_
#define LOADTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class LoadTest : public CxxTest::TestSuite
{
public:


  void testViaProxy()
  {
    IAlgorithm_sptr proxy = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(proxy->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("OutputWorkspace"), true);
    
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","IRS38633.raw"));
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);

    proxy->setPropertyValue("Filename","IRS38633.raw");
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);

    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","LOQ49886.nxs"));   
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), false);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), false);
  }

  void testPropertyValuesViaProxy()
  {
    IAlgorithm_sptr proxy = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(proxy->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("OutputWorkspace"), true);
    
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","IRS38633.raw"));
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMin","10"));
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMax","100"));

    // Test that the properties have the correct values
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMin"),"10");
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMax"),"100");
  }

  void testSwitchingLoaderViaProxy()
  {
    IAlgorithm_sptr proxy = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(proxy->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("OutputWorkspace"), true);
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","IRS38633.raw"));
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);

    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMin","10"));
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMax","100"));

    // Test that the properties have the correct values
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMin"),"10");
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMax"),"100");

    // Change loader
    proxy->setPropertyValue("Filename","LOQ49886.nxs");
    TS_ASSERT_EQUALS(proxy->existsProperty("EntryNumber"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), false);

    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMin","11"));
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMax","101"));

    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMin"),"11");
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMax"),"101");

  }

  void testFindLoader()
  {
    Load loader;
    loader.initialize();
    const char * loadraw_props[5] = {"SpectrumMin", "SpectrumMax", "SpectrumList", "Cache", "LoadLogFiles"};
    const size_t numProps = (size_t)(sizeof(loadraw_props)/sizeof(const char*));
    // Basic load has no additional loader properties
    for( size_t i = 0; i < numProps ; ++i )
    {
      TS_ASSERT_EQUALS(loader.existsProperty(loadraw_props[i]), false);
    }
    //After setting the file property, the algorithm should have aquired the appropriate properties
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename","IRS38633.raw"));
    // Now
    for( size_t i = 0; i < numProps; ++i )
    {
      TS_ASSERT_EQUALS(loader.existsProperty(loadraw_props[i]), true);
    }

    // Did it find the right loader
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadRaw");
  }

  void testRaw()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","IRS38633.raw");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void testRawWithOneSpectrum()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","IRS38633.raw");
    const std::string outputName("LoadTest_IRS38633raw");
    loader.setPropertyValue("OutputWorkspace", outputName);
    loader.setPropertyValue("SpectrumList", "1");
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    AnalysisDataServiceImpl& dataStore = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS(dataStore.doesExist(outputName), true);
    
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(outputName));
    if(!ws) TS_FAIL("Cannot retrieve workspace from the store");

    // Check it only has 1 spectrum
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 );
    AnalysisDataService::Instance().remove(outputName);
  }

  void testRaw1()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","HRP37129.s02");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void testRawGroup()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","EVS13895.raw");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(wsg);
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output_1"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
    AnalysisDataService::Instance().remove("LoadTest_Output_1");
    AnalysisDataService::Instance().remove("LoadTest_Output_2");
    AnalysisDataService::Instance().remove("LoadTest_Output_3");
    AnalysisDataService::Instance().remove("LoadTest_Output_4");
    AnalysisDataService::Instance().remove("LoadTest_Output_5");
    AnalysisDataService::Instance().remove("LoadTest_Output_6");
  }
  
  void testHDF4Nexus()
  {
    // Note that there are no 64-bit HDF4 libraries for Windows.
#ifndef _WIN64
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
    #endif
  }

  void test_ARGUS_NXS()
  {
#ifndef _WIN64
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename","argus0026287.nxs"));

    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadMuonNexus");
#endif
  }

  void xtestHDF4NexusGroup()
  {
    // Note that there are no 64-bit HDF4 libraries for Windows.
#ifndef _WIN64
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","MUSR00015189.nxs");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(wsg);
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output_1"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
    AnalysisDataService::Instance().remove("LoadTest_Output_1");
    AnalysisDataService::Instance().remove("LoadTest_Output_2");
#endif
  }
   void testISISNexus()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ49886.nxs");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void testUnknownExt()
  {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS(loader.setPropertyValue("Filename","hrpd_new_072_01.cal"), std::runtime_error);
  }

  void testSPE()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","Example.spe");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void testAscii()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","AsciiExample.txt");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void testSpice2D()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","BioSANS_exp61_scan0004_0001.xml");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }
  void testSNSSpec()
  {
     Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LoadSNSspec.txt");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void testGSS()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","gss.txt");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

   void testRKH()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","DIRECT.041");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void test_EventPreNeXus_WithNoExecute()
  {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860_neutron_event.dat"));
    TS_ASSERT_EQUALS(loader.existsProperty("EventFilename"), false);
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadEventPreNeXus");
  }

  void test_SNSEventNeXus_WithNoExecute()
  {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860.nxs"));
    TS_ASSERT_EQUALS(loader.existsProperty("EventFilename"), false);
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadEventNexus");
  }
};

#endif /*LOADTEST_H_*/
