#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOADTEST_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOADTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/MuonLoad.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::WorkflowAlgorithms::MuonLoad;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class MuonLoadTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonLoadTest *createSuite() { return new MuonLoadTest(); }
  static void destroySuite( MuonLoadTest *suite ) { delete suite; }


  void test_Init()
  {
    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_simpleLoad()
  {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for ( int i = 1; i <= 16; ++i )
      group1.push_back(i);

    for ( int i = 17; i <= 32; ++i )
      group2.push_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);
  
    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DetectorGroupingTable", grouping) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "GroupCounts") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupIndex", 0) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", output.name()) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>( output.retrieve() );

    TS_ASSERT(ws);
    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 );
      TS_ASSERT_EQUALS( ws->blocksize(), 2000 );

      TS_ASSERT_EQUALS( ws->readY(0)[0], 461 ); 
      TS_ASSERT_EQUALS( ws->readY(0)[1000], 192 ); 
      TS_ASSERT_EQUALS( ws->readY(0)[1752], 5 ); 

      TS_ASSERT_DELTA( ws->readE(0)[0], 21.471, 0.001 ); 
      TS_ASSERT_DELTA( ws->readE(0)[1000], 13.856, 0.001 ); 
      TS_ASSERT_DELTA( ws->readE(0)[1752], 2.236, 0.001 ); 

      TS_ASSERT_DELTA( ws->readX(0)[0], -0.254, 0.001 ); 
      TS_ASSERT_DELTA( ws->readX(0)[1000], 15.746, 0.001 ); 
      TS_ASSERT_DELTA( ws->readX(0)[1752], 27.778, 0.001 ); 
    }
  }

  void test_multiPeriod()
  {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for ( int i = 33; i <= 64; ++i )
      group1.push_back(i);
    for ( int i = 1; i <= 32; ++i )
      group2.push_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);
  
    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "MUSR00015189.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FirstPeriod", 0) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SecondPeriod", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeriodOperation", "+") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DetectorGroupingTable", grouping) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "GroupCounts") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupIndex", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", output.name()) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>( output.retrieve() );

    TS_ASSERT(ws);
    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 );
      TS_ASSERT_EQUALS( ws->blocksize(), 2000 );

      TS_ASSERT_EQUALS( ws->readY(0)[0], 23); 
      TS_ASSERT_EQUALS( ws->readY(0)[1000], 3); 
      TS_ASSERT_EQUALS( ws->readY(0)[1701], 1); 

      TS_ASSERT_DELTA( ws->readE(0)[0], 4.796, 0.001 ); 
      TS_ASSERT_DELTA( ws->readE(0)[1000], 1.732, 0.001 ); 
      TS_ASSERT_DELTA( ws->readE(0)[1701], 1.000, 0.001 ); 

      TS_ASSERT_DELTA( ws->readX(0)[0], -0.550, 0.001 ); 
      TS_ASSERT_DELTA( ws->readX(0)[1000], 15.450, 0.001 ); 
      TS_ASSERT_DELTA( ws->readX(0)[1701], 26.666, 0.001 ); 
    }
  }

  TableWorkspace_sptr createGroupingTable(const std::vector<int>& group1, const std::vector<int>& group2)
  {
    auto t = boost::make_shared<TableWorkspace>();

    t->addColumn("vector_int", "Detectors");

    TableRow row1 = t->appendRow();
    row1 << group1;

    TableRow row2 = t->appendRow();
    row2 << group2;

    return t;
  }

};


#endif /* MANTID_WORKFLOWALGORITHMS_MUONLOADTEST_H_ */
