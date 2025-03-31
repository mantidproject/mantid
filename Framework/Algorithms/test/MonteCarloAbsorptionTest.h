// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/IMCInteractionVolume.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WarningSuppressions.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

namespace {
enum class Environment {
  CubeRotatedSampleOnly,
  CubeSampleOnly,
  CubeSamplePlusContainer,
  CylinderSampleOnly,
  CylinderSamplePlusContainer,
  MeshSamplePlusContainer,
  CubeSamplePlusCubeGV,
};

struct TestWorkspaceDescriptor {
  int nspectra;
  int nbins;
  bool fullInstrument;
  Environment sampleEnviron;
  unsigned int emode;
  double efixed;
};

void addSample(const Mantid::API::MatrixWorkspace_sptr &ws, const Environment environment) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  using namespace Mantid::DataHandling;
  namespace PhysicalConstants = Mantid::PhysicalConstants;

  if (environment == Environment::MeshSamplePlusContainer) {
    std::string samplePath = Mantid::API::FileFinder::Instance().getFullPath("PearlSample.stl");
    ScaleUnits scaleType = ScaleUnits::millimetres;
    ReadMaterial::MaterialParameters params;
    params.chemicalSymbol = "V";
    auto binaryStlReader = LoadBinaryStl(samplePath, scaleType, params);
    std::shared_ptr<MeshObject> shape = binaryStlReader.readShape();
    ws->mutableSample().setShape(shape);

    std::string envPath = Mantid::API::FileFinder::Instance().getFullPath("PearlEnvironment.stl");
    // set up a uniform material for whole environment here to give simple case
    params.chemicalSymbol = "Ti-Zr";
    params.massDensity = 5.23;
    auto binaryStlReaderEnv = LoadBinaryStl(envPath, scaleType, params);
    std::shared_ptr<MeshObject> environmentShape = binaryStlReaderEnv.readShape();

    auto can = std::make_shared<Container>(environmentShape);
    std::unique_ptr<SampleEnvironment> environment = std::make_unique<SampleEnvironment>("PearlEnvironment", can);

    ws->mutableSample().setEnvironment(std::move(environment));
  } else {
    if (environment == Environment::CylinderSampleOnly || environment == Environment::CylinderSamplePlusContainer) {
      // Define a sample shape
      constexpr double sampleRadius{0.006};
      constexpr double sampleHeight{0.04};
      const V3D sampleBaseCentre{0., -sampleHeight / 2., 0.};
      const V3D yAxis{0., 1., 0.};
      auto sampleShape = ComponentCreationHelper::createCappedCylinder(sampleRadius, sampleHeight, sampleBaseCentre,
                                                                       yAxis, "sample-cylinder");
      // And a material assuming it's a CSG Object
      sampleShape->setMaterial(Material("Vanadium", PhysicalConstants::getNeutronAtom(23, 0), 0.072));
      ws->mutableSample().setShape(sampleShape);
      if (environment == Environment::CylinderSamplePlusContainer) {
        constexpr double containerWallThickness{0.002};
        constexpr double containerInnerRadius{1.2 * sampleHeight};
        constexpr double containerOuterRadius{containerInnerRadius + containerWallThickness};

        auto canShape = ComponentCreationHelper::createHollowShell(containerInnerRadius, containerOuterRadius);
        // Set material assuming it's a CSG Object
        canShape->setMaterial(Material("CanMaterial", PhysicalConstants::getNeutronAtom(26, 0), 0.01));
        auto can = std::make_shared<Container>(canShape);
        ws->mutableSample().setEnvironment(std::make_unique<SampleEnvironment>("can", can));
      }
    } else if (environment == Environment::CubeRotatedSampleOnly) {
      // create cube that has been rotated about x axis so that the
      // depth the neutron tracks pass through varies linearly with y
      auto cubeShape =
          ComponentCreationHelper::createCuboid(sqrt(2) / 200, sqrt(2) / 200, sqrt(2) / 200, 45, V3D{1, 0, 0});

      // create test material with mu=1 for lambda=0.5, mu=2 for lambda=1.5
      auto shape = std::shared_ptr<IObject>(cubeShape->cloneWithMaterial(
          Mantid::Kernel::Material("Test",
                                   Mantid::PhysicalConstants::NeutronAtom(
                                       0, 0, 0, 0, 0, 0.5 /*total scattering xs*/,
                                       Mantid::PhysicalConstants::NeutronAtom::ReferenceLambda /*absorption xs*/),
                                   1 /*number density*/)));
      ws->mutableSample().setShape(shape);
    } else if (environment == Environment::CubeSampleOnly || environment == Environment::CubeSamplePlusContainer ||
               environment == Environment::CubeSamplePlusCubeGV) {
      // create 1cm x 1cm cube
      auto cubeShape = ComponentCreationHelper::createCuboid(0.005, 0.005, 0.005, {0., 0., -0.005});

      // create test material with mu=1 for lambda=0.5, mu=2 for lambda=1.5
      auto shape = std::shared_ptr<IObject>(cubeShape->cloneWithMaterial(
          Mantid::Kernel::Material("Test",
                                   Mantid::PhysicalConstants::NeutronAtom(
                                       0, 0, 0, 0, 0, 0.5 /*total scattering xs*/,
                                       Mantid::PhysicalConstants::NeutronAtom::ReferenceLambda /*absorption xs*/),
                                   1 /*number density*/)));
      ws->mutableSample().setShape(shape);
      if (environment == Environment::CubeSamplePlusContainer) {
        auto xmlShapeStreamFront = ComponentCreationHelper::cuboidXML(0.005, 0.005, 0.0025, {0., 0., -0.0125}, "front");
        auto xmlShapeStreamBack = ComponentCreationHelper::cuboidXML(0.005, 0.005, 0.0025, {0., 0., 0.0025}, "back");
        std::string combinedXML = xmlShapeStreamFront + xmlShapeStreamBack + "<algebra val=\"back:front\"/>";
        ShapeFactory shapeMaker;
        auto holderShape = shapeMaker.createShape(combinedXML);
        auto shape = std::shared_ptr<IObject>(holderShape->cloneWithMaterial(
            Mantid::Kernel::Material("Test",
                                     Mantid::PhysicalConstants::NeutronAtom(
                                         0, 0, 0, 0, 0, 0.5 /*total scattering xs*/,
                                         Mantid::PhysicalConstants::NeutronAtom::ReferenceLambda /*absorption xs*/),
                                     1 /*number density*/)));
        auto can = std::make_shared<Container>(shape);
        ws->mutableSample().setEnvironment(std::make_unique<SampleEnvironment>("can", can));
      }
      if (environment == Environment::CubeSamplePlusCubeGV) {
        // create 1mm x 1mm x 1mm cube for the gauge volume
        auto gaugeVolumeXML = ComponentCreationHelper::cuboidXML(0.0005, 0.0005, 0.0005, {0., 0., -0.0005}, "gv");
        ws->mutableRun().addProperty<std::string>("GaugeVolume", gaugeVolumeXML);
      }
    }
  }
}

