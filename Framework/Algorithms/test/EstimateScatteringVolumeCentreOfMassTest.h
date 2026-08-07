// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/EstimateScatteringVolumeCentreOfMass.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::V3D;
using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;
using Mantid::API::Sample;
using Mantid::Geometry::IObject_sptr;

class EstimateScatteringVolumeCentreOfMassTest : public CxxTest::TestSuite {
public:
  void testInit() {
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    TS_ASSERT_THROWS_NOTHING(centerOfMass.initialize());
    TS_ASSERT(centerOfMass.isInitialized());
  }
  void testErrorIfNoSampleIlluminated() {
    // Create a test workspace with cylinder sample and sphere gauge volume
    MatrixWorkspace_sptr testWS = createWorkspaceWithUnilluminatedSample();
    // Run the algorithm
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.setRethrows(true);
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 1.0); // 1mm cubes
    TS_ASSERT_THROWS(centerOfMass.execute(), const std::runtime_error &);
    TS_ASSERT(!centerOfMass.isExecuted());
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
  void testGoniometerRotatedGaugeClipsSampleCorrectly() {
    // Regression test for the gauge-volume / goniometer composition bug.
    //
    // Previously the algorithm rotated the gauge volume into the sample frame via R.inv() and
    // rasterised it there. For a non-axis-aligned rotation that inflated the gauge's
    // axis-aligned bounding box and silently admitted voxels that lay outside the actual
    // (rotated) gauge - whenever the sample asymmetrically intersected the inflated bbox the
    // resulting COM was wrong by a factor that depended on the geometry. The fix rasterises
    // the gauge in its own (lab) frame and only transforms candidate voxels into the sample
    // frame to test sample inclusion.
    //
    // Setup: sample cuboid centred at (0, 0.02, 0) with half-widths 0.02 (occupies 0<y<0.04
    // in the workspace's sample frame); gauge cube of full extent 0.02 at origin; goniometer
    // R = Rx(45 deg). In the lab frame the sample's y=0 face becomes the half-plane y+z=0;
    // the gauge cube |y|,|z|<0.01 is cut by y+z>=0 into a right triangle with vertices
    // (0.01, -0.01), (-0.01, 0.01), (0.01, 0.01). Centroid in (y, z) is (0.01/3, 0.01/3); X
    // is unaffected by the rotation so it averages to 0 over the cube's x-extent.
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    IObject_sptr shape_sptr =
        ComponentCreationHelper::createCuboid(0.02, 0.02, 0.02, V3D(0.0, 0.02, 0.0), "asymmSample");
    testWS->mutableSample().setShape(shape_sptr);
    const std::string gaugeXML = " \
        <cuboid id='gv'> \
        <height val='0.02' /> \
        <width val='0.02' /> \
        <depth val='0.02' /> \
        <centre x='0.0' y='0.0' z='0.0' /> \
        </cuboid> \
        <algebra val='gv' /> \
        ";
    testWS->mutableRun().addProperty("GaugeVolume", gaugeXML);
    Mantid::Geometry::Goniometer gonio;
    gonio.pushAxis("phi", 1.0, 0.0, 0.0, 45.0, 1);
    testWS->mutableRun().setGoniometer(gonio, false);

    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass centerOfMass;
    centerOfMass.initialize();
    centerOfMass.setProperty("InputWorkspace", testWS);
    centerOfMass.setProperty("ElementSize", 0.5); // 0.5mm cubes, ~40 slices across the gauge
    TS_ASSERT_THROWS_NOTHING(centerOfMass.execute());
    TS_ASSERT(centerOfMass.isExecuted());
    std::vector<double> resultVec = centerOfMass.getProperty("CentreOfMass");
    V3D result(resultVec[0], resultVec[1], resultVec[2]);
    TS_ASSERT_DELTA(result.X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(result.Y(), 0.01 / 3.0, 0.0002);
    TS_ASSERT_DELTA(result.Z(), 0.01 / 3.0, 0.0002);
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

  //----------------------------------------------------------------------------
  // Neutron weighted centres of mass
  //----------------------------------------------------------------------------
  void testNeutronWeightingRejectsSampleWithoutMaterial() {
    auto testWS = createTwoDetectorWorkspace(0.0); // no material
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("UseNeutronWeightings", true);
    alg.setProperty("ElementSize", 1.0);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void testNeutronWeightingRejectsWorkspaceNotInWavelength() {
    auto testWS = createTwoDetectorWorkspace(FE_NUMBER_DENSITY);
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("UseNeutronWeightings", true);
    alg.setProperty("ElementSize", 1.0);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void testNegligibleAbsorptionReproducesGeometricCentre() {
    // With almost nothing to attenuate in, every element carries the same weight, so the weighted
    // answer must collapse onto the plain geometric centroid.
    const auto unweighted = runAndGetCentre(createTwoDetectorWorkspace(FE_NUMBER_DENSITY), false);
    const auto weighted = runAndGetCentre(createTwoDetectorWorkspace(1e-8), true);
    TS_ASSERT_DELTA(weighted.X(), unweighted.X(), 1e-6);
    TS_ASSERT_DELTA(weighted.Y(), unweighted.Y(), 1e-6);
    TS_ASSERT_DELTA(weighted.Z(), unweighted.Z(), 1e-6);
  }

  void testAbsorptionMovesCentresTowardsTheirOwnDetector() {
    // The sample is a slab elongated along x with the gauge volume at its centre, so scattering
    // points nearer a detector have a shorter exit path and are attenuated less. Each detector
    // should therefore see a centre pulled towards itself. This is the whole point of the
    // per-detector calculation - the outgoing path is the only detector dependent term.
    const auto centres = runAndGetDetectorCentres(createTwoDetectorWorkspace(FE_NUMBER_DENSITY));
    TS_ASSERT_EQUALS(centres.size(), 2);
    // Detector 0 sits at +x, detector 1 at -x.
    TS_ASSERT_LESS_THAN(0.0, centres[0].X());
    TS_ASSERT_LESS_THAN(centres[1].X(), 0.0);
  }

  void testOppositeDetectorsSeeMirroredCentres() {
    // The geometry is symmetric under x -> -x, so the two banks must see mirror image centres.
    const auto centres = runAndGetDetectorCentres(createTwoDetectorWorkspace(FE_NUMBER_DENSITY));
    TS_ASSERT_EQUALS(centres.size(), 2);
    TS_ASSERT_DELTA(centres[0].X(), -centres[1].X(), 1e-9);
    TS_ASSERT_DELTA(centres[0].Y(), centres[1].Y(), 1e-9);
    TS_ASSERT_DELTA(centres[0].Z(), centres[1].Z(), 1e-9);
  }

  void testScalarCentreIsTheIntensityWeightedMeanOfTheTable() {
    auto testWS = createTwoDetectorWorkspace(FE_NUMBER_DENSITY);
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("UseNeutronWeightings", true);
    alg.setProperty("ElementSize", 1.0);
    alg.setPropertyValue("DetectorScatteringCentres", "centres_table");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Mantid::API::ITableWorkspace_sptr table = alg.getProperty("DetectorScatteringCentres");
    TS_ASSERT(table);
    TS_ASSERT_EQUALS(table->rowCount(), 2);
    TS_ASSERT_EQUALS(table->columnCount(), 5);

    V3D expected(0.0, 0.0, 0.0);
    double totalWeight = 0.0;
    for (size_t row = 0; row < table->rowCount(); ++row) {
      const double weight = table->cell<double>(row, 4);
      TS_ASSERT_LESS_THAN(0.0, weight);
      expected += V3D(table->cell<double>(row, 1), table->cell<double>(row, 2), table->cell<double>(row, 3)) * weight;
      totalWeight += weight;
    }
    expected /= totalWeight;

    std::vector<double> scalar = alg.getProperty("CentreOfMass");
    TS_ASSERT_DELTA(scalar[0], expected.X(), 1e-9);
    TS_ASSERT_DELTA(scalar[1], expected.Y(), 1e-9);
    TS_ASSERT_DELTA(scalar[2], expected.Z(), 1e-9);
  }

  void testCollimatorPullsCentresTowardsTheViewingAxis() {
    // A tight collimator only accepts scattering close to the plane through the sample and the
    // detector, so it should suppress the attenuation driven shift along x for a detector at +x.
    const auto without = runAndGetDetectorCentres(createTwoDetectorWorkspace(FE_NUMBER_DENSITY));
    auto collimated = createTwoDetectorWorkspace(FE_NUMBER_DENSITY);
    setCollimatorGaugeWidth(collimated, 0.001);
    const auto with = runAndGetDetectorCentres(collimated);
    TS_ASSERT_EQUALS(with.size(), 2);
    // The collimator restricts z for a detector viewing along x, so the x shift survives; what must
    // change is that the accepted volume is narrower, so the centre moves less far in z.
    TS_ASSERT_LESS_THAN_EQUALS(std::abs(with[0].Z()), std::abs(without[0].Z()) + 1e-12);
  }

  void testPartiallyImmersedGaugeStaysInsideTheSample() {
    // Offset the slab so its face sits on the origin: the sample then spans x in [0, 0.04] while the
    // gauge volume spans [-0.002, 0.002], leaving half the gauge hanging in air. The centre must
    // move off the nominal gauge centre and into the material.
    auto testWS = createTwoDetectorWorkspace(FE_NUMBER_DENSITY, V3D(0.02, 0.0, 0.0));
    const auto centre = runAndGetCentre(testWS, true);
    TS_ASSERT_LESS_THAN(0.0, centre.X());
    TS_ASSERT_LESS_THAN(centre.X(), 0.002);
  }

  void testWithdrawingTheSampleMovesTheCentreTowardsItsSurface() {
    // As the sample is withdrawn from the gauge volume, less and less of the gauge is filled and the
    // centre of the remaining material tracks the surface. This is the near-surface behaviour that
    // makes the weighted centre differ from the nominal measurement position.
    double previous = -1.0;
    for (const double faceOffset : {0.0, 0.001, 0.002}) {
      auto testWS = createTwoDetectorWorkspace(FE_NUMBER_DENSITY, V3D(0.02 + faceOffset, 0.0, 0.0));
      const auto centre = runAndGetCentre(testWS, true);
      TS_ASSERT_LESS_THAN(previous, centre.X());
      previous = centre.X();
    }
  }

private:
  const double FE_NUMBER_DENSITY{0.0849};

  V3D runAndGetCentre(const MatrixWorkspace_sptr &ws, const bool weighted) {
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("UseNeutronWeightings", weighted);
    alg.setProperty("ElementSize", 1.0);
    alg.execute();
    std::vector<double> result = alg.getProperty("CentreOfMass");
    return V3D(result[0], result[1], result[2]);
  }

  std::vector<V3D> runAndGetDetectorCentres(const MatrixWorkspace_sptr &ws) {
    Mantid::Algorithms::EstimateScatteringVolumeCentreOfMass alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("UseNeutronWeightings", true);
    alg.setProperty("ElementSize", 1.0);
    alg.setPropertyValue("DetectorScatteringCentres", "det_centres");
    alg.execute();
    Mantid::API::ITableWorkspace_sptr table = alg.getProperty("DetectorScatteringCentres");
    std::vector<V3D> centres;
    for (size_t row = 0; row < table->rowCount(); ++row) {
      centres.emplace_back(table->cell<double>(row, 1), table->cell<double>(row, 2), table->cell<double>(row, 3));
    }
    return centres;
  }

  void setCollimatorGaugeWidth(const MatrixWorkspace_sptr &ws, const double width) {
    auto &pmap = ws->instrumentParameters();
    pmap.addDouble(ws->getInstrument()->getComponentID(), "col-gauge-width", width);
  }

  /// A slab elongated along x, viewed by two detectors at +/-x, with a 4mm cubic gauge volume at the
  /// origin. The elongation makes the outgoing path length - and so the attenuation - vary strongly
  /// with position along x, which is what distinguishes the two detectors.
  MatrixWorkspace_sptr createTwoDetectorWorkspace(const double numberDensity,
                                                  const V3D &sampleCentre = V3D(0.0, 0.0, 0.0)) {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(2, 5, 1.0, 0.4);
    auto inst =
        ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions({1.5, 1.5}, {M_PI_2, M_PI_2}, {0.0, M_PI});
    inst->setName("test-inst");
    ws->setInstrument(inst);
    ws->rebuildSpectraMapping();
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    auto shape = ComponentCreationHelper::createCuboid(0.02, 0.002, 0.002, sampleCentre, "slab");
    if (numberDensity > 0.0) {
      shape->setMaterial(
          Mantid::Kernel::Material("Fe", Mantid::PhysicalConstants::getNeutronAtom(26, 0), numberDensity));
    }
    ws->mutableSample().setShape(shape);

    const std::string gaugeXML = " \
        <cuboid id='some-cuboid'> \
        <height val='0.004'  /> \
        <width val='0.004' />  \
        <depth  val='0.004' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='some-cuboid' /> \
        ";
    ws->mutableRun().addProperty("GaugeVolume", gaugeXML);
    return ws;
  }

  MatrixWorkspace_sptr createTestWorkspace() {
    // Create a basic test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    return testWS;
  }

  MatrixWorkspace_sptr createWorkspaceWithAnyOffsetCylinderSample(V3D offset) {

    // Create the workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    IObject_sptr shape_sptr =
        ComponentCreationHelper::createCappedCylinder(0.01, 0.04, offset, V3D(0.0, 1.0, 0.0), "cyl");
    testWS->mutableSample().setShape(shape_sptr);
    return testWS;
  }

  MatrixWorkspace_sptr createWorkspaceWithCylinderSample() {
    V3D center(0.0, -0.02, 0.0); // cylinder of height 4cm is centred with base at (0,-2,0)
    return createWorkspaceWithAnyOffsetCylinderSample(center);
  }
  MatrixWorkspace_sptr createWorkspaceWithOffsetCylinderSample() {
    V3D center(0.0, -0.03, 0.0); // this is offset 1cm lower
    return createWorkspaceWithAnyOffsetCylinderSample(center);
  }
  MatrixWorkspace_sptr createWorkspaceWithOffsetCubeSample() {
    // Create a workspace with an offset cylinder sample
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    IObject_sptr shape_sptr =
        ComponentCreationHelper::createCuboid(0.01, 0.01, 0.01, V3D(0.01, 0.01, 0.01), "testCube");
    testWS->mutableSample().setShape(shape_sptr);
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
    MatrixWorkspace_sptr testWS = createWorkspaceWithAnyOffsetCylinderSample(V3D(10.0, 10.0, 10.0));
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
