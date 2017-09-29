#ifndef MANTID_GEOMETRY_COMPONENTINFOTEST_H_
#define MANTID_GEOMETRY_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <Eigen/Geometry>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

/*
 Helper function to create an ID -> index map from an ordered collection of IDs.
 First ID gets index of 0, subsequent ID entries increment index by 1.
*/
boost::shared_ptr<
    const std::unordered_map<Mantid::Geometry::ComponentID, size_t>>
makeComponentIDMap(const boost::shared_ptr<
    const std::vector<Mantid::Geometry::ComponentID>> &componentIds) {
  auto idMap = boost::make_shared<
      std::unordered_map<Mantid::Geometry::ComponentID, size_t>>();

  for (size_t i = 0; i < componentIds->size(); ++i) {
    (*idMap)[(*componentIds)[i]] = i;
  }
  return idMap;
}

boost::shared_ptr<Object> createCappedCylinder() {
  std::string C31 = "cx 0.5"; // cylinder x-axis radius 0.5
  std::string C32 = "px 1.2";
  std::string C33 = "px -3.2";

  // First create some surfaces
  std::map<int, boost::shared_ptr<Surface>> CylSurMap;
  CylSurMap[31] = boost::make_shared<Cylinder>();
  CylSurMap[32] = boost::make_shared<Plane>();
  CylSurMap[33] = boost::make_shared<Plane>();

  CylSurMap[31]->setSurface(C31);
  CylSurMap[32]->setSurface(C32);
  CylSurMap[33]->setSurface(C33);
  CylSurMap[31]->setName(31);
  CylSurMap[32]->setName(32);
  CylSurMap[33]->setName(33);

  // Capped cylinder (id 21)
  // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
  std::string ObjCapCylinder = "-31 -32 33";

  auto retVal = boost::make_shared<Object>();
  retVal->setObject(21, ObjCapCylinder);
  retVal->populate(CylSurMap);

  return retVal;
}