Mantid::API::MatrixWorkspace_sptr setUpWS(const TestWorkspaceDescriptor &wsProps) {
  using namespace Mantid::Kernel;
  using namespace Mantid::Geometry;

  Mantid::DataObjects::Workspace2D_sptr space;
  if (wsProps.fullInstrument) {
    space = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(wsProps.nspectra, wsProps.nbins);
  } else {
    // create a test instrument with a single detector on the beam line so
    // that a test case with a simple path length calculation can be created
    const auto instrName = "error_test";
    auto testInst = std::make_shared<Instrument>(instrName);

    const double cylRadius(0.008 / 2);
    const double cylHeight(0.0002);
    // One object
    auto pixelShape = ComponentCreationHelper::createCappedCylinder(
        cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.), "pixel-shape");

    // source and detector are at +/-100m so tracks approx parallel to beam
    constexpr double distance = 100.0;
    Detector *det = new Detector("det", 1, pixelShape, nullptr);
    det->setPos(0, 0, distance);
    testInst->add(det);
    testInst->markAsDetector(det);

    ComponentCreationHelper::addSourceToInstrument(testInst, V3D(0.0, 0.0, -distance));
    ComponentCreationHelper::addSampleToInstrument(testInst, V3D(0.0, 0.0, 0.0));

    space = WorkspaceCreationHelper::create2DWorkspaceBinned(wsProps.nspectra, wsProps.nbins);

    space->getSpectrum(0).setDetectorID(det->getID());
    space->setInstrument(testInst);
  }
  // Needs to have units of wavelength
  space->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  auto inst = space->getInstrument();
  auto &pmap = space->instrumentParameters();

  if (wsProps.emode == DeltaEMode::Direct) {
    pmap.addString(inst.get(), "deltaE-mode", "Direct");
    const double efixed(wsProps.efixed);
    space->mutableRun().addProperty<double>("Ei", efixed);
  } else if (wsProps.emode == DeltaEMode::Indirect) {
    const double efixed(wsProps.efixed);
    pmap.addString(inst.get(), "deltaE-mode", "Indirect");
    pmap.addDouble(inst.get(), "Efixed", efixed);
  }

  addSample(space, wsProps.sampleEnviron);
  return space;
}
} // namespace

class MonteCarloAbsorptionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MonteCarloAbsorptionTest *createSuite() { return new MonteCarloAbsorptionTest(); }
  static void destroySuite(MonteCarloAbsorptionTest *suite) { delete suite; }

  //---------------------------------------------------------------------------
  // Integration tests - Success cases
  //---------------------------------------------------------------------------
  void test_Workspace_With_Just_Sample_For_Elastic() {
    using namespace Mantid::Geometry;

    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 2, false, Environment::CubeRotatedSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);

    // calculate expected value of the att factor as integral exp(-mu*t)p(t)dt
    // where t = 2(1-y), p(t) = 2(1-y)
    // integrate over y= 0 to 1, which gives
    // E(att) = (1 - exp(-2*mu)*(2*mu+1))/2*mu^2
    // E(att^2) = (1 - exp(-4*mu)*(4*mu+1))/8*mu^2

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 500000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    verifyDimensions(wsProps, outputWS);
    auto yData = outputWS->getSpectrum(0).dataY();
    auto eData = outputWS->getSpectrum(0).dataE();
    // recreate the sd of the set of simulated paths from the sd on the mean
    double attenuationFactorsSD1 = eData[0] * sqrt(NEVENTS);

    constexpr double delta(1e-03);
    const double calculatedAttFactor1 = (1 - 3 * exp(-2)) / 2;
    const double calculatedAttFactorSq1 = (1 - 5 * exp(-4)) / 8;
    TS_ASSERT_DELTA(calculatedAttFactor1, yData[0], delta);
    const double calculatedAttFactorSD1 = sqrt(calculatedAttFactorSq1 - pow(calculatedAttFactor1, 2));
    TS_ASSERT_DELTA(calculatedAttFactorSD1, attenuationFactorsSD1, delta);

    double attenuationFactorsSD2 = eData[1] * sqrt(NEVENTS);
    const double calculatedAttFactor2 = (1 - 5 * exp(-4)) / 8;
    const double calculatedAttFactorSq2 = (1 - 9 * exp(-8)) / 32;
    TS_ASSERT_DELTA(calculatedAttFactor2, yData[1], delta);
    const double calculatedAttFactorSD2 = sqrt(calculatedAttFactorSq2 - pow(calculatedAttFactor2, 2));
    TS_ASSERT_DELTA(calculatedAttFactorSD2, attenuationFactorsSD2, delta);
  }

  void test_Workspace_With_Just_Sample_For_Direct() {
    using namespace Mantid::Geometry;
    namespace PhysicalConstants = Mantid::PhysicalConstants;

    using Mantid::Kernel::DeltaEMode;
    double lambdaFixed = 0.5;
    double EFixedForLambda =
        1e20 * pow(Mantid::PhysicalConstants::h, 2) /
        (2.0 * Mantid::PhysicalConstants::NeutronMass * Mantid::PhysicalConstants::meV * pow(lambdaFixed, 2));
    TestWorkspaceDescriptor wsProps = {
        1, 2, false, Environment::CubeRotatedSampleOnly, DeltaEMode::Direct, EFixedForLambda};
    auto testWS = setUpWS(wsProps);

    // calculate expected value of the att factor as integral att(t)p(t)dt
    // where att(t) = exp(-mu1*t)-exp(-mu2*t)/(t*(mu2-mu1))
    // where t = 2(1-y), p(t) = 2(1-y)
    // integrate over y= 0 to 1, which gives
    // E(att) = (1 - exp(-2*mu1))*mu2 - (1 -
    // exp(-2*mu2))*mu1)/(2*mu1*mu2*(mu2-mu1))

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 500000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    verifyDimensions(wsProps, outputWS);
    auto yData = outputWS->getSpectrum(0).dataY();
    auto eData = outputWS->getSpectrum(0).dataE();

    constexpr double delta(1e-03);
    const double calculatedAttFactor = (1 - 2 * exp(-2) + exp(-4)) / 4;
    TS_ASSERT_DELTA(calculatedAttFactor, yData[1], delta);
  }

  void test_Workspace_With_Just_Sample_For_Indirect() {
    using namespace Mantid::Geometry;
    namespace PhysicalConstants = Mantid::PhysicalConstants;

    using Mantid::Kernel::DeltaEMode;
    double lambdaFixed = 0.5;
    double EFixedForLambda =
        1e20 * pow(Mantid::PhysicalConstants::h, 2) /
        (2.0 * Mantid::PhysicalConstants::NeutronMass * Mantid::PhysicalConstants::meV * pow(lambdaFixed, 2));
    TestWorkspaceDescriptor wsProps = {
        1, 2, false, Environment::CubeRotatedSampleOnly, DeltaEMode::Indirect, EFixedForLambda};
    auto testWS = setUpWS(wsProps);

    // calculate expected value of the att factor as integral att(t)p(t)dt
    // where att(t) = exp(-mu1*t)-exp(-mu2*t)/(t*(mu2-mu1))
    // where t = 2(1-y), p(t) = 2(1-y)
    // integrate over y= 0 to 1, which gives
    // E(att) = (1 - exp(-2*mu1))*mu2 - (1 -
    // exp(-2*mu2))*mu1)/(2*mu1*mu2*(mu2-mu1))

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 500000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    verifyDimensions(wsProps, outputWS);
    auto yData = outputWS->getSpectrum(0).dataY();
    auto eData = outputWS->getSpectrum(0).dataE();

    constexpr double delta(1e-03);
    const double calculatedAttFactor = (1 - 2 * exp(-2) + exp(-4)) / 4;
    TS_ASSERT_DELTA(calculatedAttFactor, yData[1], delta);
  }

  void test_Workspace_With_Sample_And_Container() {
    using namespace Mantid::Geometry;

    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 2, false, Environment::CubeSamplePlusContainer, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 1000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    verifyDimensions(wsProps, outputWS);
    auto yData = outputWS->getSpectrum(0).dataY();

    constexpr double delta(1e-03);
    const double calculatedAttFactor = exp(-2);
    TS_ASSERT_DELTA(calculatedAttFactor, yData[0], delta);
  }

  void test_Workspace_With_Sample_And_Gauge_Volume() {
    using namespace Mantid::Geometry;

    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 2, false, Environment::CubeSamplePlusCubeGV, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 1000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    verifyDimensions(wsProps, outputWS);

    // for now, just checking it runs
  }

  void test_Workspace_Slit_Beam_Size_Set() {
    using namespace Mantid::Geometry;

    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 2, false, Environment::CubeRotatedSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);
    auto inst = testWS->getInstrument();
    auto &pmap = testWS->instrumentParameters();
    auto source = inst->getSource();
    pmap.addString(source->getComponentID(), "beam-shape", "Slit");
    pmap.addDouble(source->getComponentID(), "beam-width", 0.01);
    pmap.addDouble(source->getComponentID(), "beam-height", 0.01);

    // calculate expected value of the att factor as integral exp(-mu*t)p(t)dt
    // where t = 2(1-y), p(t) = 8(1-y)/3
    // integrate over y= 0 to 0.5, which gives
    // E(att) = 2*(exp(-mu)*(mu+1) - exp(-2*mu)*(2*mu+1))/3*mu^2

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 500000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    verifyDimensions(wsProps, outputWS);
    auto yData = outputWS->getSpectrum(0).dataY();

    constexpr double delta(1e-03);
    const double calculatedAttFactor1 = 2 * (2 * exp(-1) - 3 * exp(-2)) / 3;
    TS_ASSERT_DELTA(calculatedAttFactor1, yData[0], delta);
  }

  void test_Workspace_Circle_Beam_Size_Set() {
    using namespace Mantid::Geometry;

    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 2, false, Environment::CubeRotatedSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);
    auto inst = testWS->getInstrument();
    auto &pmap = testWS->instrumentParameters();
    auto source = inst->getSource();
    pmap.addString(source->getComponentID(), "beam-shape", "Circle");
    pmap.addDouble(source->getComponentID(), "beam-radius", 0.01);

    auto mcAbsorb = createAlgorithm();
    constexpr int NEVENTS = 500000;
    mcAbsorb->setProperty("EventsPerPoint", NEVENTS);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
  }
  //---------------------------------------------------------------------------
  // Unit tests
  //---------------------------------------------------------------------------

  void test_Lambda_StepSize_One() {
    using Mantid::Kernel::DeltaEMode;
    const int nspectra = 5;
    const int nbins = 10;
    TestWorkspaceDescriptor wsProps = {nspectra, nbins, true, Environment::CylinderSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);
    auto mcAbsorb = createTestAlgorithm();
    auto MCAbsorptionStrategy = std::make_shared<MockMCAbsorptionStrategy>();
    mcAbsorb->setAbsorptionStrategy(MCAbsorptionStrategy);

    using namespace ::testing;
    std::vector<double> attenuationFactorZeroes(nbins);
    std::vector<double> attenuationFactorErrorZeroes(nbins);
    std::vector<double> dummyAttFactor = {10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0};
    std::vector<double> dummyAttFactorErr = {1.0, 1.1, 1.0, 1.1, 0.9, 1.2, 1.0, 1.1, 1.1, 1.0};
    std::vector<double> wavelengths = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5};
    EXPECT_CALL(*MCAbsorptionStrategy,
                calculate(_, _, wavelengths, _, attenuationFactorZeroes, attenuationFactorErrorZeroes, _))
        .Times(Exactly(static_cast<int>(nspectra)))
        .WillRepeatedly(DoAll(SetArgReferee<4>(dummyAttFactor), SetArgReferee<5>(dummyAttFactorErr)));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nspectra);
    TS_ASSERT_EQUALS(10.0, outputWS->y(0).front());
    TS_ASSERT_EQUALS(1.0, outputWS->e(0).front());
  }

  void test_Lambda_StepSize_Two_Linear_Interpolation() {
    using Mantid::Kernel::DeltaEMode;
    const int nspectra = 5;
    const int nbins = 10;
    TestWorkspaceDescriptor wsProps = {nspectra, nbins, true, Environment::CylinderSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);
    auto mcAbsorb = createTestAlgorithm();
    auto MCAbsorptionStrategy = std::make_shared<MockMCAbsorptionStrategy>();
    mcAbsorb->setAbsorptionStrategy(MCAbsorptionStrategy);

    mcAbsorb->setProperty("ResimulateTracksForDifferentWavelengths", true);
    mcAbsorb->setProperty("NumberOfWavelengthPoints", nbins / 2);

    using namespace ::testing;
    const int nlambdabins = nbins / 2 + 1;
    std::vector<double> attenuationFactorZeroes(nlambdabins);
    std::vector<double> attenuationFactorErrorZeroes(nlambdabins);
    std::vector<double> dummyAttFactor = {10.0, 8.0, 6.0, 4.0, 2.0, 0.0};
    std::vector<double> dummyAttFactorErr = {1.0, 1.0, 1.1, 1.2, 1.1, 1.0};
    std::vector<double> wavelengths = {0.5, 2.5, 4.5, 6.5, 8.5, 9.5};
    EXPECT_CALL(*MCAbsorptionStrategy,
                calculate(_, _, wavelengths, _, attenuationFactorZeroes, attenuationFactorErrorZeroes, _))
        .Times(Exactly(static_cast<int>(nspectra)))
        .WillRepeatedly(DoAll(SetArgReferee<4>(dummyAttFactor), SetArgReferee<5>(dummyAttFactorErr)));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nspectra);
    TS_ASSERT_EQUALS(dummyAttFactor[0], outputWS->y(0).front());
    TS_ASSERT_EQUALS(dummyAttFactorErr[0], outputWS->e(0).front());
    TS_ASSERT_EQUALS((dummyAttFactor[0] + dummyAttFactor[1]) / 2, outputWS->y(0)[1]);
    TS_ASSERT_EQUALS(sqrt(pow(dummyAttFactorErr[0], 2) + pow(dummyAttFactorErr[1], 2)) / 2.0, outputWS->e(0)[1]);
    TS_ASSERT_EQUALS((dummyAttFactor[1] + dummyAttFactor[2]) / 2, outputWS->y(0)[3]);
  }

  void test_Lambda_StepSize_Two_Spline_Interpolation() {
    using Mantid::Kernel::DeltaEMode;
    const int nspectra = 5;
    const int nbins = 9;
    TestWorkspaceDescriptor wsProps = {nspectra, nbins, true, Environment::CylinderSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);
    auto mcAbsorb = createTestAlgorithm();
    auto MCAbsorptionStrategy = std::make_shared<MockMCAbsorptionStrategy>();
    mcAbsorb->setAbsorptionStrategy(MCAbsorptionStrategy);

    mcAbsorb->setProperty("ResimulateTracksForDifferentWavelengths", true);
    mcAbsorb->setProperty("NumberOfWavelengthPoints", nbins / 2);
    mcAbsorb->setProperty("Interpolation", "CSpline");

    using namespace ::testing;
    const int nlambdabins = nbins / 2 + 1;
    // create a set of points where interpolated values at half way points
    // are easy to calculate
    // The following are based on a cubic spline of form:
    // q(x) = (1-t(x))y1+t(x)y2+t(x)(1-t(x))((1-t(x))a+t(x)b)
    // where:
    // t(x) = (x-x1)/(x2-x1)
    // a = q'(x1)(x2-x1)-(y2-y1)
    // b = -q'(x2)(x2-x1)+(y2-y1)
    std::vector<double> attenuationFactorZeroes(nlambdabins);
    std::vector<double> attenuationFactorErrorZeroes(nlambdabins);
    std::vector<double> dummyAttFactor = {24.0, 13.0, 6.0, 1.0, 0.0};
    std::vector<double> dummyAttFactorErr = {1.0, 1.0, 1.1, 1.2, 1.1};
    std::vector<double> wavelengths = {0.5, 2.5, 4.5, 6.5, 8.5};
    EXPECT_CALL(*MCAbsorptionStrategy,
                calculate(_, _, wavelengths, _, attenuationFactorZeroes, attenuationFactorErrorZeroes, _))
        .Times(Exactly(static_cast<int>(nspectra)))
        .WillRepeatedly(DoAll(SetArgReferee<4>(dummyAttFactor), SetArgReferee<5>(dummyAttFactorErr)));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);

    const std::vector<double> qdash = {-12.0, -9.0, -6.0, -3.0, 0.0};
    const double tx = 0.5;
    std::vector<double> a, b, interp;
    for (size_t i = 0; i < nlambdabins - 1; i++) {
      a.emplace_back(qdash[i] - (dummyAttFactor[i + 1] - dummyAttFactor[i]));
      b.emplace_back(-qdash[i + 1] + (dummyAttFactor[i + 1] - dummyAttFactor[i]));
      interp.emplace_back(tx * (dummyAttFactor[i] + dummyAttFactor[i + 1]) + pow(tx, 3) * (a[i] + b[i]));
    }

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nspectra);
    TS_ASSERT_EQUALS(dummyAttFactor[0], outputWS->y(0).front());
    TS_ASSERT_EQUALS(dummyAttFactorErr[0], outputWS->e(0).front());
    TS_ASSERT_EQUALS(interp[0], outputWS->y(0)[1]);
    TS_ASSERT_EQUALS(interp[1], outputWS->y(0)[3]);
  }

  void test_Workspace_With_Different_Lambda_Ranges() {
    using namespace Mantid::API;

    // create an instrument including some monitors so that there's a good
    // variation in the wavelength range of the spectra when convert from TOF to
    // wavelength
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 100, true);
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setChild(true);
    convert.setProperty("InputWorkspace", testWS);
    convert.setProperty("Target", "Wavelength");
    convert.setProperty("OutputWorkspace", "dummy");
    convert.execute();
    testWS = convert.getProperty("OutputWorkspace");

    auto mcAbsorb = createAlgorithm();
    addSample(testWS, Environment::CylinderSampleOnly);
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
  }

  void test_ignore_masked_spectra() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {5, 10, true, Environment::CylinderSampleOnly, DeltaEMode::Elastic, -1};
    auto testWS = setUpWS(wsProps);
    testWS->mutableSpectrumInfo().setMasked(0, true);
    auto mcAbsorb = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    auto outputWS = getOutputWorkspace(mcAbsorb);
    // should still output a spectra but it should also be masked and equal to
    // zero
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(outputWS->spectrumInfo().isMasked(0), true);
    auto yData = outputWS->getSpectrum(0).dataY();
    bool allZero = std::all_of(yData.begin(), yData.end(), [](double i) { return i == 0; });
    TS_ASSERT_EQUALS(allZero, true);
  }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------
  void test_Workspace_With_No_Instrument_Is_Not_Accepted() {
    using namespace Mantid::API;

    auto mcAbsorb = createAlgorithm();
    // Create a simple test workspace that has no instrument
    auto testWS = WorkspaceCreationHelper::create2DWorkspace(1, 1);

    TS_ASSERT_THROWS(mcAbsorb->setProperty("InputWorkspace", testWS), const std::invalid_argument &);
  }

  void test_Workspace_With_An_Invalid_Sample_Shape_Is_Not_Accepted() {
    using namespace Mantid::API;

    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 1);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS(mcabs->execute(), const std::invalid_argument &);
  }

  void test_Lower_Limit_for_Number_of_Wavelengths() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, true, Environment::CylinderSampleOnly, DeltaEMode::Direct, 12.0};
    int nlambda{1};
    TS_ASSERT_THROWS(runAlgorithm(wsProps, true, nlambda, "Linear"), const std::runtime_error &)
    nlambda = 2;
    TS_ASSERT_THROWS(runAlgorithm(wsProps, true, nlambda, "CSpline"), const std::runtime_error &)
  }

  void test_event_workspace() {
    auto inputWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(5, 2, true);
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    addSample(inputWS, Environment::CylinderSampleOnly);

    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
    TS_ASSERT(mcabs->execute());
    // only checking that it can successfully execute
  }

  Mantid::API::MatrixWorkspace_const_sptr do_test_Sparse_Workspace(Mantid::API::MatrixWorkspace_sptr modelWS,
                                                                   int nExpectedInterpolationCalls) {
    auto mcAbsorb = createTestAlgorithm();
    auto MCAbsorptionStrategy = std::make_shared<MockMCAbsorptionStrategy>();
    mcAbsorb->setAbsorptionStrategy(MCAbsorptionStrategy);
    auto nbins = modelWS->blocksize();
    auto sparseWS = std::make_shared<MockSparseWorkspace>(*modelWS, nbins, 3, 3);
    mcAbsorb->setSparseWorkspace(sparseWS);
    using namespace ::testing;
    EXPECT_CALL(*MCAbsorptionStrategy, calculate(_, _, _, _, _, _, _)).Times(Exactly(static_cast<int>(9)));
    Mantid::HistogramData::Frequencies ysOnes(nbins, 1.0);
    Mantid::HistogramData::Points ps = modelWS->getSpectrum(0).histogram().points();
    Mantid::HistogramData::FrequencyStandardDeviations errs(nbins, 0.5);
    const Mantid::HistogramData::Histogram testHistogramOnes(ps, ysOnes, errs);
    EXPECT_CALL(*sparseWS, bilinearInterpolateFromDetectorGrid(_, _))
        .Times(Exactly(static_cast<int>(nExpectedInterpolationCalls)))
        .WillRepeatedly(Return(testHistogramOnes));

    mcAbsorb->setProperty("SparseInstrument", true);
    mcAbsorb->setProperty("NumberOfDetectorRows", 3);
    mcAbsorb->setProperty("NumberOfDetectorColumns", 3);

    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty("InputWorkspace", modelWS));
    mcAbsorb->execute();
    return getOutputWorkspace(mcAbsorb);
  }

  void test_Sparse_Workspace() {
    using Mantid::Kernel::DeltaEMode;
    constexpr int nspectra = 25;
    constexpr int nbins = 10;
    TestWorkspaceDescriptor wsProps = {nspectra, nbins, true, Environment::CylinderSampleOnly, DeltaEMode::Elastic, -1};
    auto modelWS = setUpWS(wsProps);
    auto outputWS = do_test_Sparse_Workspace(modelWS, nspectra);
    verifyDimensions(wsProps, outputWS);
    TS_ASSERT_EQUALS(1.0, outputWS->y(0)[0]);
    TS_ASSERT_EQUALS(1.0, outputWS->y(0)[1]);
    TS_ASSERT_EQUALS(1.0, outputWS->y(1)[0]);
    TS_ASSERT_EQUALS(0.5, outputWS->e(0)[0]);
  }

  void test_Sparse_Workspace_Spectrum_Without_Detector() {
    using Mantid::Kernel::DeltaEMode;
    constexpr int nspectra = 25;
    constexpr int nbins = 10;
    TestWorkspaceDescriptor wsProps = {nspectra, nbins, true, Environment::CylinderSampleOnly, DeltaEMode::Elastic, -1};
    auto modelWS = setUpWS(wsProps);
    modelWS->getSpectrum(3).clearDetectorIDs();
    auto outputWS = do_test_Sparse_Workspace(modelWS, nspectra - 1);
    verifyDimensions(wsProps, outputWS);
    TS_ASSERT_EQUALS(1.0, outputWS->y(0)[0]);
    TS_ASSERT_EQUALS(1.0, outputWS->y(0)[1]);
    TS_ASSERT_EQUALS(1.0, outputWS->y(1)[0]);
    TS_ASSERT_EQUALS(0.5, outputWS->e(0)[0]);
  }

