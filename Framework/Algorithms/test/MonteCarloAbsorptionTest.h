#ifndef MONTECARLOABSORPTIONTEST_H_
#define MONTECARLOABSORPTIONTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

namespace {
enum class Environment { SampleOnly, SamplePlusContainer, UserBeamSize };

struct TestWorkspaceDescriptor {
  int nspectra;
  int nbins;
  Environment sampleEnviron;
  unsigned int emode;
  double beamWidth;
  double beamHeight;
};

void addSample(Mantid::API::MatrixWorkspace_sptr ws,
               const Environment environment, double beamWidth = 0.,
               double beamHeight = 0.) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  namespace PhysicalConstants = Mantid::PhysicalConstants;

  // Define a sample shape
  auto sampleShape =
      ComponentCreationHelper::createSphere(0.1, V3D(), "sample-sphere");
  // And a material assuming it's a CSG Object
  sampleShape->setMaterial(
      Material("Vanadium", PhysicalConstants::getNeutronAtom(23, 0), 0.072));
  ws->mutableSample().setShape(sampleShape);

  if (environment == Environment::SamplePlusContainer) {
    const std::string id("container");
    const double radius(0.11);
    const double height(0.03);
    const V3D baseCentre(0.0, -height / 2.0, 0.0);
    const V3D axis(0.0, 1.0, 0.0);

    ShapeFactory shapeMaker;
    auto canShape =
        shapeMaker.createShape(ComponentCreationHelper::cappedCylinderXML(
            radius, height, baseCentre, axis, id));
    // Set material assuming it's a CSG Object
    canShape->setMaterial(Material(
        "CanMaterial", PhysicalConstants::getNeutronAtom(26, 0), 0.01));
    auto can = boost::make_shared<Container>(canShape);
    SampleEnvironment *env = new SampleEnvironment("can", can);
    ws->mutableSample().setEnvironment(env);
  } else if (environment == Environment::UserBeamSize) {
    auto inst = ws->getInstrument();
    auto &pmap = ws->instrumentParameters();
    auto source = inst->getSource();
    pmap.addDouble(source->getComponentID(), "beam-width", beamWidth);
    pmap.addDouble(source->getComponentID(), "beam-height", beamHeight);
  }
}

Mantid::API::MatrixWorkspace_sptr
setUpWS(const TestWorkspaceDescriptor &wsProps) {
  using namespace Mantid::Kernel;

  auto space = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
      wsProps.nspectra, wsProps.nbins);
  // Needs to have units of wavelength
  space->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  auto inst = space->getInstrument();
  auto &pmap = space->instrumentParameters();

  if (wsProps.emode == DeltaEMode::Direct) {
    pmap.addString(inst.get(), "deltaE-mode", "Direct");
    const double efixed(12.0);
    space->mutableRun().addProperty<double>("Ei", efixed);
  } else if (wsProps.emode == DeltaEMode::Indirect) {
    const double efixed(1.845);
    pmap.addString(inst.get(), "deltaE-mode", "Indirect");
    pmap.addDouble(inst.get(), "Efixed", efixed);
  }

  addSample(space, wsProps.sampleEnviron, wsProps.beamWidth,
            wsProps.beamHeight);
  return space;
}
}

