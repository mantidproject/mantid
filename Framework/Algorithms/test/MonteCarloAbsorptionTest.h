#ifndef MONTECARLOABSORPTIONTEST_H_
#define MONTECARLOABSORPTIONTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
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

Mantid::API::MatrixWorkspace_sptr
setUpWS(const TestWorkspaceDescriptor &wsProps) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  namespace PhysicalConstants = Mantid::PhysicalConstants;

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
  // Define a sample shape
  Object_sptr sampleShape =
      ComponentCreationHelper::createSphere(0.1, V3D(), "sample-sphere");
  // And a material
  sampleShape->setMaterial(
      Material("Vanadium", PhysicalConstants::getNeutronAtom(23, 0), 0.072));
  space->mutableSample().setShape(*sampleShape);

  if (wsProps.sampleEnviron == Environment::SamplePlusContainer) {
    const std::string id("container");
    const double radius(0.11);
    const double height(0.03);
    const V3D baseCentre(0.0, -height / 2.0, 0.0);
    const V3D axis(0.0, 1.0, 0.0);

    ShapeFactory shapeMaker;
    auto can = shapeMaker.createShape<Container>(
        ComponentCreationHelper::cappedCylinderXML(radius, height, baseCentre,
                                                   axis, id));
    can->setMaterial(Material("CanMaterial",
                              PhysicalConstants::getNeutronAtom(26, 0), 0.01));
    SampleEnvironment *env = new SampleEnvironment("can", can);
    space->mutableSample().setEnvironment(env);
  } else if (wsProps.sampleEnviron == Environment::UserBeamSize) {
    auto source = inst->getSource();
    pmap.addDouble(source->getComponentID(), "beam-width", wsProps.beamWidth);
    pmap.addDouble(source->getComponentID(), "beam-height", wsProps.beamHeight);
  }
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

    TS_ASSERT_DELTA(0.0176638, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.00194808, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.000129304, outputWS->y(0).back(), delta);
    TS_ASSERT_DELTA(0.0175918, outputWS->y(2).front(), delta);
    TS_ASSERT_DELTA(0.00202576, outputWS->y(2)[middle_index], delta);
    TS_ASSERT_DELTA(0.000141414, outputWS->y(2).back(), delta);
    TS_ASSERT_DELTA(0.017851, outputWS->y(4).front(), delta);
    TS_ASSERT_DELTA(0.00223242, outputWS->y(4)[middle_index], delta);
    TS_ASSERT_DELTA(0.000208047, outputWS->y(4).back(), delta);
  }

  void test_Workspace_With_Just_Sample_For_Direct() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Direct, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.00845939, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.00306779, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.00099556, outputWS->y(0).back(), delta);
  }

  void test_Workspace_With_Just_Sample_For_Indirect() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Indirect, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.00399636, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.0012619, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.000239975, outputWS->y(0).back(), delta);
  }

  void test_Workspace_With_Sample_And_Container() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SamplePlusContainer,
                                       DeltaEMode::Elastic, -1, -1};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta1(1e-03);
    const double delta2(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.0185763, outputWS->y(0).front(), delta1);
    TS_ASSERT_DELTA(0.00305882, outputWS->y(0)[middle_index], delta2);
    TS_ASSERT_DELTA(0.000341636, outputWS->y(0).back(), delta2);
  }

  void test_Workspace_Beam_Size_Set() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::UserBeamSize,
                                       DeltaEMode::Elastic, 0.18, 0.15};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-05);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.0128107, outputWS->y(0).front(), delta);
    TS_ASSERT_DELTA(0.000519085, outputWS->y(0)[middle_index], delta);
    TS_ASSERT_DELTA(4.69767e-05, outputWS->y(0).back(), delta);
  }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------
  void test_Workspace_With_No_Instrument_Is_Not_Accepted() {
    using namespace Mantid::API;

    auto mcAbsorb = createAlgorithm();
    // Create a simple test workspace that has no instrument
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace(1, 1);

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

private:
  Mantid::API::MatrixWorkspace_const_sptr
  runAlgorithm(const TestWorkspaceDescriptor &wsProps) {
    auto inputWS = setUpWS(wsProps);
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
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
