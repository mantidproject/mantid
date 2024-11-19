// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/PeakShapeDetectorBin.h"
#include "MantidJson/Json.h"
#include "MantidKernel/cow_ptr.h"
#include <json/json.h>
#include <vector>

using Mantid::DataObjects::PeakShapeDetectorBin;
using Mantid::Kernel::SpecialCoordinateSystem;
using namespace Mantid;
using namespace Mantid::Kernel;

class PeakShapeDetectorBinTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeDetectorBinTest *createSuite() { return new PeakShapeDetectorBinTest(); }
  static void destroySuite(PeakShapeDetectorBinTest *suite) { delete suite; }

  void test_constructor() {
    std::vector<std::tuple<int32_t, double, double>> detPeakBinList = {
        {100, 20.55, 40.52}, {102, 33.0, 55.67}, {104, 50.9, 70.5}};
    std::string algorithmName = "TestSuite";
    int version = 1;
    auto coordinateSys = SpecialCoordinateSystem::None;

    std::shared_ptr<DataObjects::PeakShapeBase> peakShape =
        std::make_shared<PeakShapeDetectorBin>(detPeakBinList, coordinateSys, algorithmName, version);

    TS_ASSERT_EQUALS(algorithmName, peakShape->algorithmName());
    TS_ASSERT_EQUALS(version, peakShape->algorithmVersion());
    TS_ASSERT_EQUALS(coordinateSys, peakShape->frame());
    TS_ASSERT_EQUALS("PeakShapeDetectorBin", peakShape->shapeName());
    TS_ASSERT_EQUALS(std::nullopt, peakShape->radius(Mantid::Geometry::PeakShape::RadiusType::Radius));
    TS_ASSERT_EQUALS(std::dynamic_pointer_cast<PeakShapeDetectorBin>(peakShape)->getDetectorBinList(), detPeakBinList);

    std::shared_ptr<Mantid::Geometry::PeakShape> cloneShape(peakShape->clone());

    TS_ASSERT_EQUALS(algorithmName, cloneShape->algorithmName());
    TS_ASSERT_EQUALS(version, cloneShape->algorithmVersion());
    TS_ASSERT_EQUALS(coordinateSys, cloneShape->frame());
    TS_ASSERT_EQUALS("PeakShapeDetectorBin", cloneShape->shapeName());
    TS_ASSERT_EQUALS(std::nullopt, cloneShape->radius(Mantid::Geometry::PeakShape::RadiusType::Radius));
    TS_ASSERT_EQUALS(std::dynamic_pointer_cast<PeakShapeDetectorBin>(cloneShape)->getDetectorBinList(), detPeakBinList);
  }

  void test_json_serialization() {
    std::vector<std::tuple<int32_t, double, double>> detPeakBinList = {
        {100, 20.55, 40.52}, {102, 33.0, 55.67}, {104, 50.9, 70.5}};
    std::shared_ptr<DataObjects::PeakShapeBase> peakShape =
        std::make_shared<PeakShapeDetectorBin>(detPeakBinList, SpecialCoordinateSystem::None, "TestSuite", 1);

    std::string jsonStr = peakShape->toJSON();
    Json::Value output;
    TSM_ASSERT("Should parse as JSON", Mantid::JsonHelpers::parse(jsonStr, &output));
    TS_ASSERT_EQUALS("PeakShapeDetectorBin", output["shape"].asString());
    TS_ASSERT_EQUALS("TestSuite", output["algorithm_name"].asString());
    TS_ASSERT_EQUALS(1, output["algorithm_version"].asInt());
    TS_ASSERT_EQUALS(0, output["frame"].asInt());

    Json::Value detectorBinList = output["detectors"];
    TS_ASSERT_EQUALS(detectorBinList[0]["detId"].asInt(), 100);
    TS_ASSERT_EQUALS(detectorBinList[0]["startX"].asDouble(), 20.55);
    TS_ASSERT_EQUALS(detectorBinList[0]["endX"].asDouble(), 40.52);

    TS_ASSERT_EQUALS(detectorBinList[1]["detId"].asInt(), 102);
    TS_ASSERT_EQUALS(detectorBinList[1]["startX"].asDouble(), 33.0);
    TS_ASSERT_EQUALS(detectorBinList[1]["endX"].asDouble(), 55.67);

    TS_ASSERT_EQUALS(detectorBinList[2]["detId"].asInt(), 104);
    TS_ASSERT_EQUALS(detectorBinList[2]["startX"].asDouble(), 50.9);
    TS_ASSERT_EQUALS(detectorBinList[2]["endX"].asDouble(), 70.5);
  }

  void test_constructor_throws() {
    std::vector<std::tuple<int32_t, double, double>> detPeakBinList = {};

    TSM_ASSERT_THROWS("Should throw, bad directions",
                      PeakShapeDetectorBin(detPeakBinList, SpecialCoordinateSystem::None, "test", 1);
                      , std::invalid_argument &);
  }

  void test_copy_constructor() {
    std::vector<std::tuple<int32_t, double, double>> detPeakBinList = {{100, 10, 50}, {200, 34, 55}};
    PeakShapeDetectorBin shape1(detPeakBinList, SpecialCoordinateSystem::None, "test", 1);
    PeakShapeDetectorBin shape2(shape1);

    TS_ASSERT_EQUALS(shape1.getDetectorBinList(), detPeakBinList);
    TS_ASSERT_EQUALS(shape2.getDetectorBinList(), detPeakBinList);
    TS_ASSERT_EQUALS(shape1, shape2);
  }

  void test_assignment() {
    std::vector<std::tuple<int32_t, double, double>> detPeakBinList1 = {{100, 10, 50}, {200, 34, 55}};
    PeakShapeDetectorBin shape1(detPeakBinList1, SpecialCoordinateSystem::None, "test", 1);

    std::vector<std::tuple<int32_t, double, double>> detPeakBinList2 = {{500, 68, 77}};
    PeakShapeDetectorBin shape2(detPeakBinList2, SpecialCoordinateSystem::None, "test", 1);

    shape2 = shape1;

    TS_ASSERT_EQUALS(shape2.getDetectorBinList(), shape1.getDetectorBinList());
    TS_ASSERT_EQUALS(shape2.toJSON(), shape1.toJSON());
  }
};
