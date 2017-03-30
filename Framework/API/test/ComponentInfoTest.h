#ifndef MANTID_API_COMPONENTINFOTEST_H_
#define MANTID_API_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
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

    auto detectorInfo = boost::make_shared<Mantid::Beamline::DetectorInfo>();
    std::vector<size_t> detectorIndices{}; // No detectors in this example

    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges.push_back(
        std::make_pair(0, 0)); // Another component with no detectors

    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);

    Mantid::Beamline::ComponentInfo internalInfo(
        detectorIndices, ranges, positions, rotations, detectorInfo);
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

    std::vector<Eigen::Vector3d> detPositions;
    detPositions.emplace_back(1, 0, 0);
    detPositions.emplace_back(2, 0, 0);
    detPositions.emplace_back(3, 0, 0);
    std::vector<Eigen::Quaterniond> detRotations;
    detRotations.emplace_back(
        Eigen::AngleAxisd(M_PI / 5, Eigen::Vector3d::UnitZ()));
    detRotations.emplace_back(
        Eigen::AngleAxisd(M_PI / 4, Eigen::Vector3d::UnitZ()));
    detRotations.emplace_back(
        Eigen::AngleAxisd(M_PI / 3, Eigen::Vector3d::UnitZ()));
    auto detectorInfo = boost::make_shared<Mantid::Beamline::DetectorInfo>(
        detPositions, detRotations);

    std::vector<size_t> bankSortedDetectorIndices{0, 2, 1};

    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(0, 2));

    auto compPositions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    compPositions->emplace_back(4, 0, 0);
    compPositions->emplace_back(5, 0, 0);

    auto compRotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();
    compRotations->emplace_back(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
    compRotations->emplace_back(
        Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));

    Mantid::Beamline::ComponentInfo internalInfo(bankSortedDetectorIndices,
                                                 ranges, compPositions,
                                                 compRotations, detectorInfo);

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
    TS_ASSERT(info.position(3).isApprox(compPositions->at(0)));
    TS_ASSERT(info.position(4).isApprox(compPositions->at(1)));
    TS_ASSERT(info.rotation(3).isApprox(compRotations->at(0)));
    TS_ASSERT(info.rotation(4).isApprox(compRotations->at(1)));

    TS_ASSERT(info.position(0).isApprox(detPositions.at(0)));
    TS_ASSERT(info.position(1).isApprox(detPositions.at(1)));
    TS_ASSERT(info.position(2).isApprox(detPositions.at(2)));
    TS_ASSERT(info.rotation(0).isApprox(detRotations.at(0)));
    TS_ASSERT(info.rotation(1).isApprox(detRotations.at(1)));
    TS_ASSERT(info.rotation(2).isApprox(detRotations.at(2)));
  }

  void test_indexOf() {

    std::vector<size_t> detectorIndices{}; // No detectors in this example
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges.push_back(
        std::make_pair(0, 0)); // Another component with no detectors
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);
    Mantid::Beamline::ComponentInfo internalInfo(
        detectorIndices, ranges, positions, rotations,
        boost::make_shared<Mantid::Beamline::DetectorInfo>());
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

    auto detectorInfo = boost::make_shared<Mantid::Beamline::DetectorInfo>(
        std::vector<Eigen::Vector3d>(3), std::vector<Eigen::Quaterniond>(3));

    std::vector<size_t> detectorIndices{0, 2, 1};
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(2, 3));
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);
    Mantid::Beamline::ComponentInfo internalInfo(
        detectorIndices, ranges, positions, rotations, detectorInfo);
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