class MonteCarloAbsorptionTest : public CxxTest::TestSuite {
public:
  //---------------------------------------------------------------------------
  // Success cases
  //---------------------------------------------------------------------------
  void test_Workspace_With_Just_Sample_For_Elastic() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {5, 10, Environment::SampleOnly,
                                       DeltaEMode::Elastic, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.006335, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000092, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.000029, outputWS->y(0).back(), delta);
    TS_ASSERT_DELTA(0.006265, outputWS->y(2).front(), delta);
    TS_ASSERT_DELTA(0.000137, outputWS->y(2)[middle_index], delta);
    TS_ASSERT_DELTA(0.000031, outputWS->y(2).back(), delta);
    TS_ASSERT_DELTA(0.006294, outputWS->y(4).front(), delta);
    TS_ASSERT_DELTA(0.000230, outputWS->y(4)[middle_index], delta);
    TS_ASSERT_DELTA(0.000035, outputWS->y(4).back(), delta);
  }

  void test_Workspace_With_Just_Sample_For_Direct() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Direct, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.002188, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000131, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.000276, outputWS->y(0).back(), delta);
  }

  void test_Workspace_With_Just_Sample_For_Indirect() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Indirect, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.001107, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000023, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.000051, outputWS->y(0).back(), delta);
  }

  void test_Workspace_With_Sample_And_Container() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SamplePlusContainer,
                                       DeltaEMode::Elastic, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.003521, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000225, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.000018, outputWS->y(0).back(), delta);
  }

  void test_Workspace_Beam_Size_Set() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::UserBeamSize,
                                       DeltaEMode::Elastic, 0.18, 0.15};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(0.002546, outputWS->y(0).front(), delta);
    const double delta2(1e-08);
    TS_ASSERT_DELTA(0.0000005359, outputWS->y(0)[middle_index], delta2);
    TS_ASSERT_DELTA(0.0000046103, outputWS->y(0).back(), delta2);
  }

  void test_Linear_Interpolation() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Elastic, -1, -1};
    const int nlambda(5);
    const std::string interpolation("Linear");
    auto outputWS = runAlgorithm(wsProps, nlambda, interpolation);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    TS_ASSERT_DELTA(0.006335, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000432, outputWS->y(0)[3], delta);
    TS_ASSERT_DELTA(0.000321, outputWS->y(0)[4], delta);
    TS_ASSERT_DELTA(0.000438, outputWS->y(0).back(), delta);
  }

  void test_CSpline_Interpolation() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Elastic, -1, -1};
    const int nlambda(5);
    const std::string interpolation("CSpline");
    auto outputWS = runAlgorithm(wsProps, nlambda, interpolation);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    TS_ASSERT_DELTA(0.006335, outputWS->y(0).front(), delta);
    // Interpolation gives some negative value due to test setup
    TS_ASSERT_DELTA(0.000016, outputWS->y(0)[3], delta);
    TS_ASSERT_DELTA(0.000321, outputWS->y(0)[4], delta);
    TS_ASSERT_DELTA(0.000438, outputWS->y(0).back(), delta);
  }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------
  void test_Workspace_With_No_Instrument_Is_Not_Accepted() {
    using namespace Mantid::API;

    auto mcAbsorb = createAlgorithm();
    // Create a simple test workspace that has no instrument
    auto testWS = WorkspaceCreationHelper::create2DWorkspace(1, 1);

    TS_ASSERT_THROWS(mcAbsorb->setProperty("InputWorkspace", testWS),
                     std::invalid_argument);
  }

  void test_Workspace_With_An_Invalid_Sample_Shape_Is_Not_Accepted() {
    using namespace Mantid::API;

    auto testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 1);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", testWS));
    TS_ASSERT_THROWS(mcabs->execute(), std::invalid_argument);
  }

  void test_Lower_Limit_for_Number_of_Wavelengths() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Direct, -1, -1};
    int nlambda{1};
    TS_ASSERT_THROWS(runAlgorithm(wsProps, nlambda, "Linear"),
                     std::runtime_error)
    nlambda = 2;
    TS_ASSERT_THROWS(runAlgorithm(wsProps, nlambda, "CSpline"),
                     std::runtime_error)
  }

  void test_event_workspace() {
    auto inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(5, 2,
                                                                        true);
    inputWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    addSample(inputWS, Environment::SampleOnly);

    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
    TS_ASSERT(mcabs->execute());
    // only checking that it can successfully execute
  }

  void test_Sparse_Instrument_For_Elastic() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {5, 10, Environment::SampleOnly,
                                       DeltaEMode::Elastic, -1, -1};
    auto outputWS = runAlgorithm(wsProps, 5, "Linear", true, 3, 3);

    verifyDimensions(wsProps, outputWS);
    const double delta{1e-04};
    const size_t middle_index{4};
    TS_ASSERT_DELTA(0.003861, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000062, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(3.8547e-07, outputWS->y(0).back(), delta);
    TS_ASSERT_DELTA(0.003804, outputWS->y(2).front(), delta);
    TS_ASSERT_DELTA(3.30326e-05, outputWS->y(2)[middle_index], delta);
    TS_ASSERT_DELTA(3.84174e-07, outputWS->y(2).back(), delta);
    TS_ASSERT_DELTA(0.003797, outputWS->y(4).front(), delta);
    TS_ASSERT_DELTA(0.000174, outputWS->y(4)[middle_index], delta);
    TS_ASSERT_DELTA(4.21291e-07, outputWS->y(4).back(), delta);
  }

  void test_Sparse_Instrument_For_Direct() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Direct, -1, -1};
    auto outputWS = runAlgorithm(wsProps, 5, "Linear", true, 3, 3);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.001065, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000137, outputWS->y(0)[middle_index], delta);
    const double delta2(1e-08);
    TS_ASSERT_DELTA(0.00000167, outputWS->y(0).back(), delta2);
  }

  void test_Sparse_Instrument_For_Indirect() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Indirect, -1, -1};
    auto outputWS = runAlgorithm(wsProps, 5, "Linear", true, 3, 3);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.000606, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000022, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(9.16794e-07, outputWS->y(0).back(), delta);
  }

