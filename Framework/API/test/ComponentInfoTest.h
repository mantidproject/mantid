#ifndef MANTID_API_COMPONENTINFOTEST_H_
#define MANTID_API_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

using Mantid::API::ComponentInfo;

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_throw_if_positions_rotation_inputs_different_sizes() {
    std::vector<size_t> detectorIndices{}; // No detectors in this example
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0)); // One component with no detectors
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        1); // 1 position provided
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        0); // 0 rotations provided

    TS_ASSERT_THROWS(Mantid::Beamline::ComponentInfo(detectorIndices, ranges,
                                                     positions, rotations),
                     std::invalid_argument &);
  }

  void test_throw_if_positions_and_rotations_not_same_size_as_ranges() {
    /*
     * Positions are rotations are only currently stored for non-detector
     * components
     * We should have as many ranges as we have non-detector components too.
     * All vectors should be the same size.
     */
    std::vector<size_t> detectorIndices{}; // No detectors in this example
    std::vector<std::pair<size_t, size_t>> ranges; // Empty ranges!
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        1); // 1 position provided
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        1); // 1 rotation provided

    TS_ASSERT_THROWS(Mantid::Beamline::ComponentInfo(detectorIndices, ranges,
                                                     positions, rotations),
                     std::invalid_argument &);
  }

  void test_size() {

    std::vector<size_t> detectorIndices{}; // No detectors in this example
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges.push_back(
        std::make_pair(0, 0)); // Another component with no detectors

    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);

    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges,
                                                 positions, rotations);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");
    ComponentInfo info(internalInfo, std::vector<Mantid::Geometry::ComponentID>{
                                         &comp1, &comp2});
    TS_ASSERT_EQUALS(info.size(), 2);
  }

  void test_indexOf() {

    std::vector<size_t> detectorIndices{}; // No detectors in this example
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges.push_back(
        std::make_pair(0, 0)); // Another component with no detectors
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges,
                                                 positions, rotations);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");

    ComponentInfo info(internalInfo, std::vector<Mantid::Geometry::ComponentID>{
                                         &comp1, &comp2});
    TS_ASSERT_EQUALS(info.indexOf(comp1.getComponentID()), 0);
    TS_ASSERT_EQUALS(info.indexOf(comp2.getComponentID()), 1);
  }

  void test_detector_indices() {

    /*
     Detectors marked with their indices
           |
     ------------
     |         | 1
    -------
    | 0  | 2
    */

    std::vector<size_t> detectorIndices{0, 2, 1};
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(2, 3));
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges,
                                                 positions, rotations);
    Mantid::Geometry::ObjComponent fakeComposite1("fakeComp1");
    Mantid::Geometry::ObjComponent fakeComposite2("fakeComp2");
    Mantid::Geometry::ObjComponent fakeDetector1("fakeDetector1");
    Mantid::Geometry::ObjComponent fakeDetector2("fakeDetector2");
    Mantid::Geometry::ObjComponent fakeDetector3("fakeDetector3");

    ComponentInfo info(internalInfo,
                       std::vector<Mantid::Geometry::ComponentID>{
                           &fakeComposite1, &fakeComposite2, &fakeDetector1,
                           &fakeDetector2, &fakeDetector3});
    TS_ASSERT_EQUALS(info.detectorIndices(3 /*component index*/),
                     std::vector<size_t>({0, 2, 1}));
    TS_ASSERT_EQUALS(info.detectorIndices(4 /*component index*/),
                     std::vector<size_t>({1}));
  }
};

#endif /* MANTID_API_COMPONENTINFOTEST_H_ */
