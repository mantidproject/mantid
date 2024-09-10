// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidKernel/MDUnit.h"

#include <boost/make_shared.hpp>
#include <optional>

using namespace Mantid::Geometry;

class MDFramesToSpecialCoordinateSystemTest : public CxxTest::TestSuite {
public:
  void test_that_throws_for_non_md_workspace() {
    // Arrange
    const std::shared_ptr<MatrixWorkspace> ws = std::make_shared<WorkspaceTester>();
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;
    // Act + Assert
    TSM_ASSERT_THROWS("Should throw as only MDEvent and MDHisto workspaces are allowed", converter(ws.get()),
                      const std::invalid_argument &);
  }

  void test_that_throws_for_non_uniform_Q_coodinate_system() {
    // Arrange
    Mantid::Geometry::QLab frame1;
    Mantid::Geometry::QSample frame2;
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("QLabX", "QLabX", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("QSampleY", "QSampleY", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    TSM_ASSERT_THROWS("Should throw as coordinate system is mixed with several Q types.", converter(ws.get()),
                      const std::invalid_argument &);
  }

  void test_that_doesn_not_throw_for_non_uniform_Q_coodinate_system() {
    // Arrange
    Mantid::Geometry::QLab frame1;
    Mantid::Geometry::GeneralFrame frame2("test", "Test");
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("QLabX", "QLabX", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("General Frame", "General Frame", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem;
    TSM_ASSERT_THROWS_NOTHING("Should throw nothing as coordinate system is "
                              "mixed only with one Q type.",
                              coordinateSystem = converter(ws.get()));

    TSM_ASSERT_EQUALS("Should be Qlab", *coordinateSystem, Mantid::Kernel::SpecialCoordinateSystem::QLab);
  }

  void test_that_returns_correct_equivalent_special_coordinate_system_for_QLab() {
    // Arrange
    Mantid::Geometry::QLab frame1;
    Mantid::Geometry::QLab frame2;
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("QLabX", "QLabX", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("QLabY", "QLabY", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem;
    TS_ASSERT_THROWS_NOTHING(coordinateSystem = converter(ws.get()));
    TSM_ASSERT_EQUALS("Should be Qlab", *coordinateSystem, Mantid::Kernel::SpecialCoordinateSystem::QLab);
  }

  void test_that_returns_correct_equivalent_special_coordinate_system_for_QSample() {
    // Arrange
    Mantid::Geometry::QSample frame1;
    Mantid::Geometry::QSample frame2;
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("QSampleX", "QSampleX", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("QSampleY", "QSampleY", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem;
    TS_ASSERT_THROWS_NOTHING(coordinateSystem = converter(ws.get()));
    TSM_ASSERT_EQUALS("Should be QSample", *coordinateSystem, Mantid::Kernel::SpecialCoordinateSystem::QSample);
  }

  void test_that_returns_correct_equivalent_special_coordinate_system_for_HKL() {
    // Arrange
    Mantid::Geometry::HKL frame1(new Mantid::Kernel::ReciprocalLatticeUnit);
    Mantid::Geometry::HKL frame2(new Mantid::Kernel::ReciprocalLatticeUnit);
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("H", "H", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("K", "K", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem;
    TS_ASSERT_THROWS_NOTHING(coordinateSystem = converter(ws.get()));
    TSM_ASSERT_EQUALS("Should be HKL", *coordinateSystem, Mantid::Kernel::SpecialCoordinateSystem::HKL);
  }

  void test_that_returns_correct_equivalent_special_coordinate_system_for_GeneralFrame() {
    // Arrange
    Mantid::Geometry::GeneralFrame frame1("a", "b");
    Mantid::Geometry::GeneralFrame frame2("a", "b");
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("H", "H", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("K", "K", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem;
    TS_ASSERT_THROWS_NOTHING(coordinateSystem = converter(ws.get()));
    TSM_ASSERT_EQUALS("Should be None", *coordinateSystem, Mantid::Kernel::SpecialCoordinateSystem::None);
  }

  void test_that_returns_empty_optional_when_UnknownFrame_detected() {
    // Arrange
    Mantid::Geometry::UnknownFrame frame1("b");
    Mantid::Geometry::UnknownFrame frame2("b");
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = std::make_shared<MDHistoDimension>("H", "H", frame1, min, max, bins);
    auto dimension2 = std::make_shared<MDHistoDimension>("K", "K", frame2, min, max, bins);
    auto ws = std::make_shared<Mantid::DataObjects::MDHistoWorkspace>(dimension1, dimension2);
    Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;

    // Act + Assert
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem;
    TS_ASSERT_THROWS_NOTHING(coordinateSystem = converter(ws.get()));
    TSM_ASSERT("Should not be initialized", !coordinateSystem);
  }
};
