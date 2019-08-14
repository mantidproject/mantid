// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef NEXUSGEOMETRYPARSERTEST_H_
#define NEXUSGEOMETRYPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include <H5Cpp.h>
#include <Poco/Glob.h>
#include <chrono>
#include <gmock/gmock.h>
#include <string>

using namespace Mantid;
using namespace NexusGeometry;
namespace {
std::unique_ptr<Geometry::DetectorInfo>
extractDetectorInfo(const Mantid::Geometry::Instrument &instrument) {
  Geometry::ParameterMap pmap;
  return std::move(std::get<1>(instrument.makeBeamline(pmap)));
}

std::pair<std::unique_ptr<Geometry::ComponentInfo>,
          std::unique_ptr<Geometry::DetectorInfo>>
extractBeamline(const Mantid::Geometry::Instrument &instrument) {
  Geometry::ParameterMap pmap;
  auto beamline = instrument.makeBeamline(pmap);
  return {std::move(std::get<0>(beamline)), std::move(std::get<1>(beamline))};
}

class MockLogger : public NexusGeometry::Logger {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(warning, void(const std::string &));
  MOCK_METHOD1(error, void(const std::string &));
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

} // namespace
class NexusGeometryParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometryParserTest *createSuite() {
    return new NexusGeometryParserTest();
  }
  static void destroySuite(NexusGeometryParserTest *suite) { delete suite; }

  std::unique_ptr<const Mantid::Geometry::Instrument> makeTestInstrument() {
    H5std_string nexusFilename = "unit_testing/SMALLFAKE_example_geometry.hdf5";
    const auto fullpath = Kernel::ConfigService::Instance().getFullPath(
        nexusFilename, true, Poco::Glob::GLOB_DEFAULT);

    return NexusGeometryParser::createInstrument(
        fullpath, std::make_unique<MockLogger>());
  }

  void test_basic_instrument_information() {
    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);

    TSM_ASSERT_EQUALS("Detectors + 1 monitor", detectorInfo->size(),
                      128 * 2 + 1);
    TSM_ASSERT_EQUALS("Detectors + 2 banks + 16 tubes + root + source + sample",
                      componentInfo->size(), detectorInfo->size() + 21);
    // Check 128 detectors in first bank
    TS_ASSERT_EQUALS(
        128,
        componentInfo->detectorsInSubtree(componentInfo->root() - 3).size());
    TS_ASSERT_EQUALS("rear-detector",
                     componentInfo->name(componentInfo->root() - 3));
    TS_ASSERT(Mantid::Kernel::toVector3d(
                  componentInfo->position(componentInfo->root() - 3))
                  .isApprox(Eigen::Vector3d{0, 0, 4}));
    // Check 128 detectors in second bank
    TS_ASSERT_EQUALS(
        128,
        componentInfo->detectorsInSubtree(componentInfo->root() - 12).size());
  }

  void test_source_is_where_expected() {
    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);

    auto sourcePosition =
        Kernel::toVector3d(componentInfo->position(componentInfo->source()));

    TS_ASSERT(sourcePosition.isApprox(Eigen::Vector3d(
        0, 0, -34.281))); // Check against fixed position in file
  }

  void test_simple_translation() {
    auto instrument = makeTestInstrument();
    auto detectorInfo = extractDetectorInfo(*instrument);
    // First pixel in bank 2
    auto det0Position = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(1100000)));
    Eigen::Vector3d unitVector(0, 0,
                               1); // Fixed in file location vector attribute
    const double magnitude = 4.0;  // Fixed in file location value
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    Eigen::Vector3d expectedDet0Position = offset + (magnitude * unitVector);
    TS_ASSERT(det0Position.isApprox(expectedDet0Position));

    // Test monitor position
    Mantid::detid_t monitorDetId = 1;
    auto mon0Position = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(monitorDetId)));
    // Sanity check that this is a monitor
    TS_ASSERT(detectorInfo->isMonitor(detectorInfo->indexOf(monitorDetId)));
    // Check against location in file
    TS_ASSERT(mon0Position.isApprox(Eigen::Vector3d{0, 0, -12.064}));
  }

  void test_complex_translation() {
    auto instrument = makeTestInstrument();
    auto detectorInfo = extractDetectorInfo(*instrument);
    // First pixel in bank 1

    auto det0Postion = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(2100000)));
    Eigen::Vector3d unitVectorTranslation(
        0.2651564830210424, 0.0,
        0.9642053928037905); // Fixed in file location vector attribute
    const double magnitude = 4.148493; // Fixed in file location value
    const double rotation = -15.37625; // Fixed in file orientation value
    Eigen::Vector3d rotationAxis(
        0, -1, 0); // Fixed in file orientation vector attribute
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    auto affine = Eigen::Transform<double, 3, Eigen::Affine>::Identity();
    // Rotation of bank
    affine = Eigen::Quaterniond(
                 Eigen::AngleAxisd(rotation * M_PI / 180, rotationAxis)) *
             affine;
    // Translation of bank
    affine = Eigen::Translation3d(magnitude * unitVectorTranslation) * affine;
    /*
      Affine should be for rotation around Y, and translated my U.M

      cos(theta)    0    -sin(theta)    U.M.x
      0             1    0               U.M.y
      sin(theta)    0    cos(theta)      U.M.z
      0             0    0               1
     */
    Eigen::Vector3d expectedPosition = affine * offset;
    TS_ASSERT(det0Postion.isApprox(expectedPosition, 1e-5));
  }

  void test_shape_cylinder_shape() {

    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    const auto &det1Shape = componentInfo->shape(1);
    const auto &det2Shape = componentInfo->shape(2);
    TSM_ASSERT_EQUALS("Pixel shapes should be the same within bank", &det1Shape,
                      &det2Shape);

    const auto *csgShape1 =
        dynamic_cast<const Mantid::Geometry::CSGObject *>(&det1Shape);
    TSM_ASSERT("Expected pixel shape as CSGObject", csgShape1 != nullptr);
    const auto *csgShape2 =
        dynamic_cast<const Mantid::Geometry::CSGObject *>(&det2Shape);
    TSM_ASSERT("Expected monitors shape as CSGObject", csgShape2 != nullptr);
    auto shapeBB = det1Shape.getBoundingBox();
    TS_ASSERT_DELTA(shapeBB.xMax() - shapeBB.xMin(), 0.03125 - (-0.03125),
                    1e-9); // Cylinder length fixed in file
    TS_ASSERT_DELTA(shapeBB.yMax() - shapeBB.yMin(), 2 * 0.00405,
                    1e-9); // Cylinder radius fixed in file
  }

  void test_mesh_shape() {

    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    const size_t monitorIndex = 0; // Fixed in file
    TS_ASSERT(detectorInfo->isMonitor(monitorIndex));
    TSM_ASSERT("Monitor shape", componentInfo->hasValidShape(monitorIndex));
    const auto &monitorShape = componentInfo->shape(monitorIndex);
    const auto *meshShape =
        dynamic_cast<const Mantid::Geometry::MeshObject *>(&monitorShape);
    TSM_ASSERT("Expected monitors shape as mesh", meshShape != nullptr);

    TS_ASSERT_EQUALS(meshShape->numberOfTriangles(),
                     6 * 2); // Each face of cube split into 2 triangles
    TS_ASSERT_EQUALS(meshShape->numberOfVertices(),
                     8); // 8 unique vertices in cube
    auto shapeBB = monitorShape.getBoundingBox();
    TS_ASSERT_DELTA(shapeBB.xMax() - shapeBB.xMin(), 2.0, 1e-9);
    TS_ASSERT_DELTA(shapeBB.yMax() - shapeBB.yMin(), 2.0, 1e-9);
    TS_ASSERT_DELTA(shapeBB.zMax() - shapeBB.zMin(), 2.0, 1e-9);
  }
};

class NexusGeometryParserTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometryParserTestPerformance *createSuite() {
    return new NexusGeometryParserTestPerformance();
  }

  NexusGeometryParserTestPerformance() {
    m_wishHDF5DefinitionPath = Kernel::ConfigService::Instance().getFullPath(
        "WISH_Definition_10Panels.hdf5", true, Poco::Glob::GLOB_DEFAULT);
    m_sans2dHDF5DefinitionPath = Kernel::ConfigService::Instance().getFullPath(
        "SANS2D_Definition_Tubes.hdf5", true, Poco::Glob::GLOB_DEFAULT);
    m_lokiHDF5DefinitionPath = Kernel::ConfigService::Instance().getFullPath(
        "LOKI_Definition.hdf5", true, Poco::Glob::GLOB_DEFAULT);
  }
  static void destroySuite(NexusGeometryParserTestPerformance *suite) {
    delete suite;
  }

  void test_load_wish() {
    auto start = std::chrono::high_resolution_clock::now();
    auto wishInstrument = NexusGeometryParser::createInstrument(
        m_wishHDF5DefinitionPath, std::make_unique<MockLogger>());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating WISH instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " ms" << std::endl;
    auto detInfo = extractDetectorInfo(*wishInstrument);
    TS_ASSERT_EQUALS(detInfo->size(), 778245); // Sanity check
  }

  void test_load_sans2d() {
    auto start = std::chrono::high_resolution_clock::now();
    auto sansInstrument = NexusGeometryParser::createInstrument(
        m_sans2dHDF5DefinitionPath, std::make_unique<MockLogger>());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating SANS2D instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " ms" << std::endl;
    auto detInfo = extractDetectorInfo(*sansInstrument);
    TS_ASSERT_EQUALS(detInfo->size(), 122888); // Sanity check
  }

  void test_load_loki() {
    auto start = std::chrono::high_resolution_clock::now();
    auto sansInstrument = NexusGeometryParser::createInstrument(
        m_lokiHDF5DefinitionPath, std::make_unique<MockLogger>());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating LOKI instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " ms" << std::endl;

    auto beamline = extractBeamline(*sansInstrument);
    auto componentInfo = std::move(std::get<0>(beamline));
    auto detectorInfo = std::move(std::get<1>(beamline));
    TS_ASSERT_EQUALS(detectorInfo->size(), 8000); // Sanity check

    // Add detectors are described by a meshobject 2d
    auto &shape = componentInfo->shape(0);
    auto *match = dynamic_cast<const Mantid::Geometry::MeshObject2D *>(&shape);
    TS_ASSERT(match);
  }

private:
  std::string m_wishHDF5DefinitionPath;
  std::string m_sans2dHDF5DefinitionPath;
  std::string m_lokiHDF5DefinitionPath;
};
#endif
