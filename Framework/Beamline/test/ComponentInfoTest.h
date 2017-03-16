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
    std::vector<std::vector<size_t>> indexes{{1}, {}};
    ComponentInfo info(indexes);
    TS_ASSERT_EQUALS(info.size(), 2);
  }

  void test_detector_indexes() {

    /*
           |
     ------------
     |         | 30
    -------
    | 10  | 20
    */

    std::vector<std::vector<size_t>> indexes{
        {10, 20, 30}, {10, 20}, {}, {}, {}};
    ComponentInfo info(indexes);
    TS_ASSERT_EQUALS(info.detectorIndexes(0 /*component index*/),
                     std::vector<size_t>({10, 20, 30}));
    TS_ASSERT_EQUALS(info.detectorIndexes(1 /*component index*/),
                     std::vector<size_t>({10, 20}));
  }
};
#endif /* MANTID_BEAMLINE_COMPONENTINFOTEST_H_ */
