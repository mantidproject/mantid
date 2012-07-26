#ifndef MANTID_CRYSTAL_FILTERPEAKSTEST_H_
#define MANTID_CRYSTAL_FILTERPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCrystal/FilterPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"

using Mantid::Crystal::FilterPeaks;

class FilterPeaksTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterPeaksTest *createSuite() { return new FilterPeaksTest(); }
  static void destroySuite( FilterPeaksTest *suite ) { delete suite; }

  void test_Init()
  {
    FilterPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr inputWS = WorkspaceCreationHelper::createPeaksWorkspace();

    // Name of the output workspace.
    std::string outWSName("FilterPeaksTest_OutputWS");
  
    FilterPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWS) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("FilterVariable", "H+K+L") )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FilterValue", 0.0) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Operator", ">") )
    TS_ASSERT( alg.execute() )
    
    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // Will be empty as indices not set
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 0 )
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

};


#endif /* MANTID_CRYSTAL_FILTERPEAKSTEST_H_ */
