#ifndef MANTID_INDEXING_EXTRACTTEST_H_
#define MANTID_INDEXING_EXTRACTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid;
using namespace Indexing;

class ExtractTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractTest *createSuite() { return new ExtractTest(); }
  static void destroySuite(ExtractTest *suite) { delete suite; }

  void test_extract() {
    IndexInfo source({1, 2, 3});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(10);
    specDefs[1].add(20);
    specDefs[2].add(30);
    source.setSpectrumDefinitions(specDefs);
    std::vector<size_t> indices{{0, 2}};
    auto result = extract(source, indices);
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 3);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[0], specDefs[0]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[1], specDefs[2]);
  }

  void test_reorder() {
    IndexInfo source({1, 2, 3});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(10);
    specDefs[1].add(20);
    specDefs[2].add(30);
    source.setSpectrumDefinitions(specDefs);
    std::vector<size_t> indices{{2, 1, 0}};
    auto result = extract(source, indices);
    TS_ASSERT_EQUALS(result.size(), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[0], specDefs[2]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[1], specDefs[1]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[2], specDefs[0]);
  }
};

#endif /* MANTID_INDEXING_EXTRACTTEST_H_ */
