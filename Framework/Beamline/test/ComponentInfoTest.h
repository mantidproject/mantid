#ifndef MANTID_BEAMLINE_COMPONENTINFOTEST_H_
#define MANTID_BEAMLINE_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include <Eigen/Geometry>
#include <Eigen/StdVector>
#include <boost/make_shared.hpp>
#include <numeric>
#include <string>
#include <tuple>

using namespace Mantid::Beamline;

namespace {

using PosVec = std::vector<Eigen::Vector3d>;
using RotVec = std::vector<Eigen::Quaterniond,
                           Eigen::aligned_allocator<Eigen::Quaterniond>>;
using StrVec = std::vector<std::string>;

/*
 * Makes a tree which in which all detectors are arranged in a single flat
 *level. There just one non-detector component in this tree.
 *
 * The size of the Resultant ComponentInfo/DetectorInfo are set by the number of
 *position and rotation elements in the collections arguments.
 */
std::tuple<boost::shared_ptr<ComponentInfo>, boost::shared_ptr<DetectorInfo>>
makeFlatTree(PosVec detPositions, RotVec detRotations) {
  std::vector<std::pair<size_t, size_t>> componentRanges;
  auto rootIndex = detPositions.size();
  componentRanges.push_back(
      std::make_pair(0, 1)); // sub-assembly (contains root only)
  auto bankSortedDetectorIndices =
      boost::make_shared<std::vector<size_t>>(detPositions.size());
  std::iota(bankSortedDetectorIndices->begin(),
            bankSortedDetectorIndices->end(), 0);
  auto bankSortedComponentIndices =
      boost::make_shared<const std::vector<size_t>>(
          std::vector<size_t>{rootIndex});
  auto parentIndices = boost::make_shared<const std::vector<size_t>>(
      std::vector<size_t>(detPositions.size() + 1, rootIndex));
  std::vector<std::pair<size_t, size_t>> detectorRanges(
      1, std::make_pair<size_t, size_t>(0, detPositions.size()));
  auto positions = boost::make_shared<PosVec>(
      1, Eigen::Vector3d{0, 0, 0}); // 1 position only for root
  auto rotations = boost::make_shared<RotVec>(
      1,
      Eigen::Quaterniond::Identity()); // 1 rotation only for root

  // Component scale factors
  auto scaleFactors = boost::make_shared<PosVec>(
      PosVec(detPositions.size() + 1, Eigen::Vector3d{1, 1, 1}));
  // Component names
  auto names = boost::make_shared<StrVec>();
  for (size_t detIndex = 0; detIndex < detPositions.size(); ++detIndex) {
    names->emplace_back("det" + std::to_string(detIndex));
  }
  names->emplace_back("root");
  auto detectorInfo =
      boost::make_shared<DetectorInfo>(detPositions, detRotations);
  // Rectangular bank flag
  auto isRectangularBank =
      boost::make_shared<std::vector<ComponentType>>(1, ComponentType::Generic);

  std::vector<size_t> branch(detPositions.size());
  std::iota(branch.begin(), branch.end(), 0);
  auto children =
      boost::make_shared<std::vector<std::vector<size_t>>>(1, branch);

  auto componentInfo = boost::make_shared<ComponentInfo>(
      bankSortedDetectorIndices,
      boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          detectorRanges),
      bankSortedComponentIndices,
      boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          componentRanges),
      parentIndices, children, positions, rotations, scaleFactors,
      isRectangularBank, names, -1, -1);

  componentInfo->setDetectorInfo(detectorInfo.get());

  return std::make_tuple(componentInfo, detectorInfo);
}

std::tuple<boost::shared_ptr<ComponentInfo>, PosVec, RotVec, PosVec, RotVec,
           boost::shared_ptr<DetectorInfo>>
makeTreeExampleAndReturnGeometricArguments() {

  /*
         |
   ------------
   |         | 1
  -------
  | 0  | 2
  */

  // Set detectors at different positions
  PosVec detPositions;
  detPositions.emplace_back(1, -1, 0);
  detPositions.emplace_back(2, -1, 0);
  detPositions.emplace_back(3, -1, 0);
  // Set all Detectors rotated 45 degrees around Y
  RotVec detRotations(3, Eigen::Quaterniond(Eigen::AngleAxisd(
                             M_PI / 4, Eigen::Vector3d::UnitY())));
  auto detectorInfo =
      boost::make_shared<DetectorInfo>(detPositions, detRotations);
  auto bankSortedDetectorIndices =
      boost::make_shared<const std::vector<size_t>>(
          std::vector<size_t>{0, 2, 1});
  auto bankSortedComponentIndices =
      boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{3, 4});
  auto parentIndices = boost::make_shared<const std::vector<size_t>>(
      std::vector<size_t>{3, 3, 4, 4, 4});

  std::vector<std::pair<size_t, size_t>> detectorRanges;
  detectorRanges.push_back(
      std::make_pair(0, 2)); // sub-assembly (registered first)
  detectorRanges.push_back(
      std::make_pair(0, 3)); // instrument-assembly (with 3 detectors)

  std::vector<std::pair<size_t, size_t>> componentRanges;
  componentRanges.push_back(
      std::make_pair(0, 1)); // sub-assembly (contains self)
  componentRanges.push_back(std::make_pair(
      0, 2)); // instrument assembly (with 1 sub-component and self)

  // Set non-detectors at different positions
  auto compPositions = boost::make_shared<PosVec>();
  compPositions->emplace_back(1, -1, 0);
  compPositions->emplace_back(1, -1, 0);

  // Set non-detectors at different rotations
  auto compRotations = boost::make_shared<RotVec>();
  compRotations->emplace_back(Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ()));
  compRotations->emplace_back(Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ()));

  // Component scale factors
  auto scaleFactors =
      boost::make_shared<PosVec>(PosVec(5, Eigen::Vector3d{1, 1, 1}));
  // Component names
  auto names = boost::make_shared<StrVec>(5);
  // Rectangular bank flag
  auto isRectangularBank =
      boost::make_shared<std::vector<ComponentType>>(2, ComponentType::Generic);
  auto children = boost::make_shared<std::vector<std::vector<size_t>>>(
      2, std::vector<size_t>(2));

  auto compInfo = boost::make_shared<ComponentInfo>(
      bankSortedDetectorIndices,
      boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          detectorRanges),
      bankSortedComponentIndices,
      boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          componentRanges),
      parentIndices, children, compPositions, compRotations, scaleFactors,
      isRectangularBank, names, -1, -1);

  compInfo->setDetectorInfo(detectorInfo.get());

  return std::make_tuple(compInfo, detPositions, detRotations, *compPositions,
                         *compRotations, detectorInfo);
}

