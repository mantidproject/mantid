#ifndef MANTID_API_SPECTRUMDETECTORMAPPINGTEST_H_
#define MANTID_API_SPECTRUMDETECTORMAPPINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::SpectrumDetectorMapping;
using Mantid::API::MatrixWorkspace_sptr;

class SpectrumDetectorMappingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumDetectorMappingTest *createSuite() { return new SpectrumDetectorMappingTest(); }
  static void destroySuite( SpectrumDetectorMappingTest *suite ) { delete suite; }

  void test_workspace_constructor_null_pointer()
  {
    TS_ASSERT_THROWS( SpectrumDetectorMapping(NULL), std::invalid_argument );
  }

  void test_workspace_constructor_fills_map()
  {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->init(3,1,1);
    // Override some of the default detector numbers to make it more interesting
    ws->getSpectrum(0)->setDetectorIDs(std::set<detid_t>());
    int detids[] = {10,20};
    ws->getSpectrum(2)->setDetectorIDs(std::set<detid_t>(detids,detids+2));
    SpectrumDetectorMapping map(ws.get());

    TS_ASSERT( map.getDetectorIDsForSpectrumNo(1).empty() );
    auto idsFor2 = map.getDetectorIDsForSpectrumNo(2);
    TS_ASSERT_EQUALS( idsFor2.size(), 1 );
    TS_ASSERT_DIFFERS( idsFor2.find(1), idsFor2.end() );
    auto idsFor3 = map.getDetectorIDsForSpectrumNo(3);
    TS_ASSERT_EQUALS( idsFor3.size(), 2 );
    TS_ASSERT_DIFFERS( idsFor3.find(10), idsFor3.end() );
    TS_ASSERT_DIFFERS( idsFor3.find(20), idsFor3.end() );
  }

  void test_getDetectorIDsForSpectrumNo()
  {
    auto ws = boost::make_shared<WorkspaceTester>();
    SpectrumDetectorMapping map(ws.get());
    // The happy path is tested in the methods above. Just test invalid entry here.
    TS_ASSERT_THROWS( map.getDetectorIDsForSpectrumNo(1), std::out_of_range );
    TS_ASSERT_THROWS( map.getDetectorIDsForSpectrumNo(0), std::out_of_range );
    TS_ASSERT_THROWS( map.getDetectorIDsForSpectrumNo(-1), std::out_of_range );
  }

};


#endif /* MANTID_API_SPECTRUMDETECTORMAPPINGTEST_H_ */