private:
  class MockMCAbsorptionStrategy final : public Mantid::Algorithms::IMCAbsorptionStrategy {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_METHOD7(calculate, void(Mantid::Kernel::PseudoRandomNumberGenerator &rng, const Mantid::Kernel::V3D &finalPos,
                                 const std::vector<double> &lambdas, const double lambdaFixed,
                                 std::vector<double> &attenuationFactors, std::vector<double> &attFactorErrors,
                                 Mantid::Algorithms::MCInteractionStatistics &stats));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
  class MockSparseWorkspace final : public Mantid::Algorithms::SparseWorkspace {
  public:
    MockSparseWorkspace(const Mantid::API::MatrixWorkspace &modelWS, const size_t wavelengthPoints, const size_t rows,
                        const size_t columns)
        : SparseWorkspace(modelWS, wavelengthPoints, rows, columns) {};

  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD2(interpolateFromDetectorGrid,
                       Mantid::HistogramData::Histogram(const double lat, const double lon));
    MOCK_CONST_METHOD2(bilinearInterpolateFromDetectorGrid,
                       Mantid::HistogramData::Histogram(const double lat, const double lon));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
  class TestMonteCarloAbsorption final : public Mantid::Algorithms::MonteCarloAbsorption {
  public:
    void setAbsorptionStrategy(std::shared_ptr<MockMCAbsorptionStrategy> absStrategy) {
      m_MCAbsorptionStrategy = absStrategy;
    }
    void setSparseWorkspace(std::shared_ptr<MockSparseWorkspace> sparseWS) { m_SparseWorkspace = sparseWS; }
    using MonteCarloAbsorption::createBeamProfile;

  protected:
    std::shared_ptr<Mantid::Algorithms::IMCAbsorptionStrategy>
    createStrategy(Mantid::Algorithms::IMCInteractionVolume &interactionVol,
                   const Mantid::Algorithms::IBeamProfile &beamProfile, Mantid::Kernel::DeltaEMode::Type EMode,
                   const size_t nevents, const size_t maxScatterPtAttempts,
                   const bool regenerateTracksForEachLambda) override {
      UNUSED_ARG(interactionVol);
      UNUSED_ARG(beamProfile);
      UNUSED_ARG(EMode);
      UNUSED_ARG(nevents);
      UNUSED_ARG(maxScatterPtAttempts);
      UNUSED_ARG(regenerateTracksForEachLambda);
      return m_MCAbsorptionStrategy;
    }
    std::shared_ptr<Mantid::Algorithms::SparseWorkspace>
    createSparseWorkspace(const Mantid::API::MatrixWorkspace &modelWS, const size_t wavelengthPoints, const size_t rows,
                          const size_t columns) override {
      UNUSED_ARG(modelWS);
      UNUSED_ARG(wavelengthPoints);
      UNUSED_ARG(rows);
      UNUSED_ARG(columns);
      return m_SparseWorkspace;
    }

  private:
    std::shared_ptr<MockMCAbsorptionStrategy> m_MCAbsorptionStrategy;
    std::shared_ptr<MockSparseWorkspace> m_SparseWorkspace;
  };
  Mantid::API::MatrixWorkspace_const_sptr runAlgorithm(const TestWorkspaceDescriptor &wsProps,
                                                       const bool resimulateTracksForDiffWavelengths = false,
                                                       int nlambda = -1, const std::string &interpolate = "",
                                                       const bool sparseInstrument = false, const int sparseRows = 2,
                                                       const int sparseColumns = 2) {
    auto inputWS = setUpWS(wsProps);
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
    if (resimulateTracksForDiffWavelengths) {
      TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("ResimulateTracksForDifferentWavelengths", true));
      if (nlambda > 0) {
        TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("NumberOfWavelengthPoints", nlambda));
      }
    }
    if (!interpolate.empty()) {
      TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("Interpolation", interpolate));
    }
    if (sparseInstrument) {
      mcabs->setProperty("SparseInstrument", true);
      mcabs->setProperty("NumberOfDetectorRows", sparseRows);
      mcabs->setProperty("NumberOfDetectorColumns", sparseColumns);
    }
    mcabs->execute();
    return getOutputWorkspace(mcabs);
  }

  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    using Mantid::Algorithms::MonteCarloAbsorption;
    using Mantid::API::IAlgorithm;
    auto alg = std::make_shared<MonteCarloAbsorption>();
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->setProperty("EventsPerPoint", 300);
    alg->setPropertyValue("OutputWorkspace", "__unused_on_child");
    return alg;
  }

  std::shared_ptr<TestMonteCarloAbsorption> createTestAlgorithm() {
    using Mantid::API::IAlgorithm;
    auto alg = std::make_shared<TestMonteCarloAbsorption>();
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->setProperty("EventsPerPoint", 300);
    alg->setPropertyValue("OutputWorkspace", "__unused_on_child");
    return alg;
  }

  Mantid::API::MatrixWorkspace_const_sptr getOutputWorkspace(const Mantid::API::IAlgorithm_sptr &alg) {
    using Mantid::API::MatrixWorkspace_sptr;
    MatrixWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    TS_ASSERT(output);
    if (!output)
      TS_FAIL("Algorithm has not set an output workspace");
    return output;
  }

  void verifyDimensions(TestWorkspaceDescriptor wsProps, const Mantid::API::MatrixWorkspace_const_sptr &outputWS) {
    TS_ASSERT_EQUALS(wsProps.nspectra, outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(wsProps.nbins, outputWS->blocksize());
  }
};

