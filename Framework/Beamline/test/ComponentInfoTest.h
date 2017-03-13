#ifndef MANTID_BEAMLINE_COMPONENTINFOTEST_H_
#define MANTID_BEAMLINE_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"

using Mantid::Beamline::ComponentInfo;

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_size() {
    std::vector<std::vector<size_t>> indexes{{0, 1}, {2, 3, 4}};
    ComponentInfo info(indexes);
    TS_ASSERT_EQUALS(info.size(), 2);
  }

  void test_detector_indexes() {
    std::vector<std::vector<size_t>> indexes{{0, 1}, {2, 3, 4}};
    ComponentInfo info(indexes);
    TS_ASSERT_EQUALS(info.detectorIndexes(0), std::vector<size_t>({0, 1}));
    TS_ASSERT_EQUALS(info.detectorIndexes(1), std::vector<size_t>({2, 3, 4}));
  }
};
#endif /* MANTID_BEAMLINE_COMPONENTINFOTEST_H_ */
