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

  void test_size() {
    std::vector<size_t> bankSortedDetectorIndices{0, 1, 2};
    std::vector<std::pair<size_t, size_t>> ranges;
    ComponentInfo info(bankSortedDetectorIndices, ranges);
    TS_ASSERT_EQUALS(info.size(), 3);
  }

  void test_detector_indexes() {

    /*
           |
     ------------
     |         | 1
    -------
    | 0  | 2
    */
    std::vector<size_t> bankSortedDetectorIndices{0, 2, 1};
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(0, 2));
    ComponentInfo info(bankSortedDetectorIndices, ranges);

    /*
    Note that detectors are always the first n component indexes!
    */
    TS_ASSERT_EQUALS(info.detectorIndices(0), std::vector<size_t>{0});
    TS_ASSERT_EQUALS(info.detectorIndices(1), std::vector<size_t>{1});
    TS_ASSERT_EQUALS(info.detectorIndices(2), std::vector<size_t>{2});

    // Now we have non-detector components
    TS_ASSERT_EQUALS(info.detectorIndices(bankSortedDetectorIndices.size() +
                                          0 /*component index*/),
                     std::vector<size_t>({0, 2, 1}));
    TS_ASSERT_EQUALS(info.detectorIndices(bankSortedDetectorIndices.size() +
                                          1 /*component index*/),
                     std::vector<size_t>({0, 2}));
  }
};
#endif /* MANTID_BEAMLINE_COMPONENTINFOTEST_H_ */
