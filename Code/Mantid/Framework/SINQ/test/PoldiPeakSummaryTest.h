#ifndef MANTID_SINQ_POLDIPEAKSUMMARYTEST_H_
#define MANTID_SINQ_POLDIPEAKSUMMARYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiPeakSummary.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class PoldiPeakSummaryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiPeakSummaryTest *createSuite() { return new PoldiPeakSummaryTest(); }
  static void destroySuite( PoldiPeakSummaryTest *suite ) { delete suite; }


  void test_Init()
  {
    PoldiPeakSummary alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("PoldiPeakSummaryTest_OutputWS");

    PoldiPeakSummary alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    TableWorkspace_sptr poldiPeaks = PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace();

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", poldiPeaks) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    /* Here we only check that there is a workspace. The content is determined by protected methods
     * which are tested separately.
     */

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void testGetInitializedResultWorkspace()
  {
      TestablePoldiPeakSummary alg;
      TableWorkspace_sptr table = alg.getInitializedResultWorkspace();

      TS_ASSERT_EQUALS(table->columnCount(), 6);
      TS_ASSERT_EQUALS(table->rowCount(), 0);
  }

  void testStorePeakSummary()
  {
      TestablePoldiPeakSummary alg;
      TableWorkspace_sptr table = alg.getInitializedResultWorkspace();

      PoldiPeak_sptr peak = PoldiPeak::create(MillerIndices(1, 2, 3),
                                              UncertainValue(1.2, 0.001),
                                              UncertainValue(100.0, 0.1),
                                              UncertainValue(0.01, 0.0001));

      TS_ASSERT_THROWS_NOTHING(alg.storePeakSummary(table->appendRow(), peak));

      TS_ASSERT_EQUALS(table->rowCount(), 1);
  }

  void testGetSummaryTable()
  {
      PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();

      TestablePoldiPeakSummary alg;
      TableWorkspace_sptr summary = alg.getSummaryTable(peaks);

      TS_ASSERT_EQUALS(summary->rowCount(), peaks->peakCount());
  }

private:
  class TestablePoldiPeakSummary : public PoldiPeakSummary
  {
      friend class PoldiPeakSummaryTest;
  };
};


#endif /* MANTID_SINQ_POLDIPEAKSUMMARYTEST_H_ */
