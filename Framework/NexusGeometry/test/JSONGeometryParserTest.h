// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/JSONGeometryParserTestHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/Strings.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"
#include <cxxtest/TestSuite.h>
#include <exception>
#include <json/json.h>
#include <string>

using namespace Mantid::Kernel;
using Mantid::detid_t;
using Mantid::NexusGeometry::JSONGeometryParser;

class JSONGeometryParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JSONGeometryParserTest *createSuite() { return new JSONGeometryParserTest(); }
  static void destroySuite(JSONGeometryParserTest *suite) { delete suite; }

  void test_parse_fail_with_empty_JSON_string() {
    std::string json;
    attemptParseInvalidArgument(json, "Empty geometry JSON string provided.");
  }

  void test_parse_fail_with_no_nexus_structure_in_JSON() {
    std::string json = "{}";
    attemptParseInvalidArgument(json, "JSON geometry does not contain nexus_structure.");
  }

  void test_parse_fail_with_no_child_entry_in_JSON() {
    std::string json = "{\"nexus_structure\": { \"children\":[]}}";
    attemptParseInvalidArgument(json, "No nexus \"entry\" child found in nexus_structure JSON.");
  }

  void test_parse_fail_with_no_sample_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoSample();
    attemptParseInvalidArgument(json, "No sample found in JSON.");
  }

  void test_parse_fail_with_no_instrument_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoInstrument();
    attemptParseInvalidArgument(json, "No instrument found in JSON.");
  }

  void test_parse_fail_with_no_detectors_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoDetectors();
    attemptParseInvalidArgument(json, "No detectors found in JSON.");
  }

  void test_parse_fail_if_no_detector_ids_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoDetectorIDs();
    attemptParseInvalidArgument(json, "No detector ids found in detector_1.");
  }

  void test_parse_fail_if_no_x_pixel_offset_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoXPixelOffset();
    attemptParseInvalidArgument(json, "No x_pixel_offsets found in detector_1.");
  }

  void test_parse_fail_if_no_y_pixel_offset_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoYPixelOffset();
    attemptParseInvalidArgument(json, "No y_pixel_offsets found in detector_1.");
  }

  void test_parse_fail_if_no_pixel_shape_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryNoPixelShape();
    attemptParseInvalidArgument(json, "Insufficient pixel shape information found in detector_1.");
  }

  void test_parse_fail_for_empty_off_geometry_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryEmptyOffGeometry();
    attemptParseInvalidArgument(json, "Insufficient pixel shape information found in detector_1.");
  }

  void test_parse_fail_for_invalid_off_geometry_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryInvalidOffGeometry();
    attemptParseInvalidArgument(json, "Invalid off geometry provided in JSON pixel_shape.");
  }

  void test_parse_fail_for_empty_cylindrical_geometry_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryEmptyCylindricalGeometry();
    attemptParseInvalidArgument(json, "Insufficient pixel shape information found in detector_1.");
  }

  void test_parse_fail_for_invalid_cylindrical_geometry_in_JSON() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryInvalidCylindricalGeometry();
    attemptParseInvalidArgument(json, "Invalid cylindrical geometry provided in JSON pixel_shape.");
  }

  void test_parse_fail_for_missing_transformation_dependency() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryMissingTransformations();
    attemptParseInvalidArgument(json, "Could not find dependency "
                                      "/entry/instrument/detector_1/"
                                      "transformations/location in JSON "
                                      "provided.");
  }

  void test_parse_fail_for_missing_transformation_beam_direction_offset() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryMissingBeamDirectionOffset();
    attemptParseInvalidArgument(json, "Could not find dependency "
                                      "/entry/instrument/detector_1/"
                                      "transformations/beam_direction_offset "
                                      "in JSON provided.");
  }

  void test_parse_fail_for_missing_transformation_orientation() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryMissingOrientation();
    attemptParseInvalidArgument(json, "Could not find dependency "
                                      "/entry/instrument/detector_1/"
                                      "transformations/orientation in JSON "
                                      "provided.");
  }

  void test_parse_fail_for_empty_chopper() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryMissingChopperInformation();
    attemptParseInvalidArgument(json, "Full chopper definition missing in JSON provided.");
  }

  void test_parse_fail_for_empty_monitor() {
    std::string json = Mantid::FrameworkTestHelpers::getJSONGeometryMissingMonitorInformation();
    attemptParseInvalidArgument(json, "Full monitor definition for monitor_1 missing in JSON provided.");
  }

  void test_load_full_instrument_simple_off_pixel_shape() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleOFF();
    JSONGeometryParser parser(json);
    TS_ASSERT_EQUALS(parser.name(), "SimpleInstrument");
    TS_ASSERT_EQUALS(parser.sampleName(), "sample");
    TS_ASSERT_EQUALS(parser.samplePosition(), Eigen::Vector3d(0, 0, 0));
    auto angleAxis = Eigen::AngleAxisd(parser.sampleOrientation());
    TS_ASSERT_EQUALS(angleAxis.angle(), 0);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(1, 0, 0));
    TS_ASSERT_EQUALS(parser.sourceName(), "Unspecified");
    TS_ASSERT_EQUALS(parser.numberOfBanks(), 1);
    TS_ASSERT_EQUALS(parser.detectorName(0), "detector_1");
    const auto &detIDs = parser.detectorIDs(0);
    TS_ASSERT_EQUALS(detIDs.size(), 4);
    TS_ASSERT((detIDs == std::vector<detid_t>{1, 2, 3, 4}));
    const auto &x = parser.xPixelOffsets(0);
    TS_ASSERT((x == std::vector<double>{-0.299, -0.297, -0.299, -0.297}));
    const auto &y = parser.yPixelOffsets(0);
    TS_ASSERT((y == std::vector<double>{-0.299, -0.299, -0.297, -0.297}));
    const auto &translation = parser.translation(0);
    TS_ASSERT_EQUALS(translation, Eigen::Vector3d(0.971, 0, -0.049));
    angleAxis = Eigen::AngleAxisd(parser.orientation(0));
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(90), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT(parser.isOffGeometry(0));
    std::vector<Eigen::Vector3d> testVerticesVec{
        {-0.001, -0.001, 0}, {0.001, -0.001, 0}, {0.001, 0.001, 0}, {-0.001, 0.001, 0}};
    assertVectors(parser.vertices(0), testVerticesVec);
    std::vector<uint32_t> testFacesVec{0};
    TS_ASSERT_EQUALS(testFacesVec, parser.faces(0));
    std::vector<uint32_t> testWindingOrderVec{0, 1, 2, 3};
    TS_ASSERT_EQUALS(testWindingOrderVec, parser.windingOrder(0));
    const auto &cylinders = parser.cylinders(0);
    TS_ASSERT(cylinders.empty());
  }

  void test_load_full_instrument_simple_with_source() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleWithSource();
    JSONGeometryParser parser(json);
    TS_ASSERT_EQUALS(parser.name(), "SimpleInstrument");
    TS_ASSERT_EQUALS(parser.sampleName(), "sample");
    TS_ASSERT_EQUALS(parser.samplePosition(), Eigen::Vector3d(0, 0, 0));
    auto angleAxis = Eigen::AngleAxisd(parser.sampleOrientation());
    TS_ASSERT_EQUALS(angleAxis.angle(), 0);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(1, 0, 0));
    TS_ASSERT_EQUALS(parser.sourceName(), "moderator");
    TS_ASSERT_EQUALS(parser.sourcePosition(), Eigen::Vector3d(0, 0, -28.900002));
    TS_ASSERT_EQUALS(parser.numberOfBanks(), 1);
    TS_ASSERT_EQUALS(parser.detectorName(0), "detector_1");
    const auto &detIDs = parser.detectorIDs(0);
    TS_ASSERT_EQUALS(detIDs.size(), 4);
    TS_ASSERT((detIDs == std::vector<detid_t>{1, 2, 3, 4}));
    const auto &x = parser.xPixelOffsets(0);
    TS_ASSERT((x == std::vector<double>{-0.299, -0.297, -0.299, -0.297}));
    const auto &y = parser.yPixelOffsets(0);
    TS_ASSERT((y == std::vector<double>{-0.299, -0.299, -0.297, -0.297}));
    const auto &translation = parser.translation(0);
    TS_ASSERT_EQUALS(translation, Eigen::Vector3d(0.971, 0, -0.049));
    angleAxis = Eigen::AngleAxisd(parser.orientation(0));
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(90), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT(parser.isOffGeometry(0));
    std::vector<Eigen::Vector3d> testVerticesVec{
        {-0.001, -0.001, 0}, {0.001, -0.001, 0}, {0.001, 0.001, 0}, {-0.001, 0.001, 0}};
    assertVectors(parser.vertices(0), testVerticesVec);
    std::vector<uint32_t> testFacesVec{0};
    TS_ASSERT_EQUALS(testFacesVec, parser.faces(0));
    std::vector<uint32_t> testWindingOrderVec{0, 1, 2, 3};
    TS_ASSERT_EQUALS(testWindingOrderVec, parser.windingOrder(0));
    const auto &cylinders = parser.cylinders(0);
    TS_ASSERT(cylinders.empty());
  }

  void test_load_full_instrument_simple_cylindrical_pixel_shape() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleCylindrical();
    JSONGeometryParser parser(json);
    TS_ASSERT_EQUALS(parser.name(), "SimpleInstrument");
    TS_ASSERT_EQUALS(parser.numberOfBanks(), 1);
    TS_ASSERT_EQUALS(parser.detectorName(0), "detector_1");
    const auto &detIDs = parser.detectorIDs(0);
    TS_ASSERT_EQUALS(detIDs.size(), 4);
    TS_ASSERT((detIDs == std::vector<detid_t>{1, 2, 3, 4}));
    const auto &x = parser.xPixelOffsets(0);
    TS_ASSERT((x == std::vector<double>{-0.299, -0.297, -0.299, -0.297}));
    const auto &y = parser.yPixelOffsets(0);
    TS_ASSERT((y == std::vector<double>{-0.299, -0.299, -0.297, -0.297}));
    const auto &translation = parser.translation(0);
    TS_ASSERT_EQUALS(translation, Eigen::Vector3d(0.971, 0, -0.049));
    auto angleAxis = Eigen::AngleAxisd(parser.orientation(0));
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(90), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT(!parser.isOffGeometry(0));
    std::vector<Eigen::Vector3d> testVerticesVec{{-0.001, 0, 0}, {0.001, 0.00405, 0}, {0.001, 0, 0}};
    assertVectors(parser.vertices(0), testVerticesVec);
    std::vector<uint32_t> testCylindersVec{0, 1, 2};
    TS_ASSERT_EQUALS(parser.cylinders(0), testCylindersVec);
    const auto &windingOrder = parser.windingOrder(0);
    TS_ASSERT(windingOrder.empty());
    const auto &faces = parser.faces(0);
    TS_ASSERT(faces.empty());
  }

  void test_load_full_instrument_simple_with_single_chopper() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleWithChopper();
    JSONGeometryParser parser(json);
    TS_ASSERT_EQUALS(parser.name(), "SimpleInstrument");
    TS_ASSERT_EQUALS(parser.numberOfBanks(), 1);
    // validate choppers
    const auto &choppers = parser.choppers();
    TS_ASSERT_EQUALS(choppers.size(), 1);
    const auto &chopper = choppers[0];
    TS_ASSERT_EQUALS(chopper.componentName, "chopper_1");
    TS_ASSERT_EQUALS(chopper.name, "Airbus, Source Chopper, ESS Pulse, Disc 1");
    TS_ASSERT_DELTA(chopper.radius, 350.0, TOLERANCE);
    TS_ASSERT_DELTA(chopper.slitHeight, 150.0, TOLERANCE);
    TS_ASSERT_EQUALS(chopper.slits, 1);
    TS_ASSERT_EQUALS(chopper.slitEdges, (std::vector<double>{0.0, 23.0}));
    TS_ASSERT_EQUALS(chopper.tdcSource, "HZB-V20:Chop-Drv-0401:TDC_array");
    TS_ASSERT_EQUALS(chopper.tdcTopic, "V20_choppers");
    TS_ASSERT_EQUALS(chopper.tdcWriterModule, "senv");
    // validate detectors
    TS_ASSERT_EQUALS(parser.detectorName(0), "detector_1");
    const auto &detIDs = parser.detectorIDs(0);
    TS_ASSERT_EQUALS(detIDs.size(), 4);
    TS_ASSERT((detIDs == std::vector<detid_t>{1, 2, 3, 4}));
    const auto &x = parser.xPixelOffsets(0);
    TS_ASSERT((x == std::vector<double>{-0.299, -0.297, -0.299, -0.297}));
    const auto &y = parser.yPixelOffsets(0);
    TS_ASSERT((y == std::vector<double>{-0.299, -0.299, -0.297, -0.297}));
    const auto &translation = parser.translation(0);
    TS_ASSERT_EQUALS(translation, Eigen::Vector3d(0.971, 0, -0.049));
    auto angleAxis = Eigen::AngleAxisd(parser.orientation(0));
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(90), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT(!parser.isOffGeometry(0));
    std::vector<Eigen::Vector3d> testVerticesVec{{-0.001, 0, 0}, {0.001, 0.00405, 0}, {0.001, 0, 0}};
    assertVectors(parser.vertices(0), testVerticesVec);
    std::vector<uint32_t> testCylindersVec{0, 1, 2};
    TS_ASSERT_EQUALS(parser.cylinders(0), testCylindersVec);
    const auto &windingOrder = parser.windingOrder(0);
    TS_ASSERT(windingOrder.empty());
    const auto &faces = parser.faces(0);
    TS_ASSERT(faces.empty());
  }

  void test_load_full_instrument_with_single_monitor_without_shape() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleWithMonitorNoShape();
    JSONGeometryParser parser(json);
    TS_ASSERT_EQUALS(parser.name(), "SimpleInstrument");
    TS_ASSERT_EQUALS(parser.numberOfBanks(), 1);
    // validate monitors
    const auto &monitors = parser.monitors();
    TS_ASSERT_EQUALS(monitors.size(), 1);
    const auto &monitor = monitors[0];
    TS_ASSERT_EQUALS(monitor.componentName, "monitor_1");
    TS_ASSERT_EQUALS(monitor.detectorID, 90000);
    TS_ASSERT_EQUALS(monitor.name, "Helium-3 monitor");
    TS_ASSERT_EQUALS(monitor.translation, Eigen::Vector3d(0, 0, -3.298));
    auto angleAxis = Eigen::AngleAxisd(monitor.orientation);
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(45), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT_EQUALS(monitor.eventStreamTopic, "monitor");
    TS_ASSERT_EQUALS(monitor.eventStreamSource, "Monitor_Adc0_Ch1");
    TS_ASSERT_EQUALS(monitor.eventStreamWriterModule, "ev42");
    TS_ASSERT_EQUALS(monitor.waveformTopic, "monitor");
    TS_ASSERT_EQUALS(monitor.waveformSource, "Monitor_Adc0_Ch1");
    TS_ASSERT_EQUALS(monitor.waveformWriterModule, "senv");
    TS_ASSERT_EQUALS(monitor.cylinders.size(), 0);
    TS_ASSERT_EQUALS(monitor.faces.size(), 0);
    TS_ASSERT_EQUALS(monitor.windingOrder.size(), 0);
    TS_ASSERT_EQUALS(monitor.vertices.size(), 0);

    // validate detectors
    TS_ASSERT_EQUALS(parser.detectorName(0), "detector_1");
    const auto &detIDs = parser.detectorIDs(0);
    TS_ASSERT_EQUALS(detIDs.size(), 4);
    TS_ASSERT((detIDs == std::vector<detid_t>{1, 2, 3, 4}));
    const auto &x = parser.xPixelOffsets(0);
    TS_ASSERT((x == std::vector<double>{-0.299, -0.297, -0.299, -0.297}));
    const auto &y = parser.yPixelOffsets(0);
    TS_ASSERT((y == std::vector<double>{-0.299, -0.299, -0.297, -0.297}));
    const auto &translation = parser.translation(0);
    TS_ASSERT_EQUALS(translation, Eigen::Vector3d(0.971, 0, -0.049));
    angleAxis = Eigen::AngleAxisd(parser.orientation(0));
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(90), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT(!parser.isOffGeometry(0));
    std::vector<Eigen::Vector3d> testVerticesVec{{-0.001, 0, 0}, {0.001, 0.00405, 0}, {0.001, 0, 0}};
    assertVectors(parser.vertices(0), testVerticesVec);
    std::vector<uint32_t> testCylindersVec{0, 1, 2};
    TS_ASSERT_EQUALS(parser.cylinders(0), testCylindersVec);
    const auto &windingOrder = parser.windingOrder(0);
    TS_ASSERT(windingOrder.empty());
    const auto &faces = parser.faces(0);
    TS_ASSERT(faces.empty());
  }

  void test_load_full_instrument_with_single_monitor_with_shape() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleWithMonitor();
    JSONGeometryParser parser(json);
    TS_ASSERT_EQUALS(parser.name(), "SimpleInstrument");
    TS_ASSERT_EQUALS(parser.numberOfBanks(), 1);
    // validate monitors
    const auto &monitors = parser.monitors();
    TS_ASSERT_EQUALS(monitors.size(), 1);
    const auto &monitor = monitors[0];
    TS_ASSERT_EQUALS(monitor.componentName, "monitor_1");
    TS_ASSERT_EQUALS(monitor.detectorID, 90000);
    TS_ASSERT_EQUALS(monitor.name, "Helium-3 monitor");
    TS_ASSERT_EQUALS(monitor.translation, Eigen::Vector3d(0, 0, -3.298));
    auto angleAxis = Eigen::AngleAxisd(monitor.orientation);
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(45), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT_EQUALS(monitor.eventStreamTopic, "monitor");
    TS_ASSERT_EQUALS(monitor.eventStreamSource, "Monitor_Adc0_Ch1");
    TS_ASSERT_EQUALS(monitor.eventStreamWriterModule, "ev42");
    TS_ASSERT_EQUALS(monitor.waveformTopic, "monitor");
    TS_ASSERT_EQUALS(monitor.waveformSource, "Monitor_Adc0_Ch1");
    TS_ASSERT_EQUALS(monitor.waveformWriterModule, "senv");
    TS_ASSERT_EQUALS(monitor.isOffGeometry, false);
    TS_ASSERT_EQUALS(monitor.cylinders.size(), 3);
    TS_ASSERT_EQUALS(monitor.faces.size(), 0);
    TS_ASSERT_EQUALS(monitor.windingOrder.size(), 0);
    TS_ASSERT_EQUALS(monitor.vertices.size(), 3);

    // validate detectors
    TS_ASSERT_EQUALS(parser.detectorName(0), "detector_1");
    const auto &detIDs = parser.detectorIDs(0);
    TS_ASSERT_EQUALS(detIDs.size(), 4);
    TS_ASSERT((detIDs == std::vector<detid_t>{1, 2, 3, 4}));
    const auto &x = parser.xPixelOffsets(0);
    TS_ASSERT((x == std::vector<double>{-0.299, -0.297, -0.299, -0.297}));
    const auto &y = parser.yPixelOffsets(0);
    TS_ASSERT((y == std::vector<double>{-0.299, -0.299, -0.297, -0.297}));
    const auto &translation = parser.translation(0);
    TS_ASSERT_EQUALS(translation, Eigen::Vector3d(0.971, 0, -0.049));
    angleAxis = Eigen::AngleAxisd(parser.orientation(0));
    TS_ASSERT_DELTA(angleAxis.angle(), parser.degreesToRadians(90), TOLERANCE);
    TS_ASSERT_EQUALS(angleAxis.axis(), Eigen::Vector3d(0, 1, 0));
    TS_ASSERT(!parser.isOffGeometry(0));
    std::vector<Eigen::Vector3d> testVerticesVec{{-0.001, 0, 0}, {0.001, 0.00405, 0}, {0.001, 0, 0}};
    assertVectors(parser.vertices(0), testVerticesVec);
    std::vector<uint32_t> testCylindersVec{0, 1, 2};
    TS_ASSERT_EQUALS(parser.cylinders(0), testCylindersVec);
    const auto &windingOrder = parser.windingOrder(0);
    TS_ASSERT(windingOrder.empty());
    const auto &faces = parser.faces(0);
    TS_ASSERT(faces.empty());
  }

  void test_load_full_instrument_with_z_pixel_offset() {
    std::string json = Mantid::FrameworkTestHelpers::getFullJSONInstrumentSimpleWithZPixelOffset();
    JSONGeometryParser parser(json);
    const auto &zPixelOffsets = parser.zPixelOffsets(0);
    TS_ASSERT_EQUALS(zPixelOffsets.size(), 4);
    TS_ASSERT_EQUALS(zPixelOffsets, (std::vector<double>{-0.0405, -0.0405, -0.0405, -0.0405}));
  }

