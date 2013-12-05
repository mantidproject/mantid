#ifndef MANTID_ALGORITHMS_MUONGROUPDETECTORSTEST_H_
#define MANTID_ALGORITHMS_MUONGROUPDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/MuonGroupDetectors.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::MuonGroupDetectors;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class MuonGroupDetectorsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonGroupDetectorsTest *createSuite() { return new MuonGroupDetectorsTest(); }
  static void destroySuite( MuonGroupDetectorsTest *suite ) { delete suite; }


  void test_Init()
  {
    MuonGroupDetectors alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    const std::string outWSName("MuonGroupDetectorsTest_OutputWS");

    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::Create2DWorkspace123(5,3);
    TableWorkspace_sptr grouping = createDetectorGroupingTable(); 

    MuonGroupDetectors alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DetectorGroupingTable", grouping) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);

    if ( ws )
    {
      TSM_ASSERT( "You forgot to check the results!", 0);
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

private:

  TableWorkspace_sptr createDetectorGroupingTable()
  {
    auto t = boost::make_shared<TableWorkspace>();

    t->addColumn("str", "ItemType");
    t->addColumn("str", "ItemName");
    t->addColumn("vector_int", "Elements");

    std::vector<int> group1;
    group1.push_back(0); group1.push_back(1);
    TableRow row1 = t->appendRow();
    row1 << "Group" << "1" << group1;

    std::vector<int> group2;
    group2.push_back(2); group2.push_back(2); group2.push_back(3);
    TableRow row2 = t->appendRow();
    row2 << "Group" << "2" << group2;

    // Just to make sure groups are used only
    std::vector<int> pair;
    pair.push_back(0); pair.push_back(1);
    TableRow row3 = t->appendRow();
    row3 << "Pair" << "ThePair" << pair;

    return t;
  }

};


#endif /* MANTID_ALGORITHMS_MUONGROUPDETECTORSTEST_H_ */
