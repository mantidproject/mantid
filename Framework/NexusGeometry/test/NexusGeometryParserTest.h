// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/H5Util.h"
#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include "mockobjects.h"
#include <H5Cpp.h>
#include <Poco/Glob.h>
#include <chrono>
#include <gmock/gmock.h>
#include <string>

using namespace Mantid;
using namespace NexusGeometry;
using namespace DataHandling;

namespace {
std::unique_ptr<Geometry::DetectorInfo> extractDetectorInfo(const Mantid::Geometry::Instrument &instrument) {
  Geometry::ParameterMap pmap;
  return std::move(std::get<1>(instrument.makeBeamline(pmap)));
}

std::pair<std::unique_ptr<Geometry::ComponentInfo>, std::unique_ptr<Geometry::DetectorInfo>>
extractBeamline(const Mantid::Geometry::Instrument &instrument) {
  Geometry::ParameterMap pmap;
  auto beamline = instrument.makeBeamline(pmap);
  return {std::move(std::get<0>(beamline)), std::move(std::get<1>(beamline))};
}

std::string instrument_path(const std::string &local_name) {
  return Kernel::ConfigService::Instance().getFullPath(local_name, true, Poco::Glob::GLOB_DEFAULT);
}

} // namespace

class NexusGeometryParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometryParserTest *createSuite() { return new NexusGeometryParserTest(); }
  static void destroySuite(NexusGeometryParserTest *suite) { delete suite; }

  void test_parse_from_specific_entry() {
    // Test that the parser works correctly when there are multiple NXentry
    //   in the source file.

    FileResource multipleEntryInput("test_geometry_parser_with_multiple_entries.hdf5");
    {
      // Load the multiple NXentry test input.
      // (See notes about `NexusGeometrySave` and `NexusGeometryParser` at `_verify_basic_instrument` below.)
      H5::H5File input(instrument_path("unit_testing/SMALLFAKE_example_multiple_entries.hdf5"), H5F_ACC_RDONLY);

      // Copy all of the NXentry groups to a new file.
      H5::H5File testInput(multipleEntryInput.fullPath(), H5F_ACC_TRUNC);
      H5Util::copyGroup(testInput, "/mantid_workspace_1", input, "/mantid_workspace_1");
      H5Util::copyGroup(testInput, "/mantid_workspace_2", input, "/mantid_workspace_2");
      H5Util::copyGroup(testInput, "/mantid_workspace_3", input, "/mantid_workspace_3");

      // Remove the instrument from the first NXentry group.
      H5Util::deleteObjectLink(testInput, "/mantid_workspace_1/SmallFakeTubeInstrument");
    }

    // The default `createInstrument` signature should fail: it will try to load from the first NXentry,
    //   which no longer has an instrument.
    TS_ASSERT_THROWS(
        NexusGeometryParser::createInstrument(multipleEntryInput.fullPath(), std::make_unique<MockLogger>()),
        const H5::Exception &);

    // Loading explicitly from the first entry should also fail for the same reason.
    TS_ASSERT_THROWS(NexusGeometryParser::createInstrument(multipleEntryInput.fullPath(), "mantid_workspace_1",
                                                           std::make_unique<MockLogger>()),
                     const H5::Exception &);

    // Loading explicitly from the second entry should succeed.
    auto instrument = NexusGeometryParser::createInstrument(multipleEntryInput.fullPath(), "mantid_workspace_2",
                                                            std::make_unique<MockLogger>());

    // Verify that the instrument has been parsed correctly.
    _verify_basic_instrument(*instrument, true);
  }

  void test_basic_instrument_information() {
    auto instrument = _makeTestInstrument();
    _verify_basic_instrument(*instrument);
  }

  void test_source_is_where_expected() {
    auto instrument = _makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);

    auto sourcePosition = Kernel::toVector3d(componentInfo->position(componentInfo->source()));

    TS_ASSERT(sourcePosition.isApprox(Eigen::Vector3d(0, 0, -34.281))); // Check against fixed position in file
  }

  void test_simple_translation() {
    auto instrument = _makeTestInstrument();
    auto detectorInfo = extractDetectorInfo(*instrument);
    // First pixel in bank 2
    auto det0Position = Kernel::toVector3d(detectorInfo->position(detectorInfo->indexOf(1100000)));
    Eigen::Vector3d unitVector(0, 0,
                               1);                // Fixed in file location vector attribute
    const double magnitude = 4.0;                 // Fixed in file location value
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    Eigen::Vector3d expectedDet0Position = offset + (magnitude * unitVector);
    TS_ASSERT(det0Position.isApprox(expectedDet0Position));

    // Test monitor position
    Mantid::detid_t monitorDetId = 1;
    auto mon0Position = Kernel::toVector3d(detectorInfo->position(detectorInfo->indexOf(monitorDetId)));
    // Sanity check that this is a monitor
    TS_ASSERT(detectorInfo->isMonitor(detectorInfo->indexOf(monitorDetId)));
    // Check against location in file
    TS_ASSERT(mon0Position.isApprox(Eigen::Vector3d{0, 0, -12.064}));
  }

  void test_complex_translation() {
    auto instrument = _makeTestInstrument();
    auto detectorInfo = extractDetectorInfo(*instrument);
    // First pixel in bank 1

    auto det0Postion = Kernel::toVector3d(detectorInfo->position(detectorInfo->indexOf(2100000)));
    Eigen::Vector3d unitVectorTranslation(0.2651564830210424, 0.0,
                                          0.9642053928037905); // Fixed in file location vector attribute
    const double magnitude = 4.148493;                         // Fixed in file location value
    const double rotation = -15.37625;                         // Fixed in file orientation value
    Eigen::Vector3d rotationAxis(0, -1, 0);                    // Fixed in file orientation vector attribute
    Eigen::Vector3d offset(-0.498, -0.200, 0.00);              // All offsets for pixel x,
                                                               // and y specified separately,
                                                               // z defaults to 0
    auto affine = Eigen::Transform<double, 3, Eigen::Affine>::Identity();
    // Rotation of bank
    affine = Eigen::Quaterniond(Eigen::AngleAxisd(rotation * M_PI / 180, rotationAxis)) * affine;
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
    auto instrument = _makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    const auto &det1Shape = componentInfo->shape(1);
    const auto &det2Shape = componentInfo->shape(2);
    TSM_ASSERT_EQUALS("Pixel shapes should be the same within bank", &det1Shape, &det2Shape);

    const auto *csgShape1 = dynamic_cast<const Mantid::Geometry::CSGObject *>(&det1Shape);
    TSM_ASSERT("Expected pixel shape as CSGObject", csgShape1 != nullptr);
    const auto *csgShape2 = dynamic_cast<const Mantid::Geometry::CSGObject *>(&det2Shape);
    TSM_ASSERT("Expected monitors shape as CSGObject", csgShape2 != nullptr);
    auto shapeBB = det1Shape.getBoundingBox();
    TS_ASSERT_DELTA(shapeBB.xMax() - shapeBB.xMin(), 0.03125 - (-0.03125),
                    1e-9); // Cylinder length fixed in file
    TS_ASSERT_DELTA(shapeBB.yMax() - shapeBB.yMin(), 2 * 0.00405,
                    1e-9); // Cylinder radius fixed in file
  }

  void test_mesh_shape() {
    auto instrument = _makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    const size_t monitorIndex = 0; // Fixed in file
    TS_ASSERT(detectorInfo->isMonitor(monitorIndex));
    TSM_ASSERT("Monitor shape", componentInfo->hasValidShape(monitorIndex));
    const auto &monitorShape = componentInfo->shape(monitorIndex);
    const auto *meshShape = dynamic_cast<const Mantid::Geometry::MeshObject *>(&monitorShape);
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
  void test_pixel_shape_as_mesh() {
    auto instrument = NexusGeometryParser::createInstrument(instrument_path("unit_testing/DETGEOM_example_1.nxs"),
                                                            std::make_unique<testing::NiceMock<MockLogger>>());
    auto beamline = extractBeamline(*instrument);
    auto &compInfo = *beamline.first;
    auto &detInfo = *beamline.second;
    TS_ASSERT_EQUALS(detInfo.size(), 4);
    auto &shape1 = compInfo.shape(0);
    auto &shape2 = compInfo.shape(1);
    auto *shape1Mesh = dynamic_cast<const Geometry::MeshObject2D *>(&shape1); // Test detectors
    auto *shape2Mesh = dynamic_cast<const Geometry::MeshObject2D *>(&shape2);
    ETS_ASSERT(shape1Mesh);
    ETS_ASSERT(shape2Mesh);
    TS_ASSERT_EQUALS(shape1Mesh, shape2Mesh); // pixel shape - all identical.
    TSM_ASSERT_EQUALS("Same objects, same address", &shape1,
                      &shape2); // Shapes are shared when identical
    TS_ASSERT_EQUALS(shape1Mesh->numberOfTriangles(), 2);
    TS_ASSERT_EQUALS(shape1Mesh->numberOfVertices(), 4);
  }

  void test_pixel_shape_as_cylinders() {
    auto instrument = NexusGeometryParser::createInstrument(instrument_path("unit_testing/DETGEOM_example_2.nxs"),
                                                            std::make_unique<testing::NiceMock<MockLogger>>());
    auto beamline = extractBeamline(*instrument);
    auto &compInfo = *beamline.first;
    auto &detInfo = *beamline.second;
    TS_ASSERT_EQUALS(detInfo.size(), 4);

    auto &shape1 = compInfo.shape(0);
    auto &shape2 = compInfo.shape(1);

    TSM_ASSERT_EQUALS("Same objects, same address", &shape1,
                      &shape2);                                                // Shapes are shared when identical
    auto *shape1Cylinder = dynamic_cast<const Geometry::CSGObject *>(&shape1); // Test detectors
    auto *shape2Cylinder = dynamic_cast<const Geometry::CSGObject *>(&shape2);

    ETS_ASSERT(shape1Cylinder);
    ETS_ASSERT(shape2Cylinder);
    TS_ASSERT_EQUALS(shape1Cylinder->shapeInfo().radius(), 0.25);
    TS_ASSERT_EQUALS(shape1Cylinder->shapeInfo().height(), 0.5);
    TS_ASSERT_EQUALS(shape1Cylinder->shapeInfo().radius(), shape2Cylinder->shapeInfo().radius());
    TS_ASSERT_EQUALS(shape1Cylinder->shapeInfo().height(), shape2Cylinder->shapeInfo().height());
  }

  void test_detector_shape_as_mesh() {
    auto instrument = NexusGeometryParser::createInstrument(instrument_path("unit_testing/DETGEOM_example_3.nxs"),
                                                            std::make_unique<testing::NiceMock<MockLogger>>());
    auto beamline = extractBeamline(*instrument);
    auto &compInfo = *beamline.first;
    auto &detInfo = *beamline.second;
    TS_ASSERT_EQUALS(detInfo.size(), 4);
    auto &shape1 = compInfo.shape(0);
    auto &shape2 = compInfo.shape(1);
    TSM_ASSERT_DIFFERS("Different objects, different addresses", &shape1,
                       &shape2);                                              // Shapes are not shared
    auto *shape1Mesh = dynamic_cast<const Geometry::MeshObject2D *>(&shape1); // Test detectors
    auto *shape2Mesh = dynamic_cast<const Geometry::MeshObject2D *>(&shape2);
    TS_ASSERT(shape1Mesh);
    TS_ASSERT(shape2Mesh);
    TS_ASSERT_EQUALS(shape1Mesh->numberOfTriangles(), 1);
    TS_ASSERT_EQUALS(shape1Mesh->numberOfVertices(), 3);
    TS_ASSERT_EQUALS(shape2Mesh->numberOfTriangles(), 1);
    TS_ASSERT_EQUALS(shape2Mesh->numberOfVertices(), 3);
  }

  void test_detector_shape_as_cylinders() {
    auto instrument = NexusGeometryParser::createInstrument(instrument_path("unit_testing/DETGEOM_example_4.nxs"),
                                                            std::make_unique<testing::NiceMock<MockLogger>>());
    auto beamline = extractBeamline(*instrument);
    auto &compInfo = *beamline.first;
    auto &detInfo = *beamline.second;

    ETS_ASSERT_EQUALS(detInfo.size(), 3);

    TS_ASSERT(Kernel::toVector3d(compInfo.relativePosition(0)).isApprox(Eigen::Vector3d(0.0, -0.4 / 2, 0.0)));

    auto &shape1 = compInfo.shape(0);
    auto &shape2 = compInfo.shape(1);
    auto &shape3 = compInfo.shape(2);

    TSM_ASSERT_DIFFERS("Different objects, different addresses", &shape1,
                       &shape2); // Shapes are not shared
    TSM_ASSERT_DIFFERS("Different objects, different addresses", &shape1,
                       &shape3); // Shapes are not shared
    const auto *shape1Cylinder = dynamic_cast<const Geometry::CSGObject *>(&shape1);
    const auto *shape2Cylinder = dynamic_cast<const Geometry::CSGObject *>(&shape2);
    const auto *shape3Cylinder = dynamic_cast<const Geometry::CSGObject *>(&shape3);

    ETS_ASSERT(shape1Cylinder);
    ETS_ASSERT(shape2Cylinder);
    ETS_ASSERT(shape3Cylinder);
    TS_ASSERT_EQUALS(shape1Cylinder->shapeInfo().radius(), 0.5);
    TS_ASSERT_EQUALS(shape2Cylinder->shapeInfo().radius(), 0.5);
    TS_ASSERT_EQUALS(shape3Cylinder->shapeInfo().radius(), 0.5);
    TS_ASSERT_EQUALS(shape1Cylinder->shapeInfo().height(), 0.4);
    TS_ASSERT_EQUALS(shape2Cylinder->shapeInfo().height(), 0.3); // 0.3
    TS_ASSERT_EQUALS(shape3Cylinder->shapeInfo().height(), 0.2); // 0.5- 0.3
  }

  void test_parse_detector_shape_with_3d_pixels() {
    // GIVEN a NeXus file describing a detector with two octahedral voxels
    // with:
    //   - detector numbers of 0 and 1
    //   - pixel location defined in x_pixel_offset, y_pixel_offset,
    //     z_pixel_offset datasets as [1.1, 2.2, -2.0] and [1.1, 2.2, 0.0]
    //     w.r.t. detector origin
    //   - detector position defined as [2, 0, 2] w.r.t. coord system origin
    //
    // Multiple faces in the mesh are mapped to the same detector number,
    // thus defining a 3D pixel
    // Unlike 2D pixel case, pixel offset datasets must be present in the file.
    // The parser will not try to calculate the centre of mass of the polyhedron
    // to use as the pixel position as this is computationally expensive and
    // possibly not even the "correct" pixel position for some detector types
    const std::string filename = "unit_testing/VOXEL_example.nxs";
    const int expectedDetectorNumber1 = 0;
    const int expectedDetectorNumber2 = 1;
    const auto expectedPosition1 = Eigen::Vector3d{3.1, 2.2, 0.0};
    const auto expectedPosition2 = Eigen::Vector3d{3.1, 2.2, 2.0};

    // WHEN the NeXus geometry is parsed
    const auto instrument = NexusGeometryParser::createInstrument(instrument_path(filename),
                                                                  std::make_unique<testing::NiceMock<MockLogger>>());

    // THEN the voxels are successfully parsed, locations match
    // offsets datasets from file, and shape has expected characteristics
    const auto parsedBeamline = extractBeamline(*instrument);
    const auto &parsedDetInfo = *parsedBeamline.second;
    TS_ASSERT_EQUALS(parsedDetInfo.size(), 2);

    const auto detectorInfo = extractDetectorInfo(*instrument);
    const auto voxelPosition1 =
        Kernel::toVector3d(detectorInfo->position(detectorInfo->indexOf(expectedDetectorNumber1)));
    TS_ASSERT(voxelPosition1.isApprox(expectedPosition1));
    const auto voxelPosition2 =
        Kernel::toVector3d(detectorInfo->position(detectorInfo->indexOf(expectedDetectorNumber2)));
    TS_ASSERT(voxelPosition2.isApprox(expectedPosition2));

    // Check shape of each of the two voxels
    const auto &parsedCompInfo = *parsedBeamline.first;
    const std::array<size_t, 2> pixelIndices{0, 1};
    for (const auto pixelIndex : pixelIndices) {
      const auto &parsedShape = parsedCompInfo.shape(pixelIndex);
      const auto *parsedShapeMesh = dynamic_cast<const Geometry::MeshObject *>(&parsedShape);
      // Check it looks like it might define an enclosed volume:
      TS_ASSERT(parsedShapeMesh->hasValidShape());
      // The voxel is a regular octahedron, which can be treated as 2
      // square-based pyramids connected at their bases
      // Volume is therefore 2 * a^2 * h/3
      // where a is base edge and h is pyramid height
      // Corners of the octahedron are at unit cartesian positions:
      // [1.0, 0.0, 0.0], [0.0, 1.0, 0.0] and so on, therefore
      // a = sqrt(1^2 + 1^2) and h = 1
      // 2 * sqrt(1^2 + 1^2)^2 * 1/3 = 4/3
      const double expectedVolume = 1.33;
      TS_ASSERT_DELTA(parsedShapeMesh->volume(), expectedVolume, 0.01);
      // Each face of the octahedron is a triangle,
      // therefore expect mesh to be composed of 8 triangles
      TS_ASSERT_EQUALS(parsedShapeMesh->numberOfTriangles(), 8);
    }
  }

private:
  // Parse a basic instrument from "unit_testing/SMALLFAKE_example_geometry.hdf5".
  static std::unique_ptr<const Mantid::Geometry::Instrument> _makeTestInstrument() {
    const auto fullpath = instrument_path("unit_testing/SMALLFAKE_example_geometry.hdf5");
    return NexusGeometryParser::createInstrument(fullpath, std::make_unique<MockLogger>());
  }

  // Verify that the instrument from "unit_testing/SMALLFAKE_example_geometry.hdf5" has been parsed correctly.
  // Notes:
  //   * The original HDF5 test-input file contains detector tubes.  These will be parsed correctly by
  //   `NexusGeometryParser`,
  //     but unfortunately at present, these tubes are ignored by `NexusGeometrySave`;
  //   * The `saveAndReparse` argument flag, and the `detectorInfoSize`, and `componentInfoSize`
  //     constants are provided to allow the required adjustments, when reparsing an instrument saved by
  //     `NexusGeometrySave`.
  //   * For the original instrument:
  //       -- `componentInfo->size() == 128 * 2 + 1 + 2 + 16 + 1 + 1 + 1`;
  //       -- `detectorInfo->size() == 128 * 2 + 1;
  //   * For the saved and reparsed instrument, excluding the tubes, these become:
  //       -- `componentInfo->size() == 128 * 2 + 1 + 2 + 1 + 1 + 1`;
  //       -- `detectorInfo->size() == 128 * 2 + 1`;
  //
  static void _verify_basic_instrument(const Mantid::Geometry::Instrument &instrument, bool saveAndReparse = false) {

    const size_t expectedDetectorBankSize = 128;
    const size_t numberOfDetectorBanks = 2;
    const size_t numberOfMonitors = 1;
    const size_t expectedDetectorInfoSize = numberOfDetectorBanks * expectedDetectorBankSize + numberOfMonitors;
    const size_t numberOfTubes = 16;

    const size_t expectedComponentInfoSize =
        saveAndReparse
            ? numberOfDetectorBanks * expectedDetectorBankSize + numberOfMonitors + numberOfDetectorBanks + 1 + 1 + 1
            : numberOfDetectorBanks * expectedDetectorBankSize + numberOfMonitors + numberOfDetectorBanks +
                  numberOfTubes + 1 + 1 + 1;
    const std::string componentInfoDescription = saveAndReparse
                                                     ? "Detectors + 2 banks + root + source + sample"
                                                     : "Detectors + 2 banks + 16 tubes + root + source + sample";

    auto [componentInfo, detectorInfo] = extractBeamline(instrument);

    TSM_ASSERT_EQUALS("Detectors + 1 monitor", detectorInfo->size(), expectedDetectorInfoSize);
    TSM_ASSERT_EQUALS(componentInfoDescription.c_str(), componentInfo->size(), expectedComponentInfoSize);

    // Check 128 detectors in first bank
    size_t rearBankIndex(-1);
    TS_ASSERT_THROWS_NOTHING(rearBankIndex = componentInfo->indexOfAny("rear-detector"));
    TS_ASSERT_EQUALS(128, componentInfo->detectorsInSubtree(rearBankIndex).size());

    TS_ASSERT(Mantid::Kernel::toVector3d(componentInfo->position(rearBankIndex)).isApprox(Eigen::Vector3d{0, 0, 4}));

    // Check 128 detectors in second bank
    size_t frontBankIndex(-1);
    TS_ASSERT_THROWS_NOTHING(frontBankIndex = componentInfo->indexOfAny("front-detector"));
    TS_ASSERT_EQUALS(128, componentInfo->detectorsInSubtree(frontBankIndex).size());
  }
};

class NexusGeometryParserTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometryParserTestPerformance *createSuite() { return new NexusGeometryParserTestPerformance(); }

  NexusGeometryParserTestPerformance() {
    m_wishHDF5DefinitionPath = instrument_path("WISH_Definition_10Panels.hdf5");
    m_sans2dHDF5DefinitionPath = instrument_path("SANS2D_Definition_Tubes.hdf5");
    m_lokiHDF5DefinitionPath = instrument_path("LOKI_Definition.hdf5");
  }
  static void destroySuite(NexusGeometryParserTestPerformance *suite) { delete suite; }

  void test_load_wish() {
    auto start = std::chrono::high_resolution_clock::now();
    auto wishInstrument =
        NexusGeometryParser::createInstrument(m_wishHDF5DefinitionPath, std::make_unique<MockLogger>());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating WISH instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms" << std::endl;
    auto detInfo = extractDetectorInfo(*wishInstrument);
    TS_ASSERT_EQUALS(detInfo->size(), 778245); // Sanity check
  }

  void test_load_sans2d() {
    auto start = std::chrono::high_resolution_clock::now();
    auto sansInstrument =
        NexusGeometryParser::createInstrument(m_sans2dHDF5DefinitionPath, std::make_unique<MockLogger>());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating SANS2D instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms" << std::endl;
    auto detInfo = extractDetectorInfo(*sansInstrument);
    TS_ASSERT_EQUALS(detInfo->size(), 122888); // Sanity check
  }

  void test_load_loki() {
    auto start = std::chrono::high_resolution_clock::now();
    auto sansInstrument =
        NexusGeometryParser::createInstrument(m_lokiHDF5DefinitionPath, std::make_unique<MockLogger>());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating LOKI instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms" << std::endl;

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
