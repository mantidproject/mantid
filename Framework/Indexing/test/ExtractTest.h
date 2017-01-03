#ifndef MANTID_INDEXING_EXTRACTTEST_H_
#define MANTID_INDEXING_EXTRACTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"

using namespace Mantid;
using namespace Indexing;

class ExtractTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractTest *createSuite() { return new ExtractTest(); }
  static void destroySuite(ExtractTest *suite) { delete suite; }

  void test_extract() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<size_t> indices{{0, 2}};
    auto result = extract(source, indices);
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 3);
    TS_ASSERT_EQUALS(result.detectorIDs(0), std::vector<detid_t>{10});
    TS_ASSERT_EQUALS(result.detectorIDs(1), std::vector<detid_t>{30});
  }

  void test_reorder() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<size_t> indices{{2, 1, 0}};
    auto result = extract(source, indices);
    TS_ASSERT_EQUALS(result.size(), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(result.detectorIDs(0), std::vector<detid_t>{30});
    TS_ASSERT_EQUALS(result.detectorIDs(1), std::vector<detid_t>{20});
    TS_ASSERT_EQUALS(result.detectorIDs(2), std::vector<detid_t>{10});
  }
};

#endif /* MANTID_INDEXING_EXTRACTTEST_H_ */
