// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include <Eigen/Geometry>
#include <memory>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

/*
 Helper function to create an ID -> index map from an ordered collection of IDs.
 First ID gets index of 0, subsequent ID entries increment index by 1.
*/
std::shared_ptr<const std::unordered_map<Mantid::Geometry::IComponent const *, size_t>>
makeComponentIDMap(const std::shared_ptr<const std::vector<Mantid::Geometry::ComponentID>> &componentIds) {
  auto idMap = std::make_shared<std::unordered_map<Mantid::Geometry::IComponent const *, size_t>>();

  for (size_t i = 0; i < componentIds->size(); ++i) {
    (*idMap.get())[(*componentIds)[i]] = i;
  }
  return idMap;
}

std::shared_ptr<CSGObject> createCappedCylinder() {
  std::string C31 = "cx 0.5"; // cylinder x-axis radius 0.5
  std::string C32 = "px 1.2";
  std::string C33 = "px -3.2";

  // First create some surfaces
  std::map<int, std::shared_ptr<Surface>> CylSurMap;
  CylSurMap[31] = std::make_shared<Cylinder>();
  CylSurMap[32] = std::make_shared<Plane>();
  CylSurMap[33] = std::make_shared<Plane>();

  CylSurMap[31]->setSurface(C31);
  CylSurMap[32]->setSurface(C32);
  CylSurMap[33]->setSurface(C33);
  CylSurMap[31]->setName(31);
  CylSurMap[32]->setName(32);
  CylSurMap[33]->setName(33);

  // Capped cylinder (id 21)
  // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
  std::string ObjCapCylinder = "-31 -32 33";

  auto retVal = std::make_shared<CSGObject>();
  retVal->setObject(21, ObjCapCylinder);
  retVal->populate(CylSurMap);

  return retVal;
}

