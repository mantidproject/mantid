#ifndef LOADTEST_H_
#define LOADTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class LoadTest : public CxxTest::TestSuite
{
public:
  void testRaw()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","IRS38633.raw");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
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
  //disabled because hdf4 can't be opened on windows 64-bit, I think
  void t1estNexus()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("LoadTest_Output"));
    TS_ASSERT(ws);
    AnalysisDataService::Instance().remove("LoadTest_Output");
  }

  void t1estNexusGroup()
  {
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
  }
   void t1estISISNexus()
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

  void testEntryNumber()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","TEST00000008.nxs");
    loader.setPropertyValue("OutputWorkspace","LoadTest_entry2");
    loader.setPropertyValue("EntryNumber","2");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    Workspace2D_sptr wsg = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("LoadTest_entry2"));
    TS_ASSERT(wsg);
    AnalysisDataService::Instance().remove("LoadTest_entry2");
  }
  void testUnknownExt()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","hrpd_new_072_01.cal");
    loader.setPropertyValue("OutputWorkspace","LoadTest_Output");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( !loader.isExecuted() );
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
};

#endif /*LOADTEST_H_*/
