// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/EstimateScatteringVolumeCentreOfMass.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::V3D;
class EstimateScatteringVolumeCentreOfMassTest : public CxxTest::TestSuite {
public:
  void testInit() {
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    TS_ASSERT_THROWS_NOTHING(centerOfMass.initialize());
    TS_ASSERT(centerOfMass.isInitialized());
  }
  void testExecWithCylinderSample() {
    // Create a test workspace with cylinder sample
    MatrixWorkspace_sptr testWS = createWorkspaceWithCylinderSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 1.0); // 1mm cubes
    TS_ASSERT_THROWS_NOTHING(centerOfMass.execute());
    TS_ASSERT(centerOfMass.isExecuted());
    // Check output
    std::vector<double> resultVec = centerOfMass.getProperty("CentreOfMass");
    V3D result(resultVec[0], resultVec[1], resultVec[2]);
    // For a symmetric cylinder along y-axis centered at origin,
    TS_ASSERT_DELTA(result.X(), 0.0, 0.00001);
    TS_ASSERT_DELTA(result.Y(), 0.0, 0.00001);
    TS_ASSERT_DELTA(result.Z(), 0.0, 0.00001);
  }
  void testExecWithOffsetCylinderSample() {
    // Create a test workspace with offset cylinder sample
    MatrixWorkspace_sptr testWS = createWorkspaceWithOffsetCylinderSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 1.0); // 1mm cubes
    TS_ASSERT_THROWS_NOTHING(centerOfMass.execute());
    TS_ASSERT(centerOfMass.isExecuted());
    // Check output
    std::vector<double> resultVec = centerOfMass.getProperty("CentreOfMass");
    V3D result(resultVec[0], resultVec[1], resultVec[2]);
    // For a cylinder with center offset to (0,-0.01,0),
    // expect the center of mass to be close to this
    TS_ASSERT_DELTA(result.X(), 0.0, 0.00002);
    TS_ASSERT_DELTA(result.Y(), -0.01, 0.00002);
    TS_ASSERT_DELTA(result.Z(), 0.0, 0.00002);
  }
  void testExecWithGaugeVolume() {
    // Create a test workspace with cylinder sample and sphere gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithOffsetCylinderSampleAndGaugeVolume();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 1.0); // 1mm cubes
    TS_ASSERT_THROWS_NOTHING(centerOfMass.execute());
    TS_ASSERT(centerOfMass.isExecuted());
    // Check output
    std::vector<double> resultVec = centerOfMass.getProperty("CentreOfMass");
    V3D result(resultVec[0], resultVec[1], resultVec[2]);
    // For a cubic gauge volume at (0,0,0), expect center of mass near that point
    // despite the offset cylinder having a c.o.m at (0,-0.01,0)
    TS_ASSERT_DELTA(result.X(), 0.0, 0.00002);
    TS_ASSERT_DELTA(result.Y(), 0.0, 0.00002);
    TS_ASSERT_DELTA(result.Z(), 0.0, 0.00002);
  }
  void testExecWithPartiallyIlluminatedSample() {
    // Create a test workspace with cylinder sample and sphere gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithPartiallyIlluminatedSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 1.0); // 1mm cubes
    TS_ASSERT_THROWS_NOTHING(centerOfMass.execute());
    TS_ASSERT(centerOfMass.isExecuted());
    // Check output
    std::vector<double> resultVec = centerOfMass.getProperty("CentreOfMass");
    V3D result(resultVec[0], resultVec[1], resultVec[2]);
    // cube is centre at (0.01,0.01,0.01) with sides 0.02
    // For a cubic "gauge volume" at (0,0,0), with sides 0.02
    // illuminated volume should be a cube centred on (0.005, 0.005, 0.005)
    TS_ASSERT_DELTA(result.X(), 0.005, 0.00002);
    TS_ASSERT_DELTA(result.Y(), 0.005, 0.00002);
    TS_ASSERT_DELTA(result.Z(), 0.005, 0.00002);
  }
  void testWithoutSample() {
    // Create a workspace without sample shape
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.setRethrows(true);
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    // This should throw because no sample shape is defined
    TS_ASSERT_THROWS(centerOfMass.execute(), const std::invalid_argument &);
    TS_ASSERT(!centerOfMass.isExecuted());
  }
  void testExecWithDifferentElementSizeUnits() {
    // Create a test workspace with cylinder sample
    MatrixWorkspace_sptr testWS = createWorkspaceWithCylinderSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementUnits", "m");
    centerOfMass.setProperty("ElementSize", 0.001); // 1mm cubes
    TS_ASSERT_THROWS_NOTHING(centerOfMass.execute());
    TS_ASSERT(centerOfMass.isExecuted());
    // Check output
    std::vector<double> resultVec = centerOfMass.getProperty("CentreOfMass");
    V3D result(resultVec[0], resultVec[1], resultVec[2]);
    // For a symmetric cylinder along y-axis centered at origin,
    TS_ASSERT_DELTA(result.X(), 0.0, 0.00001);
    TS_ASSERT_DELTA(result.Y(), 0.0, 0.00001);
    TS_ASSERT_DELTA(result.Z(), 0.0, 0.00001);
  }
  void testBadElementUnitsThrowsError() {
    // Create a test workspace with cylinder sample
    MatrixWorkspace_sptr testWS = createWorkspaceWithCylinderSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    TS_ASSERT_THROWS(centerOfMass.setProperty("ElementUnits", "um"), const std::invalid_argument &);
  }
  void testErrorIfNoSampleIlluminated() {
    // Create a test workspace with cylinder sample and sphere gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithUnilluminatedSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 1.0); // 1mm cubes
    TS_ASSERT_THROWS(centerOfMass.execute(), const std::runtime_error &);
    TS_ASSERT(!centerOfMass.isExecuted());
  }

