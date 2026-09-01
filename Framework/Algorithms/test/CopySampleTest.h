// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/CopySample.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"

#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CopySampleTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CopySample alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  Sample createsample() {
    Sample sample;
    sample.setName("test");
    const std::string envName("TestKit");
    auto canShape = ComponentCreationHelper::cappedCylinderXML(0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
    auto kit =
        std::make_unique<SampleEnvironment>(envName, std::make_shared<Container>(ShapeFactory().createShape(canShape)));
    sample.setEnvironment(std::move(kit));
    sample.setOrientedLattice(std::make_unique<OrientedLattice>(1.0, 2.0, 3.0, 90, 90, 90));
    auto shape_sptr = ComponentCreationHelper::createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    shape_sptr->setMaterial(Material("vanBlock", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072));
    sample.setShape(shape_sptr);
    return sample;
  }

  // ~~~~ Baking the destination's goniometer into the copied shape ~~~~
  //
  // CopySample leaves the copy baked to exactly the destination workspace's goniometer, which is
  // what makes a shape's applied rotation mean "which frame is this in". Only the part of that
  // rotation not already present is applied, so the source's own state matters: these cover a shape
  // in its own frame, one already baked to the same rotation, and one carrying a rotation of the
  // shape within its own frame that has to survive the copy.

  static Matrix<double> ninetyAboutZ() { return Matrix<double>(std::vector<double>{0, -1, 0, 1, 0, 0, 0, 0, 1}); }

  /// A sphere centred on +x, so where it ends up says what rotation was applied to it.
  static std::string sphereOnX() { return ComponentCreationHelper::sphereXML(0.5, V3D(1.0, 0.0, 0.0), "sphere"); }

  WorkspaceSingleValue_sptr copyShapeOnto(const Geometry::IObject_sptr &sourceShape, const Matrix<double> &destR,
                                          const std::string &suffix) {
    WorkspaceSingleValue_sptr source(new WorkspaceSingleValue(1, 1));
    WorkspaceSingleValue_sptr dest(new WorkspaceSingleValue(1, 1));
    source->mutableSample().setShape(sourceShape);
    dest->mutableRun().mutableGoniometer().setR(destR);

    const std::string inWSName("CopySampleTest_in_" + suffix);
    const std::string outWSName("CopySampleTest_out_" + suffix);
    AnalysisDataService::Instance().addOrReplace(inWSName, source);
    AnalysisDataService::Instance().addOrReplace(outWSName, dest);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyName", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyEnvironment", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyLattice", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
    return dest;
  }

  void test_shape_in_its_own_frame_is_baked_to_the_destination() {
    auto shape = ShapeFactory().createShape(sphereOnX());
    TS_ASSERT_EQUALS(shape->getAppliedRotation(), Matrix<double>(3, 3, true));

    auto dest = copyShapeOnto(shape, ninetyAboutZ(), "own_frame");

    const auto &copied = dest->sample().getShape();
    TS_ASSERT_EQUALS(copied.getAppliedRotation(), ninetyAboutZ());
    TS_ASSERT(copied.isValid(V3D(0.0, 1.0, 0.0)));
    TS_ASSERT(!copied.isValid(V3D(1.0, 0.0, 0.0)));
  }

  void test_shape_already_baked_to_the_destination_is_not_rotated_twice() {
    // the source came from a workspace with the same goniometer, so there is nothing left to apply
    const auto xml = ShapeFactory().rebakeGoniometer(ninetyAboutZ(), sphereOnX(), Matrix<double>(3, 3, true));
    auto shape = ShapeFactory().createShape(xml);
    TS_ASSERT_EQUALS(shape->getAppliedRotation(), ninetyAboutZ());
    TS_ASSERT(shape->isValid(V3D(0.0, 1.0, 0.0)));

    auto dest = copyShapeOnto(shape, ninetyAboutZ(), "already_baked");

    const auto &copied = dest->sample().getShape();
    TS_ASSERT_EQUALS(copied.getAppliedRotation(), ninetyAboutZ());
    // still on +y, not carried round to -x
    TS_ASSERT(copied.isValid(V3D(0.0, 1.0, 0.0)));
    TS_ASSERT(!copied.isValid(V3D(-1.0, 0.0, 0.0)));
  }

  void test_rotation_of_the_shape_in_its_own_frame_survives_the_copy() {
    // a shape turned 90 degrees within its own frame, as RotateSampleShape leaves it, and still
    // unbaked. Copying onto a 90 degree destination has to add to that rotation, not replace it -
    // the CSG branch used to overwrite the tag and silently lose it.
    auto xml = ShapeFactory().addGoniometerTag(ninetyAboutZ(), sphereOnX());
    xml = ShapeFactory().addAppliedGoniometerTag(Matrix<double>(3, 3, true), xml);
    auto shape = ShapeFactory().createShape(xml);
    TS_ASSERT_EQUALS(shape->getAppliedRotation(), Matrix<double>(3, 3, true));

    auto dest = copyShapeOnto(shape, ninetyAboutZ(), "definition_frame");

    const auto &copied = dest->sample().getShape();
    TS_ASSERT_EQUALS(copied.getAppliedRotation(), ninetyAboutZ());
    // the shape's own 90 plus the destination's 90 carries it round to -x
    TS_ASSERT(copied.isValid(V3D(-1.0, 0.0, 0.0)));
    TS_ASSERT(!copied.isValid(V3D(0.0, 1.0, 0.0)));
  }

  void test_mesh_shape_already_baked_to_the_destination_is_not_rotated_twice() {
    // the mesh branch composed unconditionally, so this case used to turn the cube twice
    auto cube = createOffsetCube();
    cube->bakeGoniometerRotation(ninetyAboutZ());
    TS_ASSERT_DELTA(cube->getBoundingBox().yMin(), 4.0, 1e-8);

    auto dest = copyShapeOnto(std::move(cube), ninetyAboutZ(), "mesh_already_baked");

    const auto &copied = dest->sample().getShape();
    TS_ASSERT_EQUALS(copied.getAppliedRotation(), ninetyAboutZ());
    // still on +y rather than carried round to -x
    TS_ASSERT_DELTA(copied.getBoundingBox().yMin(), 4.0, 1e-8);
    TS_ASSERT_DELTA(copied.getBoundingBox().yMax(), 6.0, 1e-8);
  }

  void test_mesh_shape_in_its_own_frame_is_baked_to_the_destination() {
    auto dest = copyShapeOnto(createOffsetCube(), ninetyAboutZ(), "mesh_own_frame");

    const auto &copied = dest->sample().getShape();
    TS_ASSERT_EQUALS(copied.getAppliedRotation(), ninetyAboutZ());
    TS_ASSERT_DELTA(copied.getBoundingBox().yMin(), 4.0, 1e-8);
    TS_ASSERT_DELTA(copied.getBoundingBox().yMax(), 6.0, 1e-8);
  }

  /// A cube of side 2 centred on (5, 0, 0), so it spans x = 4..6 and a turn about z is visible.
  static std::shared_ptr<Geometry::MeshObject> createOffsetCube() {
    const V3D centre(5.0, 0.0, 0.0);
    std::vector<V3D> vertices{centre + V3D(1, 1, 1),   centre + V3D(-1, 1, 1),  centre + V3D(1, -1, 1),
                              centre + V3D(-1, -1, 1), centre + V3D(1, 1, -1),  centre + V3D(-1, 1, -1),
                              centre + V3D(1, -1, -1), centre + V3D(-1, -1, -1)};
    std::vector<uint32_t> triangles{0, 1, 2, 2, 1, 3, 0, 2, 4, 4, 2, 6, 0, 4, 1, 1, 4, 5,
                                    7, 5, 6, 6, 5, 4, 7, 3, 5, 5, 3, 1, 7, 6, 3, 3, 6, 2};
    return std::make_shared<Geometry::MeshObject>(std::move(triangles), std::move(vertices), Material());
  }

  void test_exec_all() {
    WorkspaceSingleValue_sptr ws1(new WorkspaceSingleValue(1, 1));
    WorkspaceSingleValue_sptr ws2(new WorkspaceSingleValue(4, 2));
    Sample s = createsample();
    ws1->mutableSample() = s;

    // Name of the output workspace.
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ws1);
    AnalysisDataService::Instance().add(outWSName, ws2);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyName", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyMaterial", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyEnvironment", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyShape", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyLattice", "1"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    Sample copy = ws->mutableSample();
    TS_ASSERT_EQUALS(copy.getName(), "test");
    TS_ASSERT_EQUALS(copy.getOrientedLattice().c(), 3.0);
    TS_ASSERT_EQUALS(copy.getEnvironment().name(), "TestKit");
    TS_ASSERT_EQUALS(copy.getEnvironment().nelements(), 1);
    TS_ASSERT_DELTA(copy.getMaterial().cohScatterXSection(), 0.0184, 1e-02);
    TS_ASSERT_EQUALS(copy.getShape().getName(), s.getShape().getName());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_some() {
    WorkspaceSingleValue_sptr ws1(new WorkspaceSingleValue(1, 1));
    WorkspaceSingleValue_sptr ws2(new WorkspaceSingleValue(4, 2));
    Sample s = createsample();
    ws1->mutableSample() = s;

    // Name of the output workspace.
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ws1);
    AnalysisDataService::Instance().add(outWSName, ws2);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyName", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyMaterial", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyEnvironment", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyShape", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyLattice", "0"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    Sample copy = ws->mutableSample();
    TS_ASSERT_DIFFERS(copy.getName(), "test");
    TS_ASSERT(!copy.hasOrientedLattice());
    TS_ASSERT_EQUALS(copy.getEnvironment().name(), "TestKit");
    TS_ASSERT_EQUALS(copy.getEnvironment().nelements(), 1);
    TS_ASSERT_DELTA(copy.getMaterial().cohScatterXSection(), 0.0184, 1e-02);
    TS_ASSERT_DIFFERS(copy.getShape().getName(), s.getShape().getName());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_orientation() {
    WorkspaceSingleValue_sptr ws1(new WorkspaceSingleValue(1, 1));
    WorkspaceSingleValue_sptr ws2(new WorkspaceSingleValue(4, 2));
    Sample s = createsample();
    ws1->mutableSample() = s;

    // Name of the output workspace.
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ws1);
    AnalysisDataService::Instance().add(outWSName, ws2);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyName", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyMaterial", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyEnvironment", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyShape", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyLattice", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyOrientationOnly", "1"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    Sample copy = ws->mutableSample();
    TS_ASSERT(copy.hasOrientedLattice());
    TS_ASSERT_EQUALS(copy.getOrientedLattice().getUB(), s.getOrientedLattice().getUB());

    // modify the first unit cell, both U and B
    s.getOrientedLattice().setUFromVectors(V3D(1, 1, 0), V3D(1, -1, 0));
    s.getOrientedLattice().seta(1.1);
    ws1->mutableSample() = s;
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    copy = ws->mutableSample();
    TS_ASSERT(copy.hasOrientedLattice());
    TS_ASSERT_DIFFERS(copy.getOrientedLattice().a(),
                      s.getOrientedLattice().a()); // different B matrix
    TS_ASSERT_EQUALS(copy.getOrientedLattice().getU(),
                     s.getOrientedLattice().getU()); // same U
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_MDcopy() {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDEvent<3>, 3>());
    TS_ASSERT_EQUALS(ew->getNumExperimentInfo(), 0);
    ExperimentInfo_sptr ei(new ExperimentInfo);
    ExperimentInfo_sptr ei1(new ExperimentInfo);
    Sample s = createsample();
    Sample s1;
    s1.setOrientedLattice(std::make_unique<OrientedLattice>(6.0, 7.0, 8.0, 90, 90, 90));
    s1.setName("newsample");
    ei->mutableSample() = s;
    TS_ASSERT_EQUALS(ew->addExperimentInfo(ei), 0);
    TS_ASSERT_EQUALS(ew->addExperimentInfo(ei), 1);
    ei1->mutableSample() = s1;
    TS_ASSERT_EQUALS(ew->addExperimentInfo(ei1), 2);
    TS_ASSERT_EQUALS(ew->getNumExperimentInfo(), 3);
    TS_ASSERT_EQUALS(ew->getExperimentInfo(1)->sample().getOrientedLattice().c(), 3);
    TS_ASSERT_EQUALS(ew->getExperimentInfo(2)->sample().getOrientedLattice().c(), 8);

    IMDEventWorkspace_sptr ewout(new MDEventWorkspace<MDEvent<3>, 3>());
    ExperimentInfo_sptr eiout0(new ExperimentInfo);
    eiout0->mutableSample() = s;
    ExperimentInfo_sptr eiout1(new ExperimentInfo);
    ExperimentInfo_sptr eiout2(new ExperimentInfo);
    ExperimentInfo_sptr eiout3(new ExperimentInfo);
    TS_ASSERT_EQUALS(ewout->addExperimentInfo(eiout0), 0);
    TS_ASSERT_EQUALS(ewout->addExperimentInfo(eiout1), 1);
    TS_ASSERT_EQUALS(ewout->addExperimentInfo(eiout2), 2);
    TS_ASSERT_EQUALS(ewout->addExperimentInfo(eiout3), 3);
    TS_ASSERT(ewout->getExperimentInfo(0)->sample().hasOrientedLattice());
    TS_ASSERT(!ewout->getExperimentInfo(1)->sample().hasOrientedLattice());
    TS_ASSERT(!ewout->getExperimentInfo(2)->sample().hasOrientedLattice());
    TS_ASSERT(!ewout->getExperimentInfo(3)->sample().hasOrientedLattice());

    // run algorithm twice: set all samples to s1, then set sample in last
    // experiment info to s
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ew);
    AnalysisDataService::Instance().add(outWSName, ewout);
    CopySample alg, alg1;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyName", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyMaterial", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyEnvironment", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyShape", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CopyLattice", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MDInputSampleNumber", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MDOutputSampleNumber", "-1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("CopyName", "1"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("CopyMaterial", "0"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("CopyEnvironment", "0"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("CopyShape", "0"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("CopyLattice", "1"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("MDInputSampleNumber", "0"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("MDOutputSampleNumber", "3"));
    TS_ASSERT_THROWS_NOTHING(alg1.execute(););
    TS_ASSERT(alg1.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // test output
    TS_ASSERT(ws->getExperimentInfo(0)->sample().hasOrientedLattice());
    TS_ASSERT(ws->getExperimentInfo(1)->sample().hasOrientedLattice());
    TS_ASSERT(ws->getExperimentInfo(2)->sample().hasOrientedLattice());
    TS_ASSERT(ws->getExperimentInfo(3)->sample().hasOrientedLattice());
    TS_ASSERT_EQUALS(ws->getExperimentInfo(0)->sample().getOrientedLattice().a(), 6);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(1)->sample().getOrientedLattice().c(), 8);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(2)->sample().getOrientedLattice().c(), 8);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(3)->sample().getOrientedLattice().c(), 3);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(0)->sample().getName(), "newsample");
    TS_ASSERT_EQUALS(ws->getExperimentInfo(1)->sample().getName(), "newsample");
    TS_ASSERT_EQUALS(ws->getExperimentInfo(2)->sample().getName(), "newsample");
    TS_ASSERT_EQUALS(ws->getExperimentInfo(3)->sample().getName(), "test");
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }
};
