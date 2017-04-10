#ifndef MANTID_BEAMLINE_COMPONENTINFOTEST_H_
#define MANTID_BEAMLINE_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Beamline;

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_size() {
    std::vector<size_t> bankSortedDetectorIndices{0, 1, 2};
    std::vector<std::pair<size_t, size_t>> ranges;
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();

    ComponentInfo info(
        bankSortedDetectorIndices, ranges, positions, rotations,
        boost::make_shared<DetectorInfo>(std::vector<Eigen::Vector3d>(3),
                                         std::vector<Eigen::Quaterniond>(3)));
    TS_ASSERT_EQUALS(info.size(), 3);
  }

  void
  test_constructor_throws_if_size_mismatch_between_detector_indices_and_detectorinfo() {
    std::vector<size_t> bankSortedDetectorIndices{0, 1,
                                                  2}; // 3 detector indices
    std::vector<std::pair<size_t, size_t>> ranges;
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();

    TS_ASSERT_THROWS(
        ComponentInfo(
            bankSortedDetectorIndices, ranges, positions, rotations,
            boost::make_shared<DetectorInfo>() /*Detector info size 0*/),
        std::invalid_argument &);
  }

  void test_throw_if_positions_rotation_inputs_different_sizes() {
    std::vector<size_t> detectorIndices{}; // No detectors in this example
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0)); // One component with no detectors
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        1); // 1 position provided
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        0); // 0 rotations provided

    TS_ASSERT_THROWS(ComponentInfo(detectorIndices, ranges, positions,
                                   rotations,
                                   boost::make_shared<DetectorInfo>()),
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

    TS_ASSERT_THROWS(ComponentInfo(detectorIndices, ranges, positions,
                                   rotations,
                                   boost::make_shared<DetectorInfo>()),
                     std::invalid_argument &);
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
    auto detectorInfo =
        boost::make_shared<DetectorInfo>(detPositions, detRotations);
    std::vector<size_t> bankSortedDetectorIndices{0, 2, 1};

    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(0, 2));

    auto compPositions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    compPositions->emplace_back(1, 0, 0);
    compPositions->emplace_back(1, 1, 0);

    auto compRotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();
    compRotations->emplace_back(
        Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));
    compRotations->emplace_back(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));

    ComponentInfo info(bankSortedDetectorIndices, ranges, compPositions,
                       compRotations, detectorInfo);

    /*
     * Remember. We have 3 detectors. So component index 3 corresponds to
     * position
     * index 0 since we don't input positions for detectors via ComponentInfo
     * constructor.
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

  void set_positions_rotations() {

    /*
           |
     ------------
     |
     0
    */

    std::vector<Eigen::Vector3d> detPositions;
    detPositions.emplace_back(1, 0, 0);
    std::vector<Eigen::Quaterniond> detRotations;
    detRotations.emplace_back(
        Eigen::AngleAxisd(M_PI / 5, Eigen::Vector3d::UnitZ()));
    auto detectorInfo =
        boost::make_shared<DetectorInfo>(detPositions, detRotations);
    std::vector<size_t> bankSortedDetectorIndices{0};

    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 0));

    auto compPositions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    compPositions->emplace_back(2, 0, 0);

    auto compRotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();
    compRotations->emplace_back(
        Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));

    ComponentInfo info(bankSortedDetectorIndices, ranges, compPositions,
                       compRotations, detectorInfo);

    /*
     * Remember. We have 1 detectors. So component index 1 corresponds to
     * position
     * index 0 since we don't input positions for detectors via ComponentInfo
     * constructor.
     */
    TS_ASSERT(info.position(1).isApprox(compPositions->at(0)));
    TS_ASSERT(info.rotation(1).isApprox(compRotations->at(0)));

    TS_ASSERT(info.position(0).isApprox(detPositions.at(0)));
    TS_ASSERT(info.rotation(0).isApprox(detRotations.at(0)));

    Eigen::Vector3d newDetectorLocation{2, 0, 0};
    Eigen::Quaterniond newDetectorRotation(
        Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));
    Eigen::Vector3d newComponentLocation{3, 0, 0};
    Eigen::Quaterniond newComponentRotation(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));

    info.setPosition(0, newDetectorLocation);
    info.setRotation(0, newDetectorRotation);
    info.setPosition(1, newComponentLocation);
    info.setRotation(1, newComponentRotation);

    // Check that we get what we set
    TS_ASSERT(info.position(0).isApprox(newDetectorLocation));
    TS_ASSERT(info.rotation(0).isApprox(newDetectorRotation));
    TS_ASSERT(info.position(1).isApprox(newComponentLocation));
    TS_ASSERT(info.rotation(1).isApprox(newComponentRotation));
  }

  void test_detector_indexes() {

    /*
           |
     ------------
     |         | 1
    -------
    | 0  | 2
    */

    std::vector<Eigen::Vector3d> detPositions(3);
    std::vector<Eigen::Quaterniond> detRotations(3);
    std::vector<size_t> bankSortedDetectorIndices{0, 2, 1};
    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.push_back(std::make_pair(0, 3));
    ranges.push_back(std::make_pair(0, 2));
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        2); // 2 positions provided. 2 non-detectors
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        2); // 2 rotations provided. 2 non-detectors
    ComponentInfo info(
        bankSortedDetectorIndices, ranges, positions, rotations,
        boost::make_shared<DetectorInfo>(detPositions, detRotations));

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