std::tuple<boost::shared_ptr<ComponentInfo>, boost::shared_ptr<DetectorInfo>>
makeTreeExample() {
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

  PosVec detPositions(3);
  RotVec detRotations(3);
  auto bankSortedDetectorIndices =
      boost::make_shared<const std::vector<size_t>>(
          std::vector<size_t>{0, 2, 1});
  auto bankSortedComponentIndices =
      boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{3, 4});
  auto parentIndices = boost::make_shared<const std::vector<size_t>>(
      std::vector<size_t>{3, 3, 4, 4, 4});
  std::vector<std::pair<size_t, size_t>> detectorRanges;
  detectorRanges.push_back(std::make_pair(0, 2));
  detectorRanges.push_back(std::make_pair(0, 3));

  std::vector<std::pair<size_t, size_t>> componentRanges;
  componentRanges.push_back(
      std::make_pair(0, 1)); // sub-assembly (contains self)
  componentRanges.push_back(std::make_pair(
      0, 2)); // instrument assembly (with 1 sub-component and self)

  auto positions = boost::make_shared<PosVec>(
      2, Eigen::Vector3d{0, 0, 0}); // 2 positions provided. 2 non-detectors
  auto rotations = boost::make_shared<RotVec>(
      2,
      Eigen::Quaterniond::Identity()); // 2 rotations provided. 2 non-detectors

  // Component scale factors
  auto scaleFactors =
      boost::make_shared<PosVec>(PosVec(5, Eigen::Vector3d{1, 1, 1}));
  // Component names
  auto names = boost::make_shared<StrVec>(5);
  auto detectorInfo =
      boost::make_shared<DetectorInfo>(detPositions, detRotations);
  // Rectangular bank flag
  auto isRectangularBank =
      boost::make_shared<std::vector<ComponentType>>(2, ComponentType::Generic);

  auto children = boost::make_shared<std::vector<std::vector<size_t>>>(
      2, std::vector<size_t>(2));

  auto componentInfo = boost::make_shared<ComponentInfo>(
      bankSortedDetectorIndices,
      boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          detectorRanges),
      bankSortedComponentIndices,
      boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          componentRanges),
      parentIndices, children, positions, rotations, scaleFactors,
      isRectangularBank, names, -1, -1);

  componentInfo->setDetectorInfo(detectorInfo.get());

  return std::make_tuple(componentInfo, detectorInfo);
}