class MonteCarloAbsorptionTestPerformance : public CxxTest::TestSuite {
public:
  static MonteCarloAbsorptionTestPerformance *createSuite() { return new MonteCarloAbsorptionTestPerformance(); }

  static void destroySuite(MonteCarloAbsorptionTestPerformance *suite) { delete suite; }

  MonteCarloAbsorptionTestPerformance() {
    TestWorkspaceDescriptor wsProps = {
        10, 700, true, Environment::CylinderSampleOnly, Mantid::Kernel::DeltaEMode::Elastic, -1};

    inputElastic = setUpWS(wsProps);

    wsProps.emode = Mantid::Kernel::DeltaEMode::Direct;
    wsProps.efixed = 12.0;
    inputDirect = setUpWS(wsProps);

    wsProps.emode = Mantid::Kernel::DeltaEMode::Indirect;
    wsProps.efixed = 1.845;
    inputIndirect = setUpWS(wsProps);

    wsProps.sampleEnviron = Environment::MeshSamplePlusContainer;
    wsProps.emode = Mantid::Kernel::DeltaEMode::Elastic;
    inputElasticMesh = setUpWS(wsProps);
  }

  void test_exec_sample_elastic() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputElastic);
    alg.setProperty("EventsPerPoint", 300);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

  void test_exec_sample_direct() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputDirect);
    alg.setProperty("EventsPerPoint", 300);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

  void test_exec_sample_indirect() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputIndirect);
    alg.setProperty("EventsPerPoint", 300);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

  void test_exec_sample_elastic_mesh() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputElasticMesh);
    alg.setProperty("EventsPerPoint", 100);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

private:
  Mantid::API::Workspace_sptr inputElastic;
  Mantid::API::Workspace_sptr inputDirect;
  Mantid::API::Workspace_sptr inputIndirect;
  Mantid::API::Workspace_sptr inputElasticMesh;
};
