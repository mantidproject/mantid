#ifndef MANTID_BEAMLINE_COMPONENTINFOTEST_H_
#define MANTID_BEAMLINE_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include <boost/make_shared.hpp>
#include <tuple>

using namespace Mantid::Beamline;

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_size() {
    /*
     Imitate an instrument with 3 detectors and nothing more.
    */
    auto bankSortedDetectorIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 1, 2});
    auto bankSortedComponentIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 1, 2});
    auto detectorRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>();
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::vector<std::pair<size_t, size_t>>{});
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();

    ComponentInfo info(
        bankSortedDetectorIndices, detectorRanges, bankSortedComponentIndices,
        componentRanges, positions, rotations,
        boost::make_shared<DetectorInfo>(std::vector<Eigen::Vector3d>(3),
                                         std::vector<Eigen::Quaterniond>(3)));
    TS_ASSERT_EQUALS(info.size(), 3);
  }

  void
  test_constructor_throws_if_size_mismatch_between_detector_indices_and_detectorinfo() {
    /*
     Imitate an instrument with 3 detectors and nothing more.
    */
    auto bankSortedDetectorIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 1, 2});
    auto bankSortedComponentIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 1, 2});
    auto detectorRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>();
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::vector<std::pair<size_t, size_t>>{});
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>();
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();

    TS_ASSERT_THROWS(
        ComponentInfo(
            bankSortedDetectorIndices, detectorRanges,
            bankSortedComponentIndices, componentRanges, positions, rotations,
            boost::make_shared<DetectorInfo>() /*Detector info size 0*/),
        std::invalid_argument &);
  }

  void test_throw_if_positions_rotation_inputs_different_sizes() {
    auto detectorIndices = boost::make_shared<
        const std::vector<size_t>>(); // No detector indices in this example!

    auto bankSortedComponentIndices =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{0});
    auto innerDetectorRanges = std::vector<std::pair<size_t, size_t>>{
        {0, 0}}; // One component with no detectors
    auto detectorRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::move(innerDetectorRanges));

    auto innerComponentRanges = std::vector<std::pair<size_t, size_t>>{
        {0, 0}}; // One component with no sub-components
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::move(innerComponentRanges));
    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        1); // 1 position provided
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        0); // 0 rotations provided

    TS_ASSERT_THROWS(ComponentInfo(detectorIndices, detectorRanges,
                                   bankSortedComponentIndices, componentRanges,
                                   positions, rotations,
                                   boost::make_shared<DetectorInfo>()),
                     std::invalid_argument &);
  }

  void test_throw_if_positions_and_rotations_not_same_size_as_detectorRanges() {
    /*
     * Positions are rotations are only currently stored for non-detector
     * components
     * We should have as many detectorRanges as we have non-detector components
     * too.
     * All vectors should be the same size.
     */
    auto detectorIndices = boost::make_shared<
        const std::vector<size_t>>(); // No detector indices in this example!

    auto componentIndices =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{0});

    auto detectorRanges = boost::make_shared<
        const std::vector<std::pair<size_t, size_t>>>(); // Empty detectorRanges

    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        1); // 1 position provided
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        1); // 1 rotation provided

    // Only one component. So single empty component range.
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::vector<std::pair<size_t, size_t>>{{0, 0}});

    TS_ASSERT_THROWS(ComponentInfo(detectorIndices, detectorRanges,
                                   componentIndices, componentRanges, positions,
                                   rotations,
                                   boost::make_shared<DetectorInfo>()),
                     std::invalid_argument &);
  }

  void test_read_positions_rotations() {

    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo info = std::get<0>(allOutputs);
    // Arguments to ComponentInfo for geometric aspects
    std::vector<Eigen::Vector3d> detPositions = std::get<1>(allOutputs);
    std::vector<Eigen::Quaterniond> detRotations= std::get<2>(allOutputs);
    std::vector<Eigen::Vector3d> compPositions= std::get<3>(allOutputs);
    std::vector<Eigen::Quaterniond> compRotations= std::get<4>(allOutputs);

    /*
     * Remember. We have 3 detectors. So component index 3 corresponds to
     * position index 0 in the input vector since we don't input positions for detectors via ComponentInfo
     * constructor.
     */
    TS_ASSERT(info.position(3).isApprox(compPositions.at(0)));
    TS_ASSERT(info.position(4).isApprox(compPositions.at(1)));
    TS_ASSERT(info.rotation(3).isApprox(compRotations.at(0)));
    TS_ASSERT(info.rotation(4).isApprox(compRotations.at(1)));

    TS_ASSERT(info.position(0).isApprox(detPositions.at(0)));
    TS_ASSERT(info.position(1).isApprox(detPositions.at(1)));
    TS_ASSERT(info.position(2).isApprox(detPositions.at(2)));
    TS_ASSERT(info.rotation(0).isApprox(detRotations.at(0)));
    TS_ASSERT(info.rotation(1).isApprox(detRotations.at(1)));
    TS_ASSERT(info.rotation(2).isApprox(detRotations.at(2)));
  }

  void test_write_positions() {

    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo info = std::get<0>(allOutputs);
    // Arguments to ComponentInfo for geometric aspects
    std::vector<Eigen::Vector3d> originalDetPositions = std::get<1>(allOutputs);
    std::vector<Eigen::Quaterniond> originalDetRotations= std::get<2>(allOutputs);
    std::vector<Eigen::Vector3d> originalCompPositions= std::get<3>(allOutputs);
    std::vector<Eigen::Quaterniond> originalCompRotations= std::get<4>(allOutputs);

    // Change the position of the root.

    Eigen::Vector3d rootDestination{60, 0, 0};
    const size_t rootIndex = 4;

    const auto rootOriginalPosition = info.position(rootIndex);
    info.setPosition(rootIndex, rootDestination);
    TS_ASSERT(info.position(rootIndex).isApprox(rootDestination));

    const auto offset = rootDestination - rootOriginalPosition;
    /*
     * Remember. We have 3 detectors. So component index 3 corresponds to
     * position
     * index 0 since we don't input positions for detectors via ComponentInfo
     * constructor.
     */
    TS_ASSERT(info.position(3).isApprox(originalCompPositions.at(0) + offset));
    TS_ASSERT(info.position(4).isApprox(originalCompPositions.at(1) + offset));
    TS_ASSERT(info.rotation(3).isApprox(originalCompRotations.at(0)));
    TS_ASSERT(info.rotation(4).isApprox(originalCompRotations.at(1)));

    TS_ASSERT(info.position(0).isApprox(originalDetPositions.at(0) + offset));
    TS_ASSERT(info.position(1).isApprox(originalDetPositions.at(1) + offset));
    TS_ASSERT(info.position(2).isApprox(originalDetPositions.at(2) + offset));
    TS_ASSERT(info.rotation(0).isApprox(originalDetRotations.at(0)));
    TS_ASSERT(info.rotation(1).isApprox(originalDetRotations.at(1)));
    TS_ASSERT(info.rotation(2).isApprox(originalDetRotations.at(2)));
  }

  std::tuple<ComponentInfo, std::vector<Eigen::Vector3d>,
             std::vector<Eigen::Quaterniond>, std::vector<Eigen::Vector3d>,
             std::vector<Eigen::Quaterniond>>
  makeTreeExampleAndReturnGeometricArguments() {

      /*
             |
       ------------
       |         | 1
      -------
      | 0  | 2
      */

      // Set detectors at different positions
      std::vector<Eigen::Vector3d> detPositions;
      detPositions.emplace_back(1, 0, 0);
      detPositions.emplace_back(2, 0, 0);
      detPositions.emplace_back(3, 0, 0);
      // Set detectors at different rotations
      std::vector<Eigen::Quaterniond> detRotations;
      detRotations.emplace_back(
          Eigen::AngleAxisd(M_PI / 5, Eigen::Vector3d::UnitZ()));
      detRotations.emplace_back(
          Eigen::AngleAxisd(M_PI / 4, Eigen::Vector3d::UnitZ()));
      detRotations.emplace_back(
          Eigen::AngleAxisd(M_PI / 3, Eigen::Vector3d::UnitZ()));
      auto detectorInfo =
          boost::make_shared<DetectorInfo>(detPositions, detRotations);
      auto bankSortedDetectorIndices =
          boost::make_shared<const std::vector<size_t>>(
              std::vector<size_t>{0, 2, 1});
      auto bankSortedComponentIndices =
          boost::make_shared<const std::vector<size_t>>(
              std::vector<size_t>{0, 1, 3, 2, 4});

      std::vector<std::pair<size_t, size_t>> detectorRanges;
      detectorRanges.push_back(
          std::make_pair(0, 2)); // sub-assembly (registered first)
      detectorRanges.push_back(
          std::make_pair(0, 3)); // instrument-assembly (with 3 detectors)

      std::vector<std::pair<size_t, size_t>> componentRanges;
      componentRanges.push_back(
          std::make_pair(0, 2)); // sub-assembly (contains two detectors)
      componentRanges.push_back(
          std::make_pair(0, 4)); // instrument assembly (with 4 sub-components)

      // Set non-detectors at different positions
      auto compPositions = boost::make_shared<std::vector<Eigen::Vector3d>>();
      compPositions->emplace_back(1, 0, 0);
      compPositions->emplace_back(1, 1, 0);

      // Set non-detectors at different rotations
      auto compRotations = boost::make_shared<std::vector<Eigen::Quaterniond>>();
      compRotations->emplace_back(
          Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));
      compRotations->emplace_back(
          Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));

      return std::make_tuple(ComponentInfo(
          bankSortedDetectorIndices,
          boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
              detectorRanges),
          bankSortedComponentIndices,
          boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
              componentRanges),
          compPositions, compRotations, detectorInfo), detPositions, detRotations, *compPositions, *compRotations);


  }

  ComponentInfo makeTreeExample() {
    /*
     Detectors are marked with detector indices below.
     There are 3 detectors.
     There are 2 assemblies, including the root

           |
     ------------
     |         | 1
    -------
    | 0  | 2
    */

    std::vector<Eigen::Vector3d> detPositions(3);
    std::vector<Eigen::Quaterniond> detRotations(3);
    auto bankSortedDetectorIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 2, 1});
    auto bankSortedComponentIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 2, 3, 1, 4});
    std::vector<std::pair<size_t, size_t>> detectorRanges;
    detectorRanges.push_back(std::make_pair(0, 2));
    detectorRanges.push_back(std::make_pair(0, 3));

    std::vector<std::pair<size_t, size_t>> componentRanges;
    componentRanges.push_back(
        std::make_pair(0, 2)); // sub-assembly (contains two detectors)
    componentRanges.push_back(
        std::make_pair(0, 4)); // instrument assembly (with 4 sub-components)

    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(
        2); // 2 positions provided. 2 non-detectors
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(
        2); // 2 rotations provided. 2 non-detectors

    return ComponentInfo(
        bankSortedDetectorIndices,
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            detectorRanges),
        bankSortedComponentIndices,
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            componentRanges),
        positions, rotations,
        boost::make_shared<DetectorInfo>(detPositions, detRotations));
  }

  void test_detector_indexes() {

    auto info = makeTreeExample();

    /*
    Note that detectors are always the first n component indexes!
    */
    TS_ASSERT_EQUALS(info.detectorIndices(0), std::vector<size_t>{0});
    TS_ASSERT_EQUALS(info.detectorIndices(1), std::vector<size_t>{1});
    TS_ASSERT_EQUALS(info.detectorIndices(2), std::vector<size_t>{2});

    // Now we have non-detector components
    TS_ASSERT_EQUALS(info.detectorIndices(4 /*component index of root*/),
                     std::vector<size_t>({0, 2, 1}));
    TS_ASSERT_EQUALS(
        info.detectorIndices(3 /*component index of sub-assembly*/),
        std::vector<size_t>({0, 2}));
  }

  void test_component_indexes() {

    auto info = makeTreeExample();
    /*
    Note that detectors are always the first n component indexes!
    */
    TS_ASSERT_EQUALS(info.componentIndices(0), std::vector<size_t>{0});
    TS_ASSERT_EQUALS(info.componentIndices(1), std::vector<size_t>{1});
    TS_ASSERT_EQUALS(info.componentIndices(2), std::vector<size_t>{2});

    // Now we have non-detector components
    TS_ASSERT_EQUALS(info.componentIndices(4 /*component index of root*/),
                     std::vector<size_t>({0, 2, 3, 1}));

    TS_ASSERT_EQUALS(
        info.componentIndices(3 /*component index of sub-assembly*/),
        std::vector<size_t>({0, 2}));
  }

};
#endif /* MANTID_BEAMLINE_COMPONENTINFOTEST_H_ */