private:
  MatrixWorkspace_sptr createTestWorkspace() {
    // Create a basic test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    return testWS;
  }
  MatrixWorkspace_sptr createWorkspaceWithAnyOffsetCylinderSample(std::vector<double> offset) {
    // Create a workspace with an offset cylinder sample
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    // Create the geometry (cylinder with offset center)
    auto geometry = std::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty("Shape", "Cylinder");
    geometry->declareProperty("Height", 4);
    geometry->declareProperty("Radius", 1);
    geometry->declareProperty("Center", std::move(offset));
    std::vector<double> axis{0, 1, 0}; // y-axis
    geometry->declareProperty("Axis", axis);
    // Set the sample information
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", testWS);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
    return testWS;
  }
  MatrixWorkspace_sptr createWorkspaceWithCylinderSample() {
    return createWorkspaceWithAnyOffsetCylinderSample(std::vector<double>{0.0, 0.0, 0.0});
  }
  MatrixWorkspace_sptr createWorkspaceWithOffsetCylinderSample() {
    return createWorkspaceWithAnyOffsetCylinderSample(std::vector<double>{0.0, -1.0, 0.0});
  }
  MatrixWorkspace_sptr createWorkspaceWithOffsetCubeSample() {
    // Create a workspace with an offset cylinder sample
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    // Create the geometry (cylinder with offset center)
    auto geometry = std::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty("Shape", "FlatPlate");
    geometry->declareProperty("Height", 2);
    geometry->declareProperty("Width", 2);
    geometry->declareProperty("Thick", 2);
    std::vector<double> center{1, 1, 1}; // Offset center
    geometry->declareProperty("Center", std::move(center));
    geometry->declareProperty("Angle", 0);
    // Set the sample information
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", testWS);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
    return testWS;
  }
  MatrixWorkspace_sptr createWorkspaceWithOffsetCylinderSampleAndGaugeVolume() {
    // Create workspace with cylinder sample and add gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithOffsetCylinderSample();
    // Define a cubic gauge volume
    const std::string gaugeXML = " \
        <cuboid id='some-cuboid'> \
        <height val='0.01'  /> \
        <width val='0.01' />  \
        <depth  val='0.01' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='some-cuboid' /> \
        ";
    // Add the gauge volume to the run properties
    testWS->mutableRun().addProperty("GaugeVolume", gaugeXML);
    return testWS;
  }
  MatrixWorkspace_sptr createWorkspaceWithUnilluminatedSample() {
    // Create workspace with cylinder sample and add gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithAnyOffsetCylinderSample(std::vector<double>{10.0, 10.0, 10.0});
    // Define a cubic gauge volume
    const std::string gaugeXML = " \
        <cuboid id='some-cuboid'> \
        <height val='0.01'  /> \
        <width val='0.01' />  \
        <depth  val='0.01' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='some-cuboid' /> \
        ";
    // Add the gauge volume to the run properties
    testWS->mutableRun().addProperty("GaugeVolume", gaugeXML);
    return testWS;
  }
  MatrixWorkspace_sptr createWorkspaceWithPartiallyIlluminatedSample() {
    // Create workspace with cylinder sample and add gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithOffsetCubeSample();
    // Define a cubic gauge volume
    const std::string gaugeXML = " \
        <cuboid id='some-cuboid'> \
        <height val='0.02'  /> \
        <width val='0.02' />  \
        <depth  val='0.02' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='some-cuboid' /> \
        ";
    // Add the gauge volume to the run properties
    testWS->mutableRun().addProperty("GaugeVolume", gaugeXML);
    return testWS;
  }
};