private:
  const double TOLERANCE = 1e-5;
  std::string getSimpleJSONInstrument() { return ""; }

  void attemptParseInvalidArgument(const std::string &json, const std::string &expectedError) {
    TS_ASSERT_THROWS_EQUALS((JSONGeometryParser(json)), const std::invalid_argument &e, std::string(e.what()),
                            expectedError);
  }

  void assertVectors(const std::vector<Eigen::Vector3d> &lhs, const std::vector<Eigen::Vector3d> &rhs) {
    TS_ASSERT(lhs.size() == rhs.size());
    for (size_t i = 0; i < lhs.size(); ++i)
      assertVertices(lhs[i], rhs[i]);
  }

  void assertVertices(const Eigen::Vector3d &lhs, const Eigen::Vector3d &rhs) {
    TS_ASSERT_DELTA(lhs.x(), rhs.x(), TOLERANCE);
    TS_ASSERT_DELTA(lhs.y(), rhs.y(), TOLERANCE);
    TS_ASSERT_DELTA(lhs.z(), rhs.z(), TOLERANCE);
  }
};

class JSONGeometryParserTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JSONGeometryParserTestPerformance *createSuite() { return new JSONGeometryParserTestPerformance(); }
  static void destroySuite(JSONGeometryParserTestPerformance *suite) { delete suite; }

  void setUp() override {
    std::string filename = "V20_file_write_start_20190524.json";
    std::string fullPath = ConfigService::Instance().getFullPath(filename, true, Glob::GLOB_DEFAULT);

    instrument = Strings::loadFile(fullPath);
  }

  void test_parse() { TS_ASSERT_THROWS_NOTHING((JSONGeometryParser(instrument))); }

private:
  std::string instrument;
};