// Make a Beamline ComponentInfo for a single component
std::unique_ptr<Beamline::ComponentInfo> makeSingleBeamlineComponentInfo(
    Eigen::Vector3d position = Eigen::Vector3d{1, 1, 1},
    Eigen::Quaterniond rotation =
        Eigen::Quaterniond(Eigen::Affine3d::Identity().rotation()),
    Eigen::Vector3d scaleFactor = Eigen::Vector3d{1, 1, 1}) {

  auto detectorIndices =
      boost::make_shared<std::vector<size_t>>(); // No detectors in this example
  auto detectorRanges =
      boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
  detectorRanges->push_back(
      std::make_pair(0, 0)); // One component with no detectors

  auto componentIndices = boost::make_shared<std::vector<size_t>>(
      std::vector<size_t>{0}); // No detectors in this example
  auto componentRanges =
      boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
  componentRanges->push_back(
      std::make_pair(0, 1)); // One component with no sub-components

  auto parentIndices = boost::make_shared<const std::vector<size_t>>(
      std::vector<size_t>()); // These indices are invalid, but that's
                              // ok as not being tested here

  auto positions =
      boost::make_shared<std::vector<Eigen::Vector3d>>(1, position);
  auto rotations =
      boost::make_shared<std::vector<Eigen::Quaterniond>>(1, rotation);
  auto scaleFactors =
      boost::make_shared<std::vector<Eigen::Vector3d>>(1, scaleFactor);
  auto isStructuredBank = boost::make_shared<std::vector<bool>>(1, false);
  return Kernel::make_unique<Beamline::ComponentInfo>(
      detectorIndices, detectorRanges, componentIndices, componentRanges,
      parentIndices, positions, rotations, scaleFactors, isStructuredBank, -1,
      -1);
}
}

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_indexOf() {
    auto detectorIndices = boost::make_shared<
        std::vector<size_t>>(); // No detectors in this example
    auto detectorRanges =
        boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
    detectorRanges->push_back(
        std::make_pair(0, 0)); // One component with no detectors
    detectorRanges->push_back(
        std::make_pair(0, 0)); // Another component with no detectors

    auto componentIndices = boost::make_shared<std::vector<size_t>>(
        std::vector<size_t>{0, 1}); // No detectors in this example
    auto componentRanges =
        boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
    componentRanges->push_back(
        std::make_pair(0, 0)); // One component with no sub-components
    componentRanges->push_back(
        std::make_pair(0, 0)); // Another component with no subcomponents

    auto parentIndices = boost::make_shared<const std::vector<size_t>>(
        std::vector<size_t>{9, 9, 9}); // These indices are invalid, but that's
                                       // ok as not being tested here

    auto positions = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto rotations = boost::make_shared<std::vector<Eigen::Quaterniond>>(2);
    auto scaleFactors = boost::make_shared<std::vector<Eigen::Vector3d>>(2);
    auto isRectBank = boost::make_shared<std::vector<bool>>(2);
    auto internalInfo = Kernel::make_unique<Beamline::ComponentInfo>(
        detectorIndices, detectorRanges, componentIndices, componentRanges,
        parentIndices, positions, rotations, scaleFactors, isRectBank, -1, -1);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1, &comp2});

    auto shapes = boost::make_shared<
        std::vector<boost::shared_ptr<const Geometry::Object>>>();
    shapes->push_back(boost::make_shared<const Geometry::Object>());
    shapes->push_back(boost::make_shared<const Geometry::Object>());

    ComponentInfo info(std::move(internalInfo), componentIds,
                       makeComponentIDMap(componentIds), shapes);
    TS_ASSERT_EQUALS(info.indexOf(comp1.getComponentID()), 0);
    TS_ASSERT_EQUALS(info.indexOf(comp2.getComponentID()), 1);
  }

  void test_copy_construction() {
    auto internalInfo = makeSingleBeamlineComponentInfo();
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = boost::make_shared<
        std::vector<boost::shared_ptr<const Geometry::Object>>>();
    shapes->push_back(createCappedCylinder());

    ComponentInfo a(std::move(internalInfo), componentIds,
                    makeComponentIDMap(componentIds), shapes);

    // Make the copy
    ComponentInfo b = a;

    // Compare sizes
    TS_ASSERT_EQUALS(b.size(), a.size());
    // Shapes are the same
    TS_ASSERT_EQUALS(&b.shape(0), &a.shape(0));
    // IDs are the same
    TS_ASSERT_EQUALS(b.indexOf(&comp1), a.indexOf(&comp1));
  }

  void test_has_shape() {
    auto internalInfo = makeSingleBeamlineComponentInfo();
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = boost::make_shared<
        std::vector<boost::shared_ptr<const Geometry::Object>>>();
    shapes->push_back(createCappedCylinder());

    ComponentInfo compInfo(std::move(internalInfo), componentIds,
                           makeComponentIDMap(componentIds), shapes);

    TS_ASSERT(compInfo.hasShape(0));
    // Nullify the shape of the component
    shapes->at(0) = boost::shared_ptr<const Geometry::Object>(nullptr);
    TS_ASSERT(!compInfo.hasShape(0));
    TS_ASSERT_THROWS(compInfo.solidAngle(0, V3D{1, 1, 1}),
                     Mantid::Kernel::Exception::NullPointerException &);
  }

  void test_simple_solidAngle() {
    auto position = Eigen::Vector3d{0, 0, 0};
    // No rotation
    const double radius = 1.0;
    auto rotation = Eigen::Quaterniond(Eigen::Affine3d::Identity().rotation());

    auto internalInfo = makeSingleBeamlineComponentInfo(position, rotation);
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = boost::make_shared<
        std::vector<boost::shared_ptr<const Geometry::Object>>>();
    shapes->push_back(ComponentCreationHelper::createSphere(radius));

    ComponentInfo info(std::move(internalInfo), componentIds,
                       makeComponentIDMap(componentIds), shapes);

    double satol = 1e-9; // tolerance for solid angle

    // Put observer on surface of sphere and solid angle is 2PI
    V3D observer{radius, 0, 0};
    TS_ASSERT_DELTA(info.solidAngle(0, observer), 2 * M_PI, satol);
    // Put observer at center of sphere and solid angle is full 4PI square
    // radians
    observer = V3D{0, 0, 0};
    TS_ASSERT_DELTA(info.solidAngle(0, observer), 4 * M_PI, satol);
    // Nullify  the shape and retest solid angle
    shapes->at(0) = boost::shared_ptr<const Geometry::Object>(nullptr);
    TS_ASSERT_THROWS(info.solidAngle(0, observer),
                     Mantid::Kernel::Exception::NullPointerException &);
  }

  // Test adapted from ObjComponentTest
  void test_solidAngle() {

    auto position = Eigen::Vector3d{10, 0, 0};
    auto rotation = Eigen::Quaterniond(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
    auto internalInfo = makeSingleBeamlineComponentInfo(position, rotation);
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = boost::make_shared<
        std::vector<boost::shared_ptr<const Geometry::Object>>>();
    shapes->push_back(createCappedCylinder());

    ComponentInfo info(std::move(internalInfo), componentIds,
                       makeComponentIDMap(componentIds), shapes);

    double satol = 2e-2; // tolerance for solid angle
    TS_ASSERT_DELTA(info.solidAngle(0, V3D(10, 1.7, 0)), 1.840302, satol);
  }

  void test_boundingBox_single_component() {

    const double radius = 2;
    Eigen::Vector3d position{1, 1, 1};
    auto internalInfo = makeSingleBeamlineComponentInfo(position);
    Mantid::Geometry::ObjComponent comp1(
        "component1", ComponentCreationHelper::createSphere(radius));

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1});

    auto shapes = boost::make_shared<
        std::vector<boost::shared_ptr<const Geometry::Object>>>();
    shapes->push_back(ComponentCreationHelper::createSphere(radius));

    ComponentInfo componentInfo(std::move(internalInfo), componentIds,
                                makeComponentIDMap(componentIds), shapes);

    BoundingBox boundingBox = componentInfo.boundingBox(0 /*componentIndex*/);

    TS_ASSERT((boundingBox.minPoint() -
               (Kernel::V3D{position[0] - radius, position[1] - radius,
                            position[2] - radius})).norm() < 1e-9);
    TS_ASSERT((boundingBox.maxPoint() -
               (Kernel::V3D{position[0] + radius, position[1] + radius,
                            position[2] + radius})).norm() < 1e-9);
    // Nullify shape and retest BoundingBox
    shapes->at(0) = boost::shared_ptr<const Geometry::Object>(nullptr);
    boundingBox = componentInfo.boundingBox(0);
    TS_ASSERT(boundingBox.isNull());
  }

  void test_boundingBox_complex() {
    const V3D sourcePos(-1, 0, 0);
    const V3D samplePos(0, 0, 0);
    const V3D detectorPos(11, 0, 0);
    const double radius = 0.01; // See helper creation method for definition
    // Create a very basic real instrument to visit
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        sourcePos, samplePos, detectorPos);

    // CompAssembly (and hence Instrument 1.0) has getter and setter for
    // position!
    instrument->setPos(samplePos);

    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);
    // Check bounding box of detector
    auto boundingBox = componentInfo->boundingBox(0 /*detector index*/);
    TS_ASSERT((boundingBox.minPoint() -
               (Kernel::V3D{detectorPos[0] - radius, detectorPos[1] - radius,
                            detectorPos[2] - radius})).norm() < 1e-9);
    TS_ASSERT((boundingBox.maxPoint() -
               (Kernel::V3D{detectorPos[0] + radius, detectorPos[1] + radius,
                            detectorPos[2] + radius})).norm() < 1e-9);

    // Check bounding box of root (instrument)
    boundingBox = componentInfo->boundingBox(componentInfo->root() /*Root*/);

    // min in the sample (source is ignored by design in instrument 1.0 and
    // instrument 2.0).
    TS_ASSERT((boundingBox.minPoint() -
               (Kernel::V3D{samplePos[0] - radius, samplePos[1] - radius,
                            samplePos[2] - radius})).norm() < 1e-9);
    // max is the detector
    TS_ASSERT((boundingBox.maxPoint() -
               (Kernel::V3D{detectorPos[0] + radius, detectorPos[1] + radius,
                            detectorPos[2] + radius})).norm() < 1e-9);
  }

  void test_boundingBox_around_rectangular_bank() {

    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*1 bank*/, 10 /*10 by 10*/);

    // CompAssembly (and hence Instrument 1.0) has getter and setter for
    // position!
    instrument->setPos(instrument->getSample()->getPos());

    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    const auto &componentInfo = std::get<0>(wrappers);
    // Check bounding box of root (instrument)
    auto boundingBoxRoot =
        componentInfo->boundingBox(componentInfo->root() /*Root*/
                                   );
    // min Z in the sample
    auto boundingBoxSample =
        componentInfo->boundingBox(componentInfo->sample());
    TS_ASSERT((boundingBoxRoot.minPoint().Z() -
               boundingBoxSample.minPoint().Z()) < 1e-9);

    // max is the Rectangular bank
    auto bankIndex = componentInfo->root() - 3;
    TS_ASSERT(componentInfo->isStructuredBank(bankIndex));
    auto boundingBoxBank = componentInfo->boundingBox(bankIndex);
    TS_ASSERT((boundingBoxRoot.maxPoint() - boundingBoxBank.maxPoint()).norm() <
              1e-9);
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
    ComponentCreationHelper::addRectangularBank(
        instrument, idStart, pixels, pixelSpacing, bankName, bankPos, bankRot);

    // A source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D{0, 0, -10});
    instrument.add(source);
    instrument.markAsSource(source);

    // A sample
    ObjComponent *sample = new ObjComponent("some-surface-holder");
    sample->setPos(V3D{0, 0, 0});
    sample->setShape(
        ComponentCreationHelper::createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
    instrument.add(sample);
    instrument.markAsSamplePos(sample);

    // A detector
    Detector *det = new Detector(
        "point-detector", (2 * pixels * pixels) + 1 /*detector id*/, nullptr);
    det->setPos(V3D{0, 0, 3});
    det->setShape(
        ComponentCreationHelper::createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
    instrument.add(det);
    instrument.markAsDetector(det);

    // Add another rectangular bank
    idStart = pixels * pixels;
    bankName = "bank2";
    bankPos = Mantid::Kernel::V3D{0, -1, 2};
    ComponentCreationHelper::addRectangularBank(
        instrument, idStart, pixels, pixelSpacing, bankName, bankPos, bankRot);

    auto wrappers = InstrumentVisitor::makeWrappers(instrument);
    const auto &componentInfo = std::get<0>(wrappers);
    // Check bounding box of root (instrument)
    auto boundingBoxRoot =
        componentInfo->boundingBox(componentInfo->root() /*Root*/);

    // Check free detector not ignored because it's sandwidged between banks.
    // Should not be skipped over.
    const double detRadius = 0.01; // See test helper for value
    TS_ASSERT_DELTA(boundingBoxRoot.maxPoint().Z(),
                    det->getPos().Z() + detRadius, 1e-9);

    // Check bank1 represents max point in y
    const size_t bank1Index = componentInfo->root() - 4 - 10;
    auto boundingBoxBank1 = componentInfo->boundingBox(bank1Index);
    TS_ASSERT(componentInfo->isStructuredBank(bank1Index));
    TS_ASSERT_DELTA(boundingBoxRoot.maxPoint().Y(),
                    boundingBoxBank1.maxPoint().Y(), 1e-9);

    // Check bank2 represents min point in y
    const size_t bank2Index = componentInfo->root() - 1;
    auto boundingBoxBank2 = componentInfo->boundingBox(bank2Index);
    TS_ASSERT(componentInfo->isStructuredBank(bank2Index));
    TS_ASSERT_DELTA(boundingBoxRoot.minPoint().Y(),
                    boundingBoxBank2.minPoint().Y(), 1e-9);
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOTEST_H_ */
