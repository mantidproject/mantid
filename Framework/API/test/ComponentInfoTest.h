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

  void test_read_positions_rotations() {

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

    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    positions->emplace_back(1, 0, 0);
    positions->emplace_back(1, 1, 0);

    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();
    rotations->emplace_back(Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));
    rotations->emplace_back(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));

    Mantid::Beamline::ComponentInfo internalInfo(bankSortedDetectorIndices,
                                                 ranges, positions, rotations);

    Mantid::Geometry::ObjComponent comp1("det1");
    Mantid::Geometry::ObjComponent comp2("det2");
    Mantid::Geometry::ObjComponent comp3("det3");
    Mantid::Geometry::ObjComponent assemb1("bank1");
    Mantid::Geometry::ObjComponent assemb2("instrument");
    ComponentInfo info(internalInfo,
                       std::vector<Mantid::Geometry::ComponentID>{
                           &comp1, &comp2, &comp3, &assemb1, &assemb2});
    /*
     * Remember. We have 3 detectors. So component index 3 corresponds to
     * position
     * index 0 since we don't input positions for detectors.
     */
    TS_ASSERT(info.position(3).isApprox(positions->at(0)));
    TS_ASSERT(info.position(4).isApprox(positions->at(1)));
    TS_ASSERT(info.rotation(3).isApprox(rotations->at(0)));
    TS_ASSERT(info.rotation(4).isApprox(rotations->at(1)));
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