// Helper to clone and resync both Info objects
std::tuple<boost::shared_ptr<ComponentInfo>, boost::shared_ptr<DetectorInfo>>
cloneInfos(const std::tuple<boost::shared_ptr<ComponentInfo>,
                            boost::shared_ptr<DetectorInfo>> &in) {
  auto compInfo = boost::shared_ptr<ComponentInfo>(
      std::get<0>(in)->cloneWithoutDetectorInfo());
  auto detInfo = boost::make_shared<DetectorInfo>(*std::get<1>(in));
  compInfo->setDetectorInfo(detInfo.get());
  detInfo->setComponentInfo(compInfo.get());
  return std::make_tuple(compInfo, detInfo);
}

} // namespace

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
    auto infos = makeTreeExample();
    auto compInfo = std::get<0>(infos);

    TS_ASSERT_EQUALS(compInfo->size(), 5);
  }

  void test_partial_clone() {
    auto infos = makeTreeExample();
    auto compInfo = std::get<0>(infos);
    TS_ASSERT(compInfo->hasDetectorInfo());
    auto clone = compInfo->cloneWithoutDetectorInfo();
    TSM_ASSERT("DetectorInfo is not copied", !clone->hasDetectorInfo());
    // Sanity check other internals
    TS_ASSERT_EQUALS(compInfo->size(), clone->size());
  }

  void
  test_setter_throws_if_size_mismatch_between_detector_indices_and_detectorinfo() {
    /*
     Imitate an instrument with 3 detectors and nothing more.
    */
    auto bankSortedDetectorIndices =
        boost::make_shared<const std::vector<size_t>>(
            std::vector<size_t>{0, 1, 2});
    auto bankSortedComponentIndices =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>(1));
    auto parentIndices =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{
            9, 9, 9, 9}); // These indices are invalid, but that's
                          // ok as not being tested here
    auto detectorRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            1, std::pair<size_t, size_t>{0, 2});
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::vector<std::pair<size_t, size_t>>{});
    auto positions = boost::make_shared<PosVec>(1);
    auto rotations = boost::make_shared<RotVec>(1);
    auto scaleFactors = boost::make_shared<PosVec>(4);
    auto names = boost::make_shared<StrVec>(4);
    auto isRectangularBank = boost::make_shared<std::vector<ComponentType>>(1);
    auto children = boost::make_shared<std::vector<std::vector<size_t>>>(
        1, std::vector<size_t>(3));

    ComponentInfo componentInfo(bankSortedDetectorIndices, detectorRanges,
                                bankSortedComponentIndices, componentRanges,
                                parentIndices, children, positions, rotations,
                                scaleFactors, isRectangularBank, names, -1, -1);

    DetectorInfo detectorInfo; // Detector info size 0
    TS_ASSERT_THROWS(componentInfo.setDetectorInfo(&detectorInfo),
                     std::invalid_argument &);
  }

  void test_throw_if_positions_rotation_inputs_different_sizes() {
    auto detectorsInSubtree =
        boost::make_shared<const std::vector<size_t>>(); // No detector indices
                                                         // in this example!

    auto bankSortedComponentIndices =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{0});
    auto parentIndices = boost::make_shared<const std::vector<size_t>>(
        std::vector<size_t>{9, 9, 9}); // These indices are invalid, but that's
                                       // ok as not being tested here
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
    auto positions = boost::make_shared<PosVec>(1); // 1 position provided
    auto rotations = boost::make_shared<RotVec>(0); // 0 rotations provided

    auto scaleFactors = boost::make_shared<PosVec>();
    auto names = boost::make_shared<StrVec>();
    auto isRectangularBank = boost::make_shared<std::vector<ComponentType>>(
        2, ComponentType::Generic);
    auto children = boost::make_shared<
        std::vector<std::vector<size_t>>>(); // invalid but not being tested

    TS_ASSERT_THROWS(ComponentInfo(detectorsInSubtree, detectorRanges,
                                   bankSortedComponentIndices, componentRanges,
                                   parentIndices, children, positions,
                                   rotations, scaleFactors, isRectangularBank,
                                   names, -1, -1),
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
    auto detectorsInSubtree =
        boost::make_shared<const std::vector<size_t>>(); // No detector indices
                                                         // in this example!

    auto componentsInSubtree =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{0});

    auto detectorRanges = boost::make_shared<
        const std::vector<std::pair<size_t, size_t>>>(); // Empty detectorRanges

    auto parentIndices = boost::make_shared<const std::vector<size_t>>(
        std::vector<size_t>{9, 9, 9}); // These indices are invalid, but that's
                                       // ok as not being tested here
    auto positions = boost::make_shared<PosVec>(1); // 1 position provided
    auto rotations = boost::make_shared<RotVec>(1); // 1 rotation provided

    auto scaleFactors = boost::make_shared<PosVec>();
    auto names = boost::make_shared<StrVec>();
    // Only one component. So single empty component range.
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::vector<std::pair<size_t, size_t>>{{0, 0}});
    auto isRectangularBank = boost::make_shared<std::vector<ComponentType>>(
        2, ComponentType::Generic);
    auto children = boost::make_shared<
        std::vector<std::vector<size_t>>>(); // invalid but not being tested

    TS_ASSERT_THROWS(ComponentInfo(detectorsInSubtree, detectorRanges,
                                   componentsInSubtree, componentRanges,
                                   parentIndices, children, positions,
                                   rotations, scaleFactors, isRectangularBank,
                                   names, -1, -1),
                     std::invalid_argument &);
  }

  void test_throw_if_instrument_tree_not_same_size_as_number_of_components() {
    /*
     * Positions are rotations are only currently stored for non-detector
     * components
     * We should have as many detectorRanges as we have non-detector components
     * too.
     * All vectors should be the same size.
     */
    auto detectorsInSubtree =
        boost::make_shared<const std::vector<size_t>>(); // No detector indices
                                                         // in this example!

    auto componentsInSubtree =
        boost::make_shared<const std::vector<size_t>>(std::vector<size_t>{0});

    auto detectorRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            1, std::pair<size_t, size_t>(0, 0));

    auto parentIndices = boost::make_shared<const std::vector<size_t>>(
        std::vector<size_t>{9, 9, 9}); // These indices are invalid, but that's
                                       // ok as not being tested here
    auto positions = boost::make_shared<PosVec>(1);
    auto rotations = boost::make_shared<RotVec>(1);

    auto scaleFactors = boost::make_shared<PosVec>(1);
    auto names = boost::make_shared<StrVec>(1);
    // Only one component. So single empty component range.
    auto componentRanges =
        boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
            std::vector<std::pair<size_t, size_t>>{{0, 0}});
    auto componentTypes =
        boost::make_shared<std::vector<Mantid::Beamline::ComponentType>>(
            1, Mantid::Beamline::ComponentType::Generic);
    auto children = boost::make_shared<std::vector<std::vector<size_t>>>(
        1, std::vector<size_t>{1, 2}); // invalid

    TS_ASSERT_THROWS(
        ComponentInfo(detectorsInSubtree, detectorRanges, componentsInSubtree,
                      componentRanges, parentIndices, children, positions,
                      rotations, scaleFactors, componentTypes, names, -1, -1),
        std::invalid_argument &);
  }

  void test_read_positions_rotations() {
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo &info = *std::get<0>(allOutputs);
    // Arguments to ComponentInfo for geometric aspects
    PosVec detPositions = std::get<1>(allOutputs);
    RotVec detRotations = std::get<2>(allOutputs);
    PosVec compPositions = std::get<3>(allOutputs);
    RotVec compRotations = std::get<4>(allOutputs);

    /*
     * Remember. We have 3 detectors. So component index 3 corresponds to
     * position index 0 in the input vector since we don't input positions for
     * detectors via ComponentInfo
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

  template <typename IndexType>
  void do_write_positions(const IndexType rootIndex) {

    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();
    ComponentInfo &info = *std::get<0>(allOutputs);
    // Arguments to ComponentInfo for geometric aspects
    PosVec originalDetPositions = std::get<1>(allOutputs);
    RotVec originalDetRotations = std::get<2>(allOutputs);
    PosVec originalCompPositions = std::get<3>(allOutputs);
    RotVec originalCompRotations = std::get<4>(allOutputs);

    // Change the position of the root.

    Eigen::Vector3d rootDestination{60, 0, 0};

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

  void test_write_positions() {
    const size_t rootIndex = 4;
    do_write_positions(rootIndex);
  }

  template <typename IndexType>
  void do_test_write_rotation(ComponentInfo &info, const IndexType rootIndex,
                              const IndexType detectorIndex) {
    using namespace Eigen;

    const auto theta = M_PI / 2;      // 90 degree rotation
    Eigen::Vector3d axis = {0, 1, 0}; // rotate around y axis
    const auto center =
        info.position(rootIndex); // rotate around target component center.

    const auto transform = Translation3d(center) * AngleAxisd(theta, axis) *
                           Translation3d(-center);

    // Define new rotation
    const Quaterniond requestedRotation(transform.rotation());
    // Detector original rotation
    const auto detOriginalRotation = info.rotation(detectorIndex);

    // Perform 90 rotation of root
    info.setRotation(rootIndex, requestedRotation);

    // Fetch root rotation
    auto actualRootRotation = info.rotation(rootIndex);
    TSM_ASSERT("Rotations should exactly match as we are overwriting with an "
               "abs rotation",
               actualRootRotation.isApprox(requestedRotation));
    TSM_ASSERT_DELTA("Acutal rotation should be 90 deg around y",
                     std::asin(actualRootRotation.y()) * 2, theta, 1e-4);

    auto actualDetRotation = info.rotation(detectorIndex);
    TSM_ASSERT_DELTA("Detector rotation should be accumulation existing 45 + "
                     "new 90 rotation",
                     std::asin(actualDetRotation.y()) * 2,
                     theta + std::asin(detOriginalRotation.y()) * 2, 1e-4);
  }

  template <typename IndexType>
  void
  do_write_rotation_updates_positions_correctly(ComponentInfo &info,
                                                const IndexType rootIndex,
                                                const IndexType detectorIndex) {
    using namespace Eigen;
    const auto theta = M_PI / 2;                  // 90 degree rotation
    Eigen::Vector3d axis = {0, 1, 0};             // rotate around y axis
    const auto center = info.position(rootIndex); // rotate around root center.

    const auto transform = Translation3d(center) * AngleAxisd(theta, axis) *
                           Translation3d(-center);

    // Just the rotation part.
    const Quaterniond rootRotation(transform.rotation());

    const auto rootOriginalPosition = info.position(rootIndex);
    // Perform rotation
    info.setRotation(rootIndex, rootRotation);
    const auto rootUpdatedPosition = info.position(rootIndex);
    const auto detector2UpdatedPosition = info.position(detectorIndex);
    TSM_ASSERT("Rotate root around origin = root centre. It should not move!",
               rootOriginalPosition.isApprox(rootUpdatedPosition));

    /* Detector 2
     * originally at {2, -1, 0}. Rotated 90 deg around {0, 1, 0} with centre {1,
     *0, 0} should
     * put it exactly at {1, -1, -1,}
     *
     *     view down y.
     *      z
     *      ^
     *      |
     *      |--> x
     *
     * before rotation:
     *
     *      p (center p at {1, -1, 0})       d (at {2, -1, 0})
     *
     * after rotation:
     *
     *      d (now at {1, -1, -1})
     *
     *      p (centre p at {1,0,0})
     */
    TSM_ASSERT(
        "Rotate detector around origin = root centre. It should reposition!",
        detector2UpdatedPosition.isApprox(Vector3d{1, -1, -1}));
  }

  void test_write_rotation() {
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo &info = *std::get<0>(allOutputs);
    size_t rootIndex = 4;
    size_t detectorIndex = 1;
    do_test_write_rotation(info, rootIndex, detectorIndex);
  }

  void test_write_rotation_updates_positions_correctly() {
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo &info = *std::get<0>(allOutputs);
    // Arguments to ComponentInfo for geometric aspects

    const size_t rootIndex = 4;
    const size_t detectorIndex = 1;
    do_write_rotation_updates_positions_correctly(info, rootIndex,
                                                  detectorIndex);
  }

  void test_detector_indexes() {

    auto infos = makeTreeExample();
    const auto &compInfo = *std::get<0>(infos);
    /*
    Note that detectors are always the first n component indexes!
    */
    TS_ASSERT_EQUALS(compInfo.detectorsInSubtree(0), std::vector<size_t>{0});
    TS_ASSERT_EQUALS(compInfo.detectorsInSubtree(1), std::vector<size_t>{1});
    TS_ASSERT_EQUALS(compInfo.detectorsInSubtree(2), std::vector<size_t>{2});

    // Now we have non-detector components
    TS_ASSERT_EQUALS(compInfo.detectorsInSubtree(4 /*component index of root*/),
                     std::vector<size_t>({0, 2, 1}));
    TS_ASSERT_EQUALS(
        compInfo.detectorsInSubtree(3 /*component index of sub-assembly*/),
        std::vector<size_t>({0, 2}));
  }

  void test_component_indexes() {

    auto infos = makeTreeExample();
    const auto &compInfo = *std::get<0>(infos);
    /*
    Note that detectors are always the first n component indexes!
    */
    TS_ASSERT_EQUALS(compInfo.componentsInSubtree(0), std::vector<size_t>{0});
    TS_ASSERT_EQUALS(compInfo.componentsInSubtree(1), std::vector<size_t>{1});
    TS_ASSERT_EQUALS(compInfo.componentsInSubtree(2), std::vector<size_t>{2});

    // Now we have non-detector components
    TS_ASSERT_EQUALS(
        compInfo.componentsInSubtree(4 /*component index of root*/),
        std::vector<size_t>(
            {0, 2, 1, 3, 4})); // Note inclusion of self comp index

    TS_ASSERT_EQUALS(
        compInfo.componentsInSubtree(3 /*component index of sub-assembly*/),
        std::vector<size_t>({0, 2, 3})); // Note inclusion of self comp index
  }

  void test_parent_component_indices() {
    auto infos = makeTreeExample();
    const auto &compInfo = *std::get<0>(infos);
    TSM_ASSERT_EQUALS("Root component's parent index is self", 4,
                      compInfo.parent(4));
    TSM_ASSERT_EQUALS("Parent of detector 0 is assembly index 3", 3,
                      compInfo.parent(0));
  }

  void test_set_detectorInfo() {

    ComponentInfo componentInfo;
    DetectorInfo detectorInfo;
    TS_ASSERT(!componentInfo.hasDetectorInfo());
    componentInfo.setDetectorInfo(&detectorInfo);
    TS_ASSERT(componentInfo.hasDetectorInfo());
  }

  void test_read_relative_position_simple_case() {
    // Not dealing with rotations at all here in this test

    using namespace Eigen;
    auto infos = makeTreeExample();
    auto &compInfo = *std::get<0>(infos);

    const size_t rootIndex = 4;
    const size_t detectorIndex = 0;

    Eigen::Vector3d rootPosition{1, 0, 0};
    compInfo.setPosition(rootIndex, rootPosition);
    compInfo.setRotation(rootIndex, Quaterniond::Identity()); // Ensure
                                                              // Root/Parent is
                                                              // NOT rotated in
                                                              // this example
    Eigen::Vector3d detPosition{2, 0, 0};
    compInfo.setPosition(detectorIndex, detPosition);

    TSM_ASSERT("For a root (no parent) relative positions are always the same "
               "as absolute ones",
               compInfo.position(rootIndex).isApprox(
                   compInfo.relativePosition(rootIndex)));

    const Eigen::Vector3d expectedRelativePos =
        compInfo.position(detectorIndex) -
        compInfo.position(compInfo.parent(detectorIndex));

    const Eigen::Vector3d actualRelativePos =
        compInfo.relativePosition(detectorIndex);
    TS_ASSERT(expectedRelativePos.isApprox(actualRelativePos));
  }

  void test_read_relative_position_complex_case() {

    using namespace Eigen;
    auto infos = makeTreeExample();
    auto &compInfo = *std::get<0>(infos);

    const size_t rootIndex = 4;
    const size_t subComponentIndex = 3;

    Vector3d rootPosition{0, 0, 0};
    Vector3d subCompPosition{2, 0, 0};
    compInfo.setPosition(rootIndex, rootPosition);
    compInfo.setPosition(subComponentIndex, subCompPosition);
    compInfo.setRotation(
        rootIndex,
        Quaterniond(AngleAxisd(
            M_PI / 2, Vector3d::UnitY()))); // Root is rotated 90 Deg around Y

    // Quick sanity check. We now expect the absolute position of the
    // subcomponent to be rotated by above.
    TS_ASSERT(
        compInfo.position(subComponentIndex).isApprox(Vector3d{0, 0, -2}));
    // Relative position removes the parent rotation. Should be 2,0,0 (which is
    // comp - root).
    TS_ASSERT(compInfo.relativePosition(subComponentIndex)
                  .isApprox(subCompPosition - rootPosition));

    const auto diffPos =
        compInfo.position(subComponentIndex) - compInfo.position(rootIndex);
    TSM_ASSERT("Vector between comp and root is not the same as relative "
               "position. Rotation involved.",
               !compInfo.relativePosition(subComponentIndex).isApprox(diffPos));
  }

  void test_read_relative_rotation() {
    // throw std::runtime_error("Test not implemented but needed!");

    using namespace Eigen;
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo &info = *std::get<0>(allOutputs);
    // Arguments to ComponentInfo for geometric aspects

    const size_t rootIndex = 4;
    const size_t subAssemblyIndex = 3;
    const auto theta = M_PI / 2;      // 90 degree rotation
    Eigen::Vector3d axis = {0, 1, 0}; // rotate around y axis
    const auto rootCenter =
        info.position(rootIndex); // for rotation around root center.
    const auto subAssemblyCenter = info.position(
        subAssemblyIndex); // for rotation around sub-assembly center
    // Note that in the example rootCenter is the same as the subAssemblyCenter

    // Compound rotation. First rotate around the root.
    auto transform1 = Translation3d(rootCenter) * AngleAxisd(theta, axis) *
                      Translation3d(-rootCenter);
    info.setRotation(rootIndex,
                     Quaterniond(transform1.rotation())); // Do first rotation

    // Compound rotation. Secondly rotate around the sub-assembly.
    auto transform2 = Translation3d(subAssemblyCenter) *
                      AngleAxisd(theta, axis) *
                      Translation3d(-subAssemblyCenter);
    info.setRotation(rootIndex,
                     Quaterniond(transform2.rotation())); // Do second rotation

    TSM_ASSERT(
        "For a root (no parent) relative rotations are always the same as "
        "absolute ones",
        info.relativeRotation(rootIndex).isApprox(info.rotation(rootIndex)));
    TSM_ASSERT_DELTA(
        "90 degree RELATIVE rotation between root ans sub-assembly",
        info.relativeRotation(rootIndex).angularDistance(
            info.relativeRotation(subAssemblyIndex)),
        theta, 1e-6);
  }

  void test_has_parent() {
    using namespace Eigen;
    auto infos = makeTreeExample();
    auto &compInfo = *std::get<0>(infos);

    TSM_ASSERT("Detector should have a parent", compInfo.hasParent(0));
    TSM_ASSERT("Sub component should have a parent", compInfo.hasParent(3));
    TSM_ASSERT("Root component should not have a parent",
               !compInfo.hasParent(compInfo.size() - 1 /*root index*/));
  }

  void test_scale_factors() {

    using namespace Eigen;
    auto infos = makeTreeExample();
    auto &compInfo = *std::get<0>(infos);

    // No scale factors by default
    for (size_t i = 0; i < compInfo.size(); ++i) {
      TS_ASSERT_EQUALS(Eigen::Vector3d(1.0, 1.0, 1.0), compInfo.scaleFactor(i));
    }
    Eigen::Vector3d newFactor(1, 2, 3);
    // Overwrite
    compInfo.setScaleFactor(0, newFactor);
    // Read-back
    TS_ASSERT_EQUALS(compInfo.scaleFactor(0), newFactor);
  }

  void test_name() {
    auto infos = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &compInfo = *std::get<0>(infos);
    TS_ASSERT_EQUALS(compInfo.name(compInfo.root()), "root");
    TS_ASSERT_EQUALS(compInfo.name(0), "det0");
  }

  void test_indexOfAny_name_throws_when_name_invalid() {
    auto infos = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &compInfo = *std::get<0>(infos);
    TSM_ASSERT_THROWS("Should throw, this name does not exist",
                      compInfo.indexOfAny("phantom"), std::invalid_argument &)
    // Sanity check.
    TSM_ASSERT_THROWS_NOTHING("Should NOT throw if provided with a valid name",
                              compInfo.indexOfAny(compInfo.name(0)));
  }

  void test_indexOfAny() {
    auto infos = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &compInfo = *std::get<0>(infos);
    TS_ASSERT_EQUALS(compInfo.indexOfAny("det0"), 0);
    TS_ASSERT_EQUALS(compInfo.indexOfAny("root"), compInfo.root());
  }

  void test_scan_count_no_scanning() {
    ComponentInfo info;
    TS_ASSERT_EQUALS(info.scanCount(), 1);
  }

  void test_unmerged_is_not_scanning() {
    auto infos = makeTreeExample();
    auto &compInfo = *std::get<0>(infos);

    TSM_ASSERT("No time indexed points added so should not be scanning",
               !compInfo.isScanning());
    // Add a scan interval
    compInfo.setScanInterval(std::pair<int64_t, int64_t>{1000, 1001});
    TSM_ASSERT("No time indexed points added so should still not be scanning",
               !compInfo.isScanning());
  }

  void test_setPosition_single_scan() {
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    const std::pair<size_t, size_t> rootIndex = {4, 0};
    do_write_positions(rootIndex);
  }

  void test_setRotation_single_scan() {
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo &info = *std::get<0>(allOutputs);
    const std::pair<size_t, size_t> rootIndex{4, 0};
    const std::pair<size_t, size_t> detectorIndex{1, 0};
    do_test_write_rotation(info, rootIndex, detectorIndex);
  }

  void test_setRotation_single_scan_updates_positions_correctly() {
    auto allOutputs = makeTreeExampleAndReturnGeometricArguments();

    // Resulting ComponentInfo
    ComponentInfo &info = *std::get<0>(allOutputs);
    const std::pair<size_t, size_t> rootIndex{4, 0};
    const std::pair<size_t, size_t> detectorIndex{1, 0};
    do_write_rotation_updates_positions_correctly(info, rootIndex,
                                                  detectorIndex);
  }

  void test_setScanInterval_failures() {
    auto infos = makeTreeExample();
    auto &compInfo = *std::get<0>(infos);
    TS_ASSERT_THROWS_EQUALS(
        compInfo.setScanInterval({1, 1}), const std::runtime_error &e,
        std::string(e.what()),
        "ComponentInfo: cannot set scan interval with start >= end");
    TS_ASSERT_THROWS_EQUALS(
        compInfo.setScanInterval({2, 1}), const std::runtime_error &e,
        std::string(e.what()),
        "ComponentInfo: cannot set scan interval with start >= end");
  }

  void test_merge_fail_size() {

    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos2 = makeFlatTree(PosVec(2), RotVec(2));
    auto &a = *std::get<0>(infos1);
    auto &b = *std::get<0>(infos2);
    a.setScanInterval({0, 1});
    b.setScanInterval({0, 1});
    b.setScanInterval({0, 1});
    TS_ASSERT_THROWS_EQUALS(a.merge(b), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: size mismatch");
  }

  void test_merge_fail_no_intervals() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos2 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos3 = makeFlatTree(PosVec(1), RotVec(1));
    auto &a = *std::get<0>(infos1);
    auto &b = *std::get<0>(infos2);
    auto &c = *std::get<0>(infos3);
    TS_ASSERT_THROWS_EQUALS(
        a.merge(b), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge ComponentInfo: scan intervals not defined");
    c.setScanInterval({0, 1});
    TS_ASSERT_THROWS_EQUALS(
        a.merge(c), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge ComponentInfo: scan intervals not defined");
    a.setScanInterval({0, 1});
    TS_ASSERT_THROWS_EQUALS(
        a.merge(b), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge ComponentInfo: scan intervals not defined");
  }

  void test_merge_identical() {
    auto infos1 = makeFlatTree(PosVec(1, Eigen::Vector3d(0, 0, 0)),
                               RotVec(1, Eigen::Quaterniond(Eigen::AngleAxisd(
                                             0, Eigen::Vector3d::UnitY()))));
    ComponentInfo &a = *std::get<0>(infos1);
    a.setScanInterval({0, 10});

    auto infos2 = makeFlatTree(PosVec(1, Eigen::Vector3d(0, 0, 0)),
                               RotVec(1, Eigen::Quaterniond(Eigen::AngleAxisd(
                                             0, Eigen::Vector3d::UnitY()))));
    ComponentInfo &b = *std::get<0>(infos2);
    b.setScanInterval({0, 10});

    TSM_ASSERT_EQUALS("Scan size should be 1", b.scanCount(), 1);
    b.merge(a);
    TS_ASSERT_THROWS_NOTHING(b.merge(a));
    TSM_ASSERT_EQUALS("Intervals identical. Scan size should not grow",
                      b.scanCount(), 1)
  }

  void test_merge_identical_interval_when_positions_differ() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);
    a.setScanInterval({0, 1});
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    auto rootIndex = a.root();
    a.setPosition(rootIndex, pos1);
    auto infos2 = cloneInfos(infos1);
    ComponentInfo &b = *std::get<0>(infos2);
    // Sanity check
    TS_ASSERT_THROWS_NOTHING(b.merge(a));

    auto infos3 = cloneInfos(infos1);
    ComponentInfo &c = *std::get<0>(infos3);
    c.setPosition(rootIndex, pos2);
    TS_ASSERT_THROWS_EQUALS(c.merge(a), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: "
                            "matching scan interval but "
                            "positions differ");
    c.setPosition(rootIndex, pos1);
    TS_ASSERT_THROWS_NOTHING(c.merge(a));
  }

  void test_merge_identical_interval_when_rotations_differ() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);
    a.setScanInterval({0, 1});
    Eigen::Quaterniond rot1(
        Eigen::AngleAxisd(30.0, Eigen::Vector3d{1, 2, 3}.normalized()));
    Eigen::Quaterniond rot2(
        Eigen::AngleAxisd(31.0, Eigen::Vector3d{1, 2, 3}.normalized()));
    auto rootIndex = a.root();
    a.setRotation(rootIndex, rot1);
    auto infos2 = cloneInfos(infos1);
    ComponentInfo &b = *std::get<0>(infos2);
    // Sanity check
    TS_ASSERT_THROWS_NOTHING(b.merge(a));

    auto infos3 = cloneInfos(infos1);
    ComponentInfo &c = *std::get<0>(infos3);
    c.setRotation(rootIndex, rot2);
    TS_ASSERT_THROWS_EQUALS(c.merge(a), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: "
                            "matching scan interval but "
                            "rotations differ");
  }

  void test_merge_fail_partial_overlap() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);
    a.setScanInterval({0, 10});

    auto infos2 = cloneInfos(infos1);
    ComponentInfo &b = *std::get<0>(infos2);
    b.setScanInterval({-1, 5});
    TS_ASSERT_THROWS_EQUALS(b.merge(a), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: sync scan intervals "
                            "overlap but not identical");
    b.setScanInterval({1, 5});
    TS_ASSERT_THROWS_EQUALS(b.merge(a), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: sync scan intervals "
                            "overlap but not identical");
    b.setScanInterval({1, 11});
    TS_ASSERT_THROWS_EQUALS(b.merge(a), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: sync scan intervals "
                            "overlap but not identical");
  }

  void test_merge_detectors() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos2 = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);

    ComponentInfo &b = *std::get<0>(infos2);
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    a.setPosition(0, pos1);
    b.setPosition(0, pos2);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    a.setScanInterval(interval1);
    b.setScanInterval(interval2);
    a.merge(b); // Execute the merge
    TS_ASSERT(a.isScanning());
    TS_ASSERT_EQUALS(a.size(), 2);
    TS_ASSERT_EQUALS(a.scanSize(), a.size() + b.size());
    TS_ASSERT_EQUALS(a.scanCount(), 2);
    // Note that the order is not guaranteed, currently these are just in the
    // order in which the are merged.
    auto index1 =
        std::pair<size_t, size_t>(0 /*static index*/, 0 /*time index*/);
    auto index2 =
        std::pair<size_t, size_t>(0 /*static index*/, 1 /*time index*/);
    TS_ASSERT_EQUALS(a.scanIntervals()[index1.second], interval1);
    TS_ASSERT_EQUALS(a.scanIntervals()[index2.second], interval2);
    TS_ASSERT_EQUALS(a.position(index1), pos1);
    TS_ASSERT_EQUALS(a.position(index2), pos2);
    // Test Detector info is synched internally
    const DetectorInfo &mergeDetectorInfo = *std::get<1>(infos1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanCount(), 2);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[index1.second], interval1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[index2.second], interval2);
    TS_ASSERT_EQUALS(mergeDetectorInfo.position(index1), pos1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.position(index2), pos2);
  }

  void test_merge_root_with_offset() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos2 = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);

    ComponentInfo &b = *std::get<0>(infos2);
    const auto detPosA = a.position(0);
    const auto detPosB = b.position(0);
    const auto rootPosA = a.position(a.root());
    const auto rootPosB = b.position(b.root());
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    a.setPosition(a.root(), pos1);
    b.setPosition(b.root(), pos2);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    a.setScanInterval(interval1);
    b.setScanInterval(interval2);
    a.merge(b); // Execute the merge
    TS_ASSERT(a.isScanning());
    TS_ASSERT_EQUALS(a.size(), 2);
    TS_ASSERT_EQUALS(a.scanSize(), 2 * 2);
    TS_ASSERT_EQUALS(a.scanCount(), 2);
    // Note that the order is not guaranteed, currently these are just in the
    // order in which the are merged.
    auto index1 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 0 /*time index*/);
    auto index2 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 1 /*time index*/);
    TS_ASSERT_EQUALS(a.scanIntervals()[index1.second], interval1);
    TS_ASSERT_EQUALS(a.scanIntervals()[index2.second], interval2);
    TS_ASSERT_EQUALS(a.position(index1), pos1);
    TS_ASSERT_EQUALS(a.position(index2), pos2);

    // Test Detector info is synched internally
    const DetectorInfo &mergeDetectorInfo = *std::get<1>(infos1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanCount(), 1 * 2);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[0], interval1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[1], interval2);
    // Check that the child detectors have been positioned according to the
    // correct offsets
    const auto rootOffsetA = pos1 - rootPosA;
    const auto rootOffsetB = pos2 - rootPosB;
    TS_ASSERT_EQUALS(mergeDetectorInfo.position({0, 0}), rootOffsetA + detPosA);
    TS_ASSERT_EQUALS(mergeDetectorInfo.position({0, 1}), rootOffsetB + detPosB);
  }

  void test_merge_root_with_rotation() {
    auto detPos = Eigen::Vector3d{1, 0, 0};
    auto infos1 = makeFlatTree(PosVec(1, detPos), RotVec(1));
    auto infos2 = makeFlatTree(PosVec(1, detPos), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);

    ComponentInfo &b = *std::get<0>(infos2);
    Eigen::Quaterniond rot1(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY()));
    Eigen::Quaterniond rot2(
        Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitY()));
    a.setRotation(a.root(), rot1);
    b.setRotation(b.root(), rot2);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    a.setScanInterval(interval1);
    b.setScanInterval(interval2);
    a.merge(b); // Execute the merge
    TS_ASSERT(a.isScanning());
    TS_ASSERT_EQUALS(a.size(), 2);
    TS_ASSERT_EQUALS(a.scanSize(), 2 * 2);
    TS_ASSERT_EQUALS(a.scanCount(), 2);
    // Note that the order is not guaranteed, currently these are just in the
    // order in which the are merged.
    auto index1 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 0 /*time index*/);
    auto index2 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 1 /*time index*/);
    TS_ASSERT_EQUALS(a.scanIntervals()[index1.second], interval1);
    TS_ASSERT_EQUALS(a.scanIntervals()[index2.second], interval2);
    TS_ASSERT(a.rotation(index1).isApprox(rot1));
    TS_ASSERT(a.rotation(index2).isApprox(rot2));

    // Test Detector info is synched internally
    const DetectorInfo &mergeDetectorInfo = *std::get<1>(infos1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanCount(), 1 * 2);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[0], interval1);
    // Check detectors moved correctly as a result of root rotation
    // Detector at x=1,y=0,z=0 rotated around root at x=0,y=0,z=0 with rotation
    // vector y=1, 90 degrees
    TS_ASSERT(
        mergeDetectorInfo.position({0, 0}).isApprox(Eigen::Vector3d{0, 0, -1}));
    // Detector at x=1,y=0,z=0 rotated around root at x=0,y=0,z=0 with rotation
    // vector y=1, -90 degrees
    TS_ASSERT(
        mergeDetectorInfo.position({0, 1}).isApprox(Eigen::Vector3d{0, 0, 1}));
  }

  void test_merge_root_multiple() {
    auto infos1 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos2 = makeFlatTree(PosVec(1), RotVec(1));
    auto infos3 = makeFlatTree(PosVec(1), RotVec(1));
    ComponentInfo &a = *std::get<0>(infos1);
    ComponentInfo &b = *std::get<0>(infos2);
    ComponentInfo &c = *std::get<0>(infos3);
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    Eigen::Vector3d pos3(3, 0, 0);
    a.setPosition(a.root(), pos1);
    b.setPosition(b.root(), pos2);
    c.setPosition(c.root(), pos3);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    std::pair<int64_t, int64_t> interval3(2, 3);
    a.setScanInterval(interval1);
    b.setScanInterval(interval2);
    c.setScanInterval(interval3);
    b.merge(c); // Execute the merge
    a.merge(b); // Merge again
    TS_ASSERT(a.isScanning());
    TS_ASSERT_EQUALS(a.size(), 2);
    TS_ASSERT_EQUALS(a.scanSize(), 2 * 3);
    TS_ASSERT_EQUALS(a.scanCount(), 3);
    // Note that the order is not guaranteed, currently these are just in the
    // order in which the are merged.
    auto index1 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 0 /*time index*/);
    auto index2 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 1 /*time index*/);
    auto index3 =
        std::pair<size_t, size_t>(a.root() /*static index*/, 2 /*time index*/);
    TS_ASSERT_EQUALS(a.scanIntervals()[index1.second], interval1);
    TS_ASSERT_EQUALS(a.scanIntervals()[index2.second], interval2);
    TS_ASSERT_EQUALS(a.scanIntervals()[index3.second], interval3);
    TS_ASSERT_EQUALS(a.position(index1), pos1);
    TS_ASSERT_EQUALS(a.position(index2), pos2);
    TS_ASSERT_EQUALS(a.position(index3), pos3);

    // Test Detector info is synched internally
    const DetectorInfo &mergeDetectorInfo = *std::get<1>(infos1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanCount(), 1 * 3);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[0], interval1);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[1], interval2);
    TS_ASSERT_EQUALS(mergeDetectorInfo.scanIntervals()[2], interval3);
  }
};
#endif /* MANTID_BEAMLINE_COMPONENTINFOTEST_H_ */
