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

  void test_empty_size() {
    ComponentInfo info;
    TS_ASSERT_EQUALS(info.size(), 0);
  }

  void test_empty_throw_out_of_range() {
    ComponentInfo info;
    TSM_ASSERT_THROWS(
        "Empty ComponenetInfo, so any attempt to use this should throw",
        info.detectorIndices(0), std::out_of_range &);
  }

  void test_size() {
    std::vector<size_t> detectorIndices{1, 2, 3};
    std::vector<std::pair<size_t, size_t>> ranges;
    ComponentInfo info(detectorIndices, ranges);
    TS_ASSERT_EQUALS(info.size(), 3);
  }

  void test_detector_indexes() {

    /*
           |
     ------------
     |         | 30
    -------
    | 10  | 20
    */
    std::vector<size_t> detectorIndices{10, 20, 30};
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(0, 2));
    ComponentInfo info(detectorIndices, ranges);

    /*
    Note that detectors are always the first n component indexes!
    */
    TS_ASSERT_EQUALS(info.detectorIndices(0), std::vector<size_t>{10});
    TS_ASSERT_EQUALS(info.detectorIndices(1), std::vector<size_t>{20});
    TS_ASSERT_EQUALS(info.detectorIndices(2), std::vector<size_t>{30});

    // Now we have non-detector components
    TS_ASSERT_EQUALS(
        info.detectorIndices(detectorIndices.size() + 0 /*component index*/),
        std::vector<size_t>({10, 20, 30}));
    TS_ASSERT_EQUALS(
        info.detectorIndices(detectorIndices.size() + 1 /*component index*/),
        std::vector<size_t>({10, 20}));
  }

  void test_out_of_range_index_throws() {

    std::vector<size_t> detectorIndices{1, 2, 3};
    std::vector<std::pair<size_t, size_t>> ranges;
    ComponentInfo info(detectorIndices, ranges);
    TSM_ASSERT_THROWS_NOTHING("This should not throw. Valid index.",
                              info.detectorIndices(0));
    TSM_ASSERT_THROWS("This should throw when index is out of range",
                      info.detectorIndices(info.size()), std::out_of_range &);
  }
};
#endif /* MANTID_BEAMLINE_COMPONENTINFOTEST_H_ */
