#ifndef MANTID_GEOMETRY_COMPONENTINFOTEST_H_
#define MANTID_GEOMETRY_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/Sphere.h"
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

// Make a Beamline ComponentInfo for a single component
std::unique_ptr<Beamline::ComponentInfo> makeSingleComponentInfo(
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
      std::make_pair(0, 0)); // One component with no sub-components

  auto parentIndices = boost::make_shared<const std::vector<size_t>>(
      std::vector<size_t>()); // These indices are invalid, but that's
                              // ok as not being tested here

  auto positions =
      boost::make_shared<std::vector<Eigen::Vector3d>>(1, position);
  auto rotations =
      boost::make_shared<std::vector<Eigen::Quaterniond>>(1, rotation);
  auto scaleFactors =
      boost::make_shared<std::vector<Eigen::Vector3d>>(1, scaleFactor);
  return Kernel::make_unique<Beamline::ComponentInfo>(
      detectorIndices, detectorRanges, componentIndices, componentRanges,
      parentIndices, positions, rotations, scaleFactors, -1, -1);
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
    auto internalInfo = Kernel::make_unique<Beamline::ComponentInfo>(
        detectorIndices, detectorRanges, componentIndices, componentRanges,
        parentIndices, positions, rotations, scaleFactors, -1, -1);
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

  void test_simple_solidAngle() {
    auto position = Eigen::Vector3d{0, 0, 0};
    // No rotation
    auto rotation = Eigen::Quaterniond(Eigen::Affine3d::Identity().rotation());
    auto internalInfo = makeSingleComponentInfo(position, rotation);
    Mantid::Geometry::ObjComponent comp1("component1", createCappedCylinder());

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1});

    const double radius = 1.0;
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
  }

  // Test adapted from ObjComponentTest
  void test_solidAngle() {

    auto position = Eigen::Vector3d{10, 0, 0};
    auto rotation = Eigen::Quaterniond(
        Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
    auto internalInfo = makeSingleComponentInfo(position, rotation);
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

  void test_copy_construction() {

    auto internalInfo = makeSingleComponentInfo();
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
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOTEST_H_ */