private:
  Mantid::API::MatrixWorkspace_const_sptr
  runAlgorithm(const TestWorkspaceDescriptor &wsProps, int nlambda = -1,
               const std::string &interpolate = "",
               const bool sparseInstrument = false, const int sparseRows = 2,
               const int sparseColumns = 2) {
    auto inputWS = setUpWS(wsProps);
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
    if (nlambda > 0) {
      TS_ASSERT_THROWS_NOTHING(
          mcabs->setProperty("NumberOfWavelengthPoints", nlambda));
    }
    if (!interpolate.empty()) {
      TS_ASSERT_THROWS_NOTHING(
          mcabs->setProperty("Interpolation", interpolate));
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
    using Mantid::API::IAlgorithm;
    using Mantid::Algorithms::MonteCarloAbsorption;
    auto alg = boost::make_shared<MonteCarloAbsorption>();
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->setPropertyValue("OutputWorkspace", "__unused_on_child");
    return alg;
  }

  Mantid::API::MatrixWorkspace_const_sptr
  getOutputWorkspace(Mantid::API::IAlgorithm_sptr alg) {
    using Mantid::API::MatrixWorkspace_sptr;
    MatrixWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    TS_ASSERT(output);
    if (!output)
      TS_FAIL("Algorithm has not set an output workspace");
    return output;
  }

  void verifyDimensions(TestWorkspaceDescriptor wsProps,
                        Mantid::API::MatrixWorkspace_const_sptr outputWS) {
    TS_ASSERT_EQUALS(wsProps.nspectra, outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(wsProps.nbins, outputWS->blocksize());
  }
};

class MonteCarloAbsorptionTestPerformance : public CxxTest::TestSuite {
public:
  static MonteCarloAbsorptionTestPerformance *createSuite() {
    return new MonteCarloAbsorptionTestPerformance();
  }

  static void destroySuite(MonteCarloAbsorptionTestPerformance *suite) {
    delete suite;
  }

  MonteCarloAbsorptionTestPerformance() {
    TestWorkspaceDescriptor wsProps = {10, 700, Environment::SampleOnly,
                                       Mantid::Kernel::DeltaEMode::Elastic, -1,
                                       -1};

    inputElastic = setUpWS(wsProps);

    wsProps.emode = Mantid::Kernel::DeltaEMode::Direct;
    inputDirect = setUpWS(wsProps);

    wsProps.emode = Mantid::Kernel::DeltaEMode::Indirect;
    inputIndirect = setUpWS(wsProps);
  }

  void test_exec_sample_elastic() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputElastic);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

  void test_exec_sample_direct() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputDirect);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

  void test_exec_sample_indirect() {
    Mantid::Algorithms::MonteCarloAbsorption alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputIndirect);
    alg.setPropertyValue("OutputWorkspace", "__unused_on_child");
    alg.execute();
  }

private:
  Mantid::API::Workspace_sptr inputElastic;
  Mantid::API::Workspace_sptr inputDirect;
  Mantid::API::Workspace_sptr inputIndirect;
};

#endif
