#ifndef MANTID_MDALGORITHMS_SETMDFRAMETEST_H_
#define MANTID_MDALGORITHMS_SETMDFRAMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/SetMDFrame.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidKernel/MDUnit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <utility>
#include <memory>
using Mantid::MDAlgorithms::SetMDFrame;

class SetMDFrameTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SetMDFrameTest *createSuite() { return new SetMDFrameTest(); }
  static void destroySuite(SetMDFrameTest *suite) { delete suite; }

  void test_Init() {
    SetMDFrame alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_that_is_not_executed_when_non_mdevent_and_non_mdhisto() {
    // Arrange
    auto inputWorkspace = WorkspaceCreationHelper::create2DWorkspace(2, 5);
    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    alg.setProperty("InputWorkspace", inputWorkspace);

    // Act + Assert
    TSM_ASSERT_THROWS_ANYTHING("Should not accept a MatrixWorkspace",
                               alg.execute());
  }

  void test_that_accepts_mdevent_and_mdhisto() {
    // Arrange
    auto eventType =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    auto histoType =
        Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace(
            1.0, 2, 10, 10.0, 1.0, "A");

    std::vector<int> axes;
    axes.push_back(1);

    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    auto dim0Selection = Mantid::Geometry::QLab::QLabName;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MDFrame", dim0Selection));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Axes", axes));
    // Act + Assert
    alg.setProperty("InputWorkspace", eventType);
    TSM_ASSERT_THROWS_NOTHING("Should accept an MDEvent workspace",
                              alg.execute());
    alg.setProperty("InputWorkspace", histoType);
    TSM_ASSERT_THROWS_NOTHING("Should accept an MDHisto workspace",
                              alg.execute());
  }

  void test_that_index_out_of_bounds_is_not_valid() {
    // Arrange
    auto eventType =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    auto histoType =
        Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace(
            1.0, 1, 10, 10.0, 1.0, "A");

    std::vector<int> axes;
    axes.push_back(0);
    axes.push_back(7);

    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    // Act + Assert
    auto dim0Selection = Mantid::Geometry::QLab::QLabName;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MDFrame", dim0Selection));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Axes", axes));
    alg.setProperty("InputWorkspace", eventType);
    TSM_ASSERT_THROWS_ANYTHING("Should not accept an index out of bounds",
                               alg.execute());
  }

  void test_that_can_set_to_QLab() {
    // Arrange
    const size_t numberOfDimensions = 2;
    std::vector<Mantid::Geometry::MDFrame_sptr> frames;
    frames.push_back(std::make_shared<Mantid::Geometry::UnknownFrame>("test"));
    frames.push_back(std::make_shared<Mantid::Geometry::UnknownFrame>("test"));
    auto inputWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithIndividualFrames<
            numberOfDimensions>(5, -2, 2, frames, 3);

    std::vector<int> axes;
    axes.push_back(0);

    // Act
    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputWorkspace);

    auto dim0Selection = Mantid::Geometry::QLab::QLabName;

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MDFrame", dim0Selection));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Axes", axes));
    TSM_ASSERT_THROWS_NOTHING("Should accept an MDEvent workspace",
                              alg.execute());

    // Assert
    auto dimension0 = inputWorkspace->getDimension(0);
    TSM_ASSERT_EQUALS("Should be a QLab frame", dimension0->getMDFrame().name(),
                      dim0Selection);
    auto dimension1 = inputWorkspace->getDimension(1);
    TSM_ASSERT_EQUALS("Should be an Unknown frame",
                      dimension1->getMDFrame().name(),
                      Mantid::Geometry::UnknownFrame::UnknownFrameName);
  }

  void test_that_cannot_set_to_HKL_when_units_are_correct_on_multiple_axes() {
    // Arrange
    const size_t numberOfDimensions = 2;
    std::vector<Mantid::Geometry::MDFrame_sptr> frames;

    // Create unit which is suitable for HKL
    auto unitFactory = Mantid::Kernel::makeMDUnitFactoryChain();
    std::string unitString0 = "in 2.6437 A^-1";
    std::string unitString1 = "in 1.6437 A^-1";
    auto unit0 = unitFactory->create(unitString0);
    auto unit1 = unitFactory->create(unitString1);

    frames.push_back(
        std::make_shared<Mantid::Geometry::UnknownFrame>(std::move(unit0)));
    frames.push_back(
        std::make_shared<Mantid::Geometry::UnknownFrame>(std::move(unit1)));
    auto inputWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithIndividualFrames<
            numberOfDimensions>(5, -2, 2, frames, 3);

    std::vector<int> axes;
    axes.push_back(0);
    axes.push_back(1);

    // Act
    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputWorkspace);

    auto dim0Selection = Mantid::Geometry::HKL::HKLName;

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MDFrame", dim0Selection));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Axes", axes));
    TSM_ASSERT_THROWS_NOTHING("Should accept an MDEvent workspace",
                              alg.execute());

    // Assert
    auto dimension0 = inputWorkspace->getDimension(0);
    TSM_ASSERT_EQUALS("Should be an HKL frame", dimension0->getMDFrame().name(),
                      dim0Selection);
    TSM_ASSERT_EQUALS("Should have the original units",
                      dimension0->getMDFrame().getUnitLabel(), unitString0);

    auto dimension1 = inputWorkspace->getDimension(1);
    TSM_ASSERT_EQUALS("Should be an HKL frame", dimension1->getMDFrame().name(),
                      dim0Selection);
    TSM_ASSERT_EQUALS("Should have the original units",
                      dimension1->getMDFrame().getUnitLabel(), unitString1);
  }

  void test_that_cannot_set_to_HKL_when_units_are_wrong() {
    // Arrange
    const size_t numberOfDimensions = 2;
    std::vector<Mantid::Geometry::MDFrame_sptr> frames;

    // Create unit which is unsuitable for HKL
    auto unitFactory = Mantid::Kernel::makeMDUnitFactoryChain();
    std::string unitString0 = "wrongUNits";
    std::string unitString1 = "wrongUnits";
    auto unit0 = unitFactory->create(unitString0);
    auto unit1 = unitFactory->create(unitString1);

    frames.push_back(
        std::make_shared<Mantid::Geometry::UnknownFrame>(std::move(unit0)));
    frames.push_back(
        std::make_shared<Mantid::Geometry::UnknownFrame>(std::move(unit1)));
    auto inputWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithIndividualFrames<
            numberOfDimensions>(5, -2, 2, frames, 3);

    std::vector<int> axes;
    axes.push_back(1);

    // Act
    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputWorkspace);

    auto dim0Selection = Mantid::Geometry::HKL::HKLName;

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MDFrame", dim0Selection));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Axes", axes));
    TSM_ASSERT_THROWS_ANYTHING("Should not accept a wrong HKL Units",
                               alg.execute());
  }

  void test_that_can_convert_to_GeneralFrame() {
    // Arrange
    const size_t numberOfDimensions = 2;
    std::vector<Mantid::Geometry::MDFrame_sptr> frames;

    // Create unit which is suitable for HKL
    auto unitFactory = Mantid::Kernel::makeMDUnitFactoryChain();
    std::string unitString0 = "in 2.6437 A^-1";
    std::string unitString1 = "in 1.6437 A^-1";
    auto unit0 = unitFactory->create(unitString0);
    auto unit1 = unitFactory->create(unitString1);

    frames.push_back(
        std::make_shared<Mantid::Geometry::UnknownFrame>(std::move(unit0)));
    frames.push_back(
        std::make_shared<Mantid::Geometry::UnknownFrame>(std::move(unit1)));
    auto inputWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithIndividualFrames<
            numberOfDimensions>(5, -2, 2, frames, 3);

    std::vector<int> axes;
    axes.push_back(0);
    axes.push_back(1);

    // Act
    SetMDFrame alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputWorkspace);

    auto dim0Selection = Mantid::Geometry::GeneralFrame::GeneralFrameName;

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MDFrame", dim0Selection));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Axes", axes));
    TSM_ASSERT_THROWS_NOTHING("Should accept an MDEvent workspace",
                              alg.execute());

    // Assert
    auto dimension0 = inputWorkspace->getDimension(0);
    TSM_ASSERT_EQUALS("Should be a General frame",
                      dimension0->getMDFrame().name(), dim0Selection);
    TSM_ASSERT_EQUALS("Should have the original units",
                      dimension0->getMDFrame().getUnitLabel(), unitString0);

    auto dimension1 = inputWorkspace->getDimension(1);
    TSM_ASSERT_EQUALS("Should be a General frame",
                      dimension1->getMDFrame().name(), dim0Selection);
    TSM_ASSERT_EQUALS("Should have the original units",
                      dimension1->getMDFrame().getUnitLabel(), unitString1);
  }
};

#endif /* MANTID_MDALGORITHMS_SetMDFrameTEST_H_ */