// Make a Beamline ComponentInfo for a single component
std::unique_ptr<Beamline::ComponentInfo> makeSingleBeamlineComponentInfo(
    Eigen::Vector3d position = Eigen::Vector3d{1, 1, 1},
    Eigen::Quaterniond rotation = Eigen::Quaterniond(Eigen::Affine3d::Identity().rotation()),
    Eigen::Vector3d scaleFactor = Eigen::Vector3d{1, 1, 1}) {

  auto detectorIndices = std::make_shared<std::vector<size_t>>(); // No detectors in this example
  auto detectorRanges = std::make_shared<std::vector<std::pair<size_t, size_t>>>();
  detectorRanges->emplace_back(std::make_pair(0, 0)); // One component with no detectors

  auto componentIndices = std::make_shared<std::vector<size_t>>(std::vector<size_t>{0}); // No detectors in this example
  auto componentRanges = std::make_shared<std::vector<std::pair<size_t, size_t>>>();
  componentRanges->emplace_back(std::make_pair(0, 1)); // One component with no sub-components

  auto parentIndices =
      std::make_shared<const std::vector<size_t>>(std::vector<size_t>()); // These indices are invalid, but that's
                                                                          // ok as not being tested here

  auto positions = std::make_shared<std::vector<Eigen::Vector3d>>(1, position);
  auto rotations =
      std::make_shared<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>(1, rotation);
  auto scaleFactors = std::make_shared<std::vector<Eigen::Vector3d>>(1, scaleFactor);
  auto names = std::make_shared<std::vector<std::string>>(1);
  using Mantid::Beamline::ComponentType;
  auto componentType = std::make_shared<std::vector<ComponentType>>(1, ComponentType::Generic);
  auto children = std::make_shared<std::vector<std::vector<size_t>>>(1);
  return std::make_unique<Beamline::ComponentInfo>(detectorIndices, detectorRanges, componentIndices, componentRanges,
                                                   parentIndices, children, positions, rotations, scaleFactors,
                                                   componentType, names, -1, -1);
}
} // namespace

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_indexOf() {
    auto detectorIndices = std::make_shared<std::vector<size_t>>(); // No detectors in this
                                                                    // example
    auto detectorRanges = std::make_shared<std::vector<std::pair<size_t, size_t>>>();
    detectorRanges->emplace_back(std::make_pair(0, 0)); // One component with no detectors
    detectorRanges->emplace_back(std::make_pair(0, 0)); // Another component with no detectors

    auto componentIndices =
        std::make_shared<std::vector<size_t>>(std::vector<size_t>{0, 1}); // No detectors in this example
    auto componentRanges = std::make_shared<std::vector<std::pair<size_t, size_t>>>();
    componentRanges->emplace_back(std::make_pair(0, 0)); // One component with no sub-components
    componentRanges->emplace_back(std::make_pair(0, 0)); // Another component with no subcomponents

    auto parentIndices =
        std::make_shared<const std::vector<size_t>>(std::vector<size_t>{9, 9, 9}); // These indices are invalid, but
                                                                                   // that's ok as not being tested here

    auto positions = std::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = std::make_shared<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>(2);
    auto scaleFactors = std::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto names = std::make_shared<std::vector<std::string>>(2);
    using Mantid::Beamline::ComponentType;
    auto isRectBank = std::make_shared<std::vector<ComponentType>>(2, ComponentType::Generic);
    auto children = std::make_shared<std::vector<std::vector<size_t>>>(1, std::vector<size_t>(1));
    auto internalInfo = std::make_unique<Beamline::ComponentInfo>(detectorIndices, detectorRanges, componentIndices,
                                                                  componentRanges, parentIndices, children, positions,
                                                                  rotations, scaleFactors, isRectBank, names, -1, -1);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1, &comp2});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->emplace_back(std::make_shared<const Geometry::CSGObject>());
    shapes->emplace_back(std::make_shared<const Geometry::CSGObject>());

    ComponentInfo info(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);
    TS_ASSERT_EQUALS(info.indexOf(comp1.getComponentID()), 0);
    TS_ASSERT_EQUALS(info.indexOf(comp2.getComponentID()), 1);
  }

  void test_partial_copy() {
    auto internalInfo = makeSingleBeamlineComponentInfo();
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->emplace_back(createCappedCylinder());

    ComponentInfo a(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);

    // Make the copy
    auto b = a.cloneWithoutDetectorInfo();

    // Compare sizes
    TS_ASSERT_EQUALS(b->size(), a.size());
    // Shapes are the same
    TS_ASSERT_EQUALS(&b->shape(0), &a.shape(0));
    // IDs are the same
    TS_ASSERT_EQUALS(b->indexOf(&comp1), a.indexOf(&comp1));
    TS_ASSERT(!b->hasDetectorInfo());
  }

  void test_hasValidShape() {
    auto internalInfo = makeSingleBeamlineComponentInfo();
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->emplace_back(createCappedCylinder());

    ComponentInfo compInfo(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);

    TS_ASSERT(compInfo.hasValidShape(0));
    // Nullify the shape of the component
    shapes->at(0) = std::shared_ptr<const Geometry::IObject>(nullptr);
    TS_ASSERT(!compInfo.hasValidShape(0));
    TS_ASSERT_THROWS(compInfo.solidAngle(0, V3D{1, 1, 1}), Mantid::Kernel::Exception::NullPointerException &);
  }

  void test_simple_solidAngle() {
    auto position = Eigen::Vector3d{0, 0, 0};
    // No rotation
    const double radius = 1.0;
    auto rotation = Eigen::Quaterniond(Eigen::Affine3d::Identity().rotation());

    auto internalInfo = makeSingleBeamlineComponentInfo(position, rotation);
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->emplace_back(ComponentCreationHelper::createSphere(radius));

    ComponentInfo info(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);

    double satol = 1e-9; // tolerance for solid angle

    // Put observer on surface of sphere and solid angle is 2PI
    V3D observer{radius, 0, 0};
    TS_ASSERT_DELTA(info.solidAngle(0, observer), 2 * M_PI, satol);
    // Put observer at center of sphere and solid angle is full 4PI square
    // radians
    observer = V3D{0, 0, 0};
    TS_ASSERT_DELTA(info.solidAngle(0, observer), 4 * M_PI, satol);
    // Nullify  the shape and retest solid angle
    shapes->at(0) = std::shared_ptr<const Geometry::IObject>(nullptr);
    TS_ASSERT_THROWS(info.solidAngle(0, observer), Mantid::Kernel::Exception::NullPointerException &);
  }

  // Test adapted from ObjComponentTest
  void test_solidAngle() {

    auto position = Eigen::Vector3d{10, 0, 0};
    auto rotation = Eigen::Quaterniond(Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
    auto internalInfo = makeSingleBeamlineComponentInfo(position, rotation);
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->emplace_back(createCappedCylinder());

    ComponentInfo info(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);

    double satol = 2e-2; // tolerance for solid angle
    TS_ASSERT_DELTA(info.solidAngle(0, V3D(10, 1.7, 0)), 1.840302, satol);
  }

  void test_boundingBox_single_component() {

    const double radius = 2;
    Eigen::Vector3d position{1, 1, 1};
    auto internalInfo = makeSingleBeamlineComponentInfo(position);
    Mantid::Geometry::ObjComponent comp1("component1", ComponentCreationHelper::createSphere(radius));

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->emplace_back(ComponentCreationHelper::createSphere(radius));

    ComponentInfo componentInfo(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);

    BoundingBox boundingBox = componentInfo.boundingBox(0 /*componentIndex*/);

    TS_ASSERT((boundingBox.minPoint() - (Kernel::V3D{position[0] - radius, position[1] - radius, position[2] - radius}))
                  .norm() < 1e-9);
    TS_ASSERT((boundingBox.maxPoint() - (Kernel::V3D{position[0] + radius, position[1] + radius, position[2] + radius}))
                  .norm() < 1e-9);
    // Nullify shape and retest BoundingBox
    shapes->at(0) = std::shared_ptr<const Geometry::IObject>(nullptr);
    boundingBox = componentInfo.boundingBox(0);
    TS_ASSERT((boundingBox.minPoint() - Kernel::V3D{1., 1., 1.}).norm() < 1e-9);
    TS_ASSERT((boundingBox.maxPoint() - Kernel::V3D{1., 1., 1.}).norm() < 1e-9);
  }

  // Test calculation of the bounding box for a milimiter-sized
  // capped-cylinder detector pixel
  void test_boundingBox_single_component_capped_cylinder() {

    const double radius = 0.00275;
    const double height = 0.0042;
    const Mantid::Kernel::V3D baseCentre(0., 0., 0.);
    const Mantid::Kernel::V3D axis(0, 1, 0);
    const std::string id("cy-1");

    Eigen::Vector3d position{1., 1., 1.};
    auto internalInfo = makeSingleBeamlineComponentInfo(position);
    Mantid::Geometry::ObjComponent comp1(
        "component1", ComponentCreationHelper::createCappedCylinder(radius, height, baseCentre, axis, id));

    auto componentIds = std::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
        std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->push_back(ComponentCreationHelper::createCappedCylinder(radius, height, baseCentre, axis, id));

    ComponentInfo componentInfo(std::move(internalInfo), componentIds, makeComponentIDMap(componentIds), shapes);

    BoundingBox boundingBox = componentInfo.boundingBox(0 /*componentIndex*/);

    TS_ASSERT((boundingBox.width() - (Kernel::V3D{2 * radius, height, 2 * radius})).norm() < 1e-9);
    TS_ASSERT((boundingBox.minPoint() - (Kernel::V3D{position[0] - radius, position[1], position[2] - radius})).norm() <
              1e-9);
    TS_ASSERT((boundingBox.maxPoint() - (Kernel::V3D{position[0] + radius, position[1] + height, position[2] + radius}))
                  .norm() < 1e-9);
    // Nullify shape and retest BoundingBox
    shapes->at(0) = std::shared_ptr<const Geometry::IObject>(nullptr);
    boundingBox = componentInfo.boundingBox(0);
    TS_ASSERT((boundingBox.minPoint() - Kernel::V3D{1., 1., 1.}).norm() < 1e-9);
    TS_ASSERT((boundingBox.maxPoint() - Kernel::V3D{1., 1., 1.}).norm() < 1e-9);
  }

  void test_boundingBox_complex() {
    const V3D sourcePos(-1, 0, 0);
    const V3D samplePos(0, 0, 0);
    const V3D detectorPos(11, 0, 0);
    const double radius = 0.01; // See helper creation method for definition
    // Create a very basic real instrument to visit
    auto instrument = ComponentCreationHelper::createMinimalInstrument(sourcePos, samplePos, detectorPos);

    // CompAssembly (and hence Instrument 1.0) has getter and setter for
    // position!
    instrument->setPos(samplePos);

    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);
    // Check bounding box of detector
    auto boundingBox = componentInfo->boundingBox(0 /*detector index*/);
    TS_ASSERT((boundingBox.minPoint() -
               (Kernel::V3D{detectorPos[0] - radius, detectorPos[1] - radius, detectorPos[2] - radius}))
                  .norm() < 1e-9);
    TS_ASSERT((boundingBox.maxPoint() -
               (Kernel::V3D{detectorPos[0] + radius, detectorPos[1] + radius, detectorPos[2] + radius}))
                  .norm() < 1e-9);

    // Check bounding box of root (instrument)
    boundingBox = componentInfo->boundingBox(componentInfo->root() /*Root*/);

    // min in the sample (source is ignored by design in instrument 1.0 and
    // instrument 2.0).
    TS_ASSERT((boundingBox.minPoint() - (Kernel::V3D{samplePos[0], detectorPos[1] - radius, detectorPos[2] - radius}))
                  .norm() < 1e-9);
    // max is the detector
    TS_ASSERT((boundingBox.maxPoint() -
               (Kernel::V3D{detectorPos[0] + radius, detectorPos[1] + radius, detectorPos[2] + radius}))
                  .norm() < 1e-9);
  }

  void test_boundingBox_around_rectangular_bank() {

    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(1 /*1 bank*/, 10 /*10 by 10*/);

    // CompAssembly (and hence Instrument 1.0) has getter and setter for
    // position!
    instrument->setPos(instrument->getSample()->getPos());

    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);
    // Check bounding box of root (instrument)
    auto boundingBoxRoot = componentInfo->boundingBox(componentInfo->root() /*Root*/
    );
    // min Z in the sample
    auto boundingBoxSample = componentInfo->boundingBox(componentInfo->sample());
    TS_ASSERT((boundingBoxRoot.minPoint().Z() - boundingBoxSample.minPoint().Z()) < 1e-9);

    // max is the Rectangular bank
    auto bankIndex = componentInfo->root() - 3;
    using Mantid::Beamline::ComponentType;
    TS_ASSERT_EQUALS(componentInfo->componentType(bankIndex), ComponentType::Rectangular);
    auto boundingBoxBank = componentInfo->boundingBox(bankIndex);
    TS_ASSERT((boundingBoxRoot.maxPoint() - boundingBoxBank.maxPoint()).norm() < 1e-9);
  }

  void test_boundingBox_complex_rectangular_bank_setup() {

    /* y
     * |
     * |---z                        bank1
     *            source    sample              det
     *                                   bank2
     */

    Mantid::Geometry::Instrument instrument;

    int pixels = 10; // 10*10 total
    double pixelSpacing = 0;
    Mantid::Kernel::Quat bankRot{}; // No rotation

    // Add a rectangular bank
    int idStart = 0;
    std::string bankName = "bank1";
    Mantid::Kernel::V3D bankPos{0, 1, 1};
    ComponentCreationHelper::addRectangularBank(instrument, idStart, pixels, pixelSpacing, bankName, bankPos, bankRot);

    // A source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D{0, 0, -10});
    instrument.add(source);
    instrument.markAsSource(source);

    // A sample
    Component *sample = new Component("some-surface-holder");
    sample->setPos(V3D{0, 0, 0});
    instrument.add(sample);
    instrument.markAsSamplePos(sample);

    // A detector
    Detector *det = new Detector("point-detector", (2 * pixels * pixels) + 1 /*detector id*/, nullptr);
    det->setPos(V3D{0, 0, 3});
    det->setShape(ComponentCreationHelper::createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
    instrument.add(det);
    instrument.markAsDetector(det);

    // Add another rectangular bank
    idStart = pixels * pixels;
    bankName = "bank2";
    bankPos = Mantid::Kernel::V3D{0, -1, 2};
    ComponentCreationHelper::addRectangularBank(instrument, idStart, pixels, pixelSpacing, bankName, bankPos, bankRot);

    auto wrappers = InstrumentVisitor::makeWrappers(instrument);
    const auto &componentInfo = std::get<0>(wrappers);
    // Check bounding box of root (instrument)
    auto boundingBoxRoot = componentInfo->boundingBox(componentInfo->root() /*Root*/);

    // Check free detector not ignored because it's sandwidged between banks.
    // Should not be skipped over.
    const double detRadius = 0.01; // See test helper for value
    TS_ASSERT_DELTA(boundingBoxRoot.maxPoint().Z(), det->getPos().Z() + detRadius, 1e-9);

    // Check bank1 represents max point in y
    const size_t bank1Index = componentInfo->root() - 4 - 10;
    auto boundingBoxBank1 = componentInfo->boundingBox(bank1Index);
    using Mantid::Beamline::ComponentType;
    TS_ASSERT_EQUALS(componentInfo->componentType(bank1Index), ComponentType::Rectangular);
    TS_ASSERT_DELTA(boundingBoxRoot.maxPoint().Y(), boundingBoxBank1.maxPoint().Y(), 1e-9);

    // Check bank2 represents min point in y
    const size_t bank2Index = componentInfo->root() - 1;
    auto boundingBoxBank2 = componentInfo->boundingBox(bank2Index);
    TS_ASSERT_EQUALS(componentInfo->componentType(bank2Index), ComponentType::Rectangular);
    TS_ASSERT_DELTA(boundingBoxRoot.minPoint().Y(), boundingBoxBank2.minPoint().Y(), 1e-9);
  }

  void test_boundingBox_with_regular_bank_of_tubes() {

    size_t nTubes = 4;
    size_t nDetectorsPerTube = 10;
    std::vector<double> offsets(4, 0); // No offsets in tubes
    double width = 10;
    double height = 12;
    auto pixelHeight = height / static_cast<double>(nDetectorsPerTube);
    auto pixelWidth = width / static_cast<double>(nTubes);
    double minYCenter = -height / 2;
    double maxYCenter = minYCenter + (static_cast<double>(nDetectorsPerTube)) * pixelHeight;
    double minXCenter = -width / 2;
    double maxXCenter = minXCenter + (static_cast<double>(nTubes) - 1) * pixelWidth;

    auto instrument = ComponentCreationHelper::createCylInstrumentWithVerticalOffsetsSpecified(
        nTubes, offsets, nDetectorsPerTube, -width / 2, width / 2, -height / 2, height / 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);

    size_t bankOfTubesIndex = componentInfo->root() - 3;
    TS_ASSERT_EQUALS(componentInfo->componentType(bankOfTubesIndex),
                     Beamline::ComponentType::Unstructured); // Sanity check

    auto boundingBox = componentInfo->boundingBox(bankOfTubesIndex);
    TS_ASSERT_DELTA(boundingBox.minPoint().Y(), minYCenter, 1e-5);
    TS_ASSERT_DELTA(boundingBox.maxPoint().Y(), maxYCenter, 1e-5);
    TS_ASSERT_DELTA(boundingBox.minPoint().Z(), componentInfo->position(0).Z() - pixelWidth / 2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.maxPoint().Z(), componentInfo->position(0).Z() + pixelWidth / 2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.minPoint().X(), minXCenter - pixelWidth / 2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.maxPoint().X(), maxXCenter + pixelWidth / 2, 1e-5);
  }

  void test_boundingBox_with_irregular_bank_of_tubes() {

    size_t nTubes = 4;
    size_t nDetectorsPerTube = 10;
    std::vector<double> offsets{0, 0, 1.2, -0.3}; // one tube offset by +1.2, another by -0.3
    double width = 10;
    double height = 12;
    auto pixelHeight = height / static_cast<double>(nDetectorsPerTube);
    auto pixelWidth = width / static_cast<double>(nTubes);
    double minYCenter = -height / 2;
    double maxYCenter = minYCenter + static_cast<double>(nDetectorsPerTube) * pixelHeight;
    double minXCenter = -width / 2;
    double maxXCenter = minXCenter + (static_cast<double>(nTubes) - 1) * pixelWidth;

    auto instrument = ComponentCreationHelper::createCylInstrumentWithVerticalOffsetsSpecified(
        nTubes, offsets, nDetectorsPerTube, -width / 2, width / 2, -height / 2, height / 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);

    size_t bankOfTubesIndex = componentInfo->root() - 3;
    TS_ASSERT_EQUALS(componentInfo->componentType(bankOfTubesIndex),
                     Beamline::ComponentType::Unstructured); // Sanity check

    auto boundingBox = componentInfo->boundingBox(bankOfTubesIndex);
    TS_ASSERT_DELTA(boundingBox.minPoint().Y(), minYCenter + offsets[3],
                    1e-5); // Offset controls max Y
    TS_ASSERT_DELTA(boundingBox.maxPoint().Y(), maxYCenter + offsets[2],
                    1e-5); // Offset controls min Y
    TS_ASSERT_DELTA(boundingBox.minPoint().Z(), componentInfo->position(0).Z() - pixelWidth / 2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.maxPoint().Z(), componentInfo->position(0).Z() + pixelWidth / 2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.minPoint().X(), minXCenter - pixelWidth / 2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.maxPoint().X(), maxXCenter + pixelWidth / 2, 1e-5);
  }

  void test_tube_bounding_box() {
    size_t nTubes = 2;
    size_t nDetectorsPerTube = 5;
    std::vector<double> offsets{0.1, -0.2}; // Just two tubes one offset up the other one down
    double width = 2;
    double height = 10;
    auto pixelHeight = height / static_cast<double>(nDetectorsPerTube);
    double minYCenter = -height / 2;
    double maxYCenter = minYCenter + static_cast<double>(nDetectorsPerTube) * pixelHeight;

    auto instrument = ComponentCreationHelper::createCylInstrumentWithVerticalOffsetsSpecified(
        nTubes, offsets, nDetectorsPerTube, -width / 2, width / 2, -height / 2, height / 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);

    size_t tube1Index = componentInfo->root() - 5;
    TS_ASSERT_EQUALS(componentInfo->componentType(tube1Index),
                     Beamline::ComponentType::OutlineComposite); // Sanity check
    size_t tube2Index = componentInfo->root() - 4;
    TS_ASSERT_EQUALS(componentInfo->componentType(tube2Index),
                     Beamline::ComponentType::OutlineComposite); // Sanity check

    auto boundingBox = componentInfo->boundingBox(tube1Index);
    TS_ASSERT_DELTA(boundingBox.minPoint().Y(), minYCenter + offsets[0],
                    1e-6); // Offset controls max Y
    TS_ASSERT_DELTA(boundingBox.maxPoint().Y(), maxYCenter + offsets[0],
                    1e-6); // Offset controls min Y

    boundingBox = componentInfo->boundingBox(tube2Index);
    TS_ASSERT_DELTA(boundingBox.minPoint().Y(), minYCenter + offsets[1],
                    1e-6); // Offset controls max Y
    TS_ASSERT_DELTA(boundingBox.maxPoint().Y(), maxYCenter + offsets[1],
                    1e-6); // Offset controls min Y
  }

  void test_scanning_non_bank_throws() {
    const V3D sourcePos(-1, 0, 0);
    const V3D samplePos(0, 0, 0);
    const V3D detectorPos(11, 0, 0);
    auto instrument = ComponentCreationHelper::createMinimalInstrument(sourcePos, samplePos, detectorPos);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);

    const auto &componentInfo = std::get<0>(wrappers);
    componentInfo->setScanInterval({0, 1});
    std::pair<size_t, size_t> temporalSourceIndex(componentInfo->source(), 0); // Source index with time component
    TSM_ASSERT_THROWS("Source is NOT allowed to time-scan",
                      componentInfo->setPosition(temporalSourceIndex, V3D(0, 0, 0)), std::runtime_error &);
    TSM_ASSERT_THROWS_NOTHING("Source position set as time-constant allowed",
                              componentInfo->setPosition(componentInfo->source(), V3D(0, 0, 0)));
    TSM_ASSERT_THROWS("Source is NOT allowed to time-scan", componentInfo->setRotation(temporalSourceIndex, Quat()),
                      std::runtime_error &);
    TSM_ASSERT_THROWS_NOTHING("Source rotation set as time-constant allowed",
                              componentInfo->setRotation(componentInfo->source(), Quat()));
  }

  void test_merge_sync_scan() {

    const V3D sourcePos(-1, 0, 0);
    const V3D samplePos(0, 0, 0);
    const V3D detectorPos(11, 0, 0);
    auto instrument = ComponentCreationHelper::createMinimalInstrument(sourcePos, samplePos, detectorPos);

    auto wrappers1 = InstrumentVisitor::makeWrappers(*instrument);
    const auto &infoScan1 = std::get<0>(wrappers1);

    auto wrappers2 = InstrumentVisitor::makeWrappers(*instrument);
    const auto &infoScan2 = std::get<0>(wrappers2);

    infoScan1->setScanInterval({0, 1});
    infoScan2->setScanInterval({1, 2});
    infoScan1->setPosition(0 /*detector index*/, V3D(0, 0, 0));

    infoScan1->merge(*infoScan2);

    TS_ASSERT_EQUALS(infoScan1->size() * infoScan1->scanCount(), infoScan1->size() + infoScan2->size());
    TS_ASSERT_EQUALS(infoScan1->position({0 /*detector index*/, 0}), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(infoScan1->position({0 /*detector index*/, 1}), detectorPos);
  }

  void test_throws_if_ComponentType_is_not_Quadrilateral() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 4);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);

    // find quadrilateral component
    size_t structuredIndex = componentInfo->root() - 3;
    // Does not throw for valid index
    TS_ASSERT_THROWS_NOTHING(componentInfo->quadrilateralComponent(structuredIndex));
    // Throws for other non quadrilateral component
    TS_ASSERT_THROWS(componentInfo->quadrilateralComponent(componentInfo->root() - 1), std::runtime_error &);
    // Throws for root
    TS_ASSERT_THROWS(componentInfo->quadrilateralComponent(componentInfo->root()), std::runtime_error &);
    // Throws for detector
    TS_ASSERT_THROWS(componentInfo->quadrilateralComponent(0), std::runtime_error &);
  }

  void test_QuadrilateralComponent_for_single_rectangular_bank() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 4);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);

    // find quadrilateral component
    size_t structuredIndex = componentInfo->root() - 3;
    auto panel = componentInfo->quadrilateralComponent(structuredIndex);
    TS_ASSERT_EQUALS(panel.nX, 4);
    TS_ASSERT_EQUALS(panel.nY, 4);
    TS_ASSERT_EQUALS(panel.bottomLeft, 0);
    TS_ASSERT_EQUALS(panel.bottomRight, 12);
    TS_ASSERT_EQUALS(panel.topLeft, 3);
    TS_ASSERT_EQUALS(panel.topRight, 15);
  }

  void test_has_detectors() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(1 /*1 bank*/, 4 /*4 by 4 pixels*/);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);

    TS_ASSERT(componentInfo->hasDetectors(componentInfo->root()));
    TS_ASSERT(!componentInfo->hasDetectors(0));
    TS_ASSERT(!componentInfo->hasDetectors(componentInfo->sample()));
    TS_ASSERT(!componentInfo->hasDetectors(componentInfo->source()));
    TS_ASSERT(componentInfo->hasDetectors(componentInfo->indexOfAny("bank1")));
  }
};
