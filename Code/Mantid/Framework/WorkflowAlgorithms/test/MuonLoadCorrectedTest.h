#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTEDTEST_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTEDTEST_H_

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/SaveNexus.h"
#include "MantidWorkflowAlgorithms/MuonLoadCorrected.h"

using Mantid::WorkflowAlgorithms::MuonLoadCorrected;

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class MuonLoadCorrectedTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonLoadCorrectedTest *createSuite() { return new MuonLoadCorrectedTest(); }
  static void destroySuite( MuonLoadCorrectedTest *suite ) { delete suite; }

  // Name of the output workspace.
  const std::string g_outWSName; 

  MuonLoadCorrectedTest() : g_outWSName("MuonLoadCorrectedTest_OutputWS")
  {}

  ~MuonLoadCorrectedTest() 
  {
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();

    // Remove workspace from the data service if exists
    if ( ads.doesExist(g_outWSName) )
    {
      if ( ads.retrieveWS<WorkspaceGroup>(g_outWSName) )
        ads.deepRemoveGroup(g_outWSName);
      else
        ads.remove(g_outWSName);
    }
  }

  void test_init()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_properties()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT( alg.existsProperty("Filename") )
    TS_ASSERT( alg.existsProperty("DTCType") )
    TS_ASSERT( alg.existsProperty("DTCFile") )
    TS_ASSERT( alg.existsProperty("OutputWorkspace") )
  }
  
  void test_singlePeriod_noCorrection()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DTCType", "None") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", g_outWSName) );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(g_outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    TS_ASSERT_EQUALS( ws->blocksize(), 2000 );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 32 );

    TS_ASSERT_EQUALS( ws->readY(0)[0], 52);
    TS_ASSERT_EQUALS( ws->readY(7)[500], 166);
    TS_ASSERT_EQUALS( ws->readY(15)[1000], 7);
    TS_ASSERT_EQUALS( ws->readY(20)[1500], 1);
    TS_ASSERT_EQUALS( ws->readY(31)[1999], 0);
 
    TS_ASSERT_DELTA( ws->readX(0)[0], -0.254, 0.001 );
    TS_ASSERT_DELTA( ws->readX(15)[1000], 15.746, 0.001 );
    TS_ASSERT_DELTA( ws->readX(31)[2000], 31.741, 0.001 );

    TS_ASSERT_DELTA( ws->readE(0)[0], 7.211, 0.001 );
    TS_ASSERT_DELTA( ws->readE(15)[1000], 2.646, 0.001 );
    TS_ASSERT_DELTA( ws->readE(31)[1999], 0, 0.001);
  }

  void test_singlePeriod_fromData()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DTCType", "FromData") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", g_outWSName) );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(g_outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    TS_ASSERT_EQUALS( ws->blocksize(), 2000 );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 32 );

    TS_ASSERT_DELTA( ws->readY(0)[0], 52.0007, 0.0001);
    TS_ASSERT_DELTA( ws->readY(7)[500], 166.017, 0.001);
    TS_ASSERT_DELTA( ws->readY(15)[1000], 6.99998, 0.00001);
    TS_ASSERT_DELTA( ws->readY(20)[1500], 1.000002, 0.000001);
    TS_ASSERT_EQUALS( ws->readY(31)[1999], 0);
 
    TS_ASSERT_DELTA( ws->readX(0)[0], -0.254, 0.001 );
    TS_ASSERT_DELTA( ws->readX(15)[1000], 15.746, 0.001 );
    TS_ASSERT_DELTA( ws->readX(31)[2000], 31.741, 0.001 );

    TS_ASSERT_DELTA( ws->readE(0)[0], 7.211, 0.001 );
    TS_ASSERT_DELTA( ws->readE(15)[1000], 2.646, 0.001 );
    TS_ASSERT_DELTA( ws->readE(31)[1999], 0, 0.001);
  }

  void test_singlePeriod_fromSpecifiedFile()
  {
    const std::string filename = "TestDeadTimeFile.nxs";

    createDeadTimesTableFile(filename, 0.15);

    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DTCType", "FromSpecifiedFile") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DTCFile", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", g_outWSName) );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(g_outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    TS_ASSERT_EQUALS( ws->blocksize(), 2000 );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 32 );

    TS_ASSERT_DELTA( ws->readY(0)[0], 52.0608, 0.0001);
    TS_ASSERT_DELTA( ws->readY(7)[500], 166.6211, 0.0001);
    TS_ASSERT_DELTA( ws->readY(15)[1000], 7.0011, 0.0001);
    TS_ASSERT_DELTA( ws->readY(20)[1500], 1.000022, 0.000001);
    TS_ASSERT_EQUALS( ws->readY(31)[1999], 0);
 
    TS_ASSERT_DELTA( ws->readX(0)[0], -0.254, 0.001 );
    TS_ASSERT_DELTA( ws->readX(15)[1000], 15.746, 0.001 );
    TS_ASSERT_DELTA( ws->readX(31)[2000], 31.741, 0.001 );

    TS_ASSERT_DELTA( ws->readE(0)[0], 7.211, 0.001 );
    TS_ASSERT_DELTA( ws->readE(15)[1000], 2.646, 0.001 );
    TS_ASSERT_DELTA( ws->readE(31)[1999], 0, 0.001);

    Poco::File(filename).remove();
  }

  void test_multiPeriod()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "MUSR00015189.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DTCType", "FromData") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", g_outWSName) );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    WorkspaceGroup_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(g_outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    TS_ASSERT_EQUALS( ws->size(), 2 );
   
    MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>( ws->getItem(0) );
    TS_ASSERT( ws1 );
    if (!ws1) return;

    TS_ASSERT_EQUALS( ws1->blocksize(), 2000 );
    TS_ASSERT_EQUALS( ws1->getNumberHistograms(), 64 );

    TS_ASSERT_DELTA( ws1->readY(0)[0], 4.0025, 0.0001);
    TS_ASSERT_DELTA( ws1->readY(16)[500], 6.006469, 0.000001);
    TS_ASSERT_DELTA( ws1->readY(31)[999], 1.000237, 0.000001) ;
    TS_ASSERT_DELTA( ws1->readY(47)[1503], 1.000128, 0.000001);
    TS_ASSERT_DELTA( ws1->readY(63)[1999], 0.0, 0.1);
 
    MatrixWorkspace_sptr ws2 = boost::dynamic_pointer_cast<MatrixWorkspace>( ws->getItem(1) );
    TS_ASSERT( ws2 );
    if (!ws2) return;

    TS_ASSERT_EQUALS( ws2->blocksize(), 2000 );
    TS_ASSERT_EQUALS( ws2->getNumberHistograms(), 64 );

    TS_ASSERT_DELTA( ws2->readY(0)[0], 12.0232, 0.0001);
    TS_ASSERT_DELTA( ws2->readY(17)[498], 7.0084 , 0.0001);
    TS_ASSERT_DELTA( ws2->readY(31)[997], 1.000237, 0.000001) ;
    TS_ASSERT_DELTA( ws2->readY(47)[1250], 1.000128, 0.000001);
    TS_ASSERT_DELTA( ws2->readY(63)[1999], 0.0, 0.1);
  }

  void createDeadTimesTableFile(const std::string& filename, double value)
  {
    ITableWorkspace_sptr deadTimeTable = WorkspaceFactory::Instance().createTable("TableWorkspace");
    deadTimeTable->addColumn("int","spectrum");
    deadTimeTable->addColumn("double","dead-time");

    for(int i = 0; i < 32; i++)
    {
      TableRow row = deadTimeTable->appendRow();
      row << (i+1) << value;
    }

    SaveNexus saveNexusAlg;
    TS_ASSERT_THROWS_NOTHING(saveNexusAlg.initialize());
    saveNexusAlg.setProperty<ITableWorkspace_sptr>("InputWorkspace", deadTimeTable);
    saveNexusAlg.setPropertyValue("Filename", filename);
    TS_ASSERT_THROWS_NOTHING(saveNexusAlg.execute());
    TS_ASSERT(saveNexusAlg.isExecuted());
  }
};


#endif /* MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTEDTEST_H_ */
