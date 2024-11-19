// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <json/json.h>

#include "MantidDataObjects/PeakShapeDetectorBin.h"
#include "MantidDataObjects/PeakShapeDetectorBinFactory.h"
#include "MantidJson/Json.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/cow_ptr.h"
#include "MockObjects.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::DataObjects::PeakShapeDetectorBin;
using Mantid::Kernel::SpecialCoordinateSystem;

class PeakShapeDetectorBinFactoryTest : public CxxTest::TestSuite {
public:
  static PeakShapeDetectorBinFactoryTest *createSuite() { return new PeakShapeDetectorBinFactoryTest(); }
  static void destroySuite(PeakShapeDetectorBinFactoryTest *suite) { delete suite; }

  void test_invalid_json_with_no_successor() {
    PeakShapeDetectorBinFactory factory;
    TS_ASSERT_THROWS(factory.create(""), std::invalid_argument &);
  }

  void test_successor_calling_when_shape_is_unhandled() {
    using namespace testing;
    MockPeakShapeFactory *delegate = new MockPeakShapeFactory;
    EXPECT_CALL(*delegate, create(_)).Times(1);

    PeakShapeDetectorBinFactory factory;
    factory.setSuccessor(PeakShapeFactory_const_sptr(delegate));

    // Minimal valid JSON for describing the shape.
    Json::Value root;
    root["shape"] = "NotHandled";
    const std::string str_json = Mantid::JsonHelpers::jsonToString(root);

    TS_ASSERT_THROWS_NOTHING(factory.create(str_json));
    TS_ASSERT(Mock::VerifyAndClearExpectations(delegate));
  }

  void test_when_no_successor() {
    PeakShapeDetectorBinFactory factory;

    // Minimal valid JSON for describing the shape.
    Json::Value root;
    root["shape"] = "NotHandled";
    const std::string str_json = Mantid::JsonHelpers::jsonToString(root);

    TS_ASSERT_THROWS(factory.create(str_json), std::invalid_argument &);
  }

  void test_factory_create() {
    PeakShapeDetectorBin shape({{100, 10, 50}, {200, 34, 55}}, SpecialCoordinateSystem::None, "test", 1);

    PeakShapeDetectorBinFactory factory;
    std::shared_ptr<Mantid::Geometry::PeakShape> productShape(factory.create(shape.toJSON()));

    std::shared_ptr<PeakShapeDetectorBin> factoryShape = std::dynamic_pointer_cast<PeakShapeDetectorBin>(productShape);
    TS_ASSERT(factoryShape);

    TS_ASSERT_EQUALS(shape, *factoryShape);
    TS_ASSERT_EQUALS(factoryShape->getDetectorBinList(), shape.getDetectorBinList())
  }
};
