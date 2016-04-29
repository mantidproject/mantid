#ifndef MONTECARLOABSORPTIONTEST_H_
#define MONTECARLOABSORPTIONTEST_H_

#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class MonteCarloAbsorptionTest : public CxxTest::TestSuite {
public:
  //---------------------------------------------------------------------------
  // Success cases
  //---------------------------------------------------------------------------
  void test_Workspace_With_Just_Sample_For_Elastic() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {5, 10, Environment::SampleOnly,
                                       DeltaEMode::Elastic};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);

    TS_ASSERT_DELTA(0.21339478, outputWS->readY(0).front(), delta);
    TS_ASSERT_DELTA(0.23415902, outputWS->readY(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.18711438, outputWS->readY(0).back(), delta);
    TS_ASSERT_DELTA(0.21347241, outputWS->readY(2).front(), delta);
    TS_ASSERT_DELTA(0.2341577, outputWS->readY(2)[middle_index], delta);
    TS_ASSERT_DELTA(0.18707489, outputWS->readY(2).back(), delta);
    TS_ASSERT_DELTA(0.21367069, outputWS->readY(4).front(), delta);
    TS_ASSERT_DELTA(0.23437129, outputWS->readY(4)[middle_index], delta);
    TS_ASSERT_DELTA(0.18710594, outputWS->readY(4).back(), delta);
  }

  void test_Workspace_With_Just_Sample_For_Direct() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Direct};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(0.20488748, outputWS->readY(0).front(), delta);
    TS_ASSERT_DELTA(0.23469609, outputWS->readY(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.187899, outputWS->readY(0).back(), delta);
  }

  void test_Workspace_With_Just_Sample_For_Indirect() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Indirect};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(0.20002242, outputWS->readY(0).front(), delta);
    TS_ASSERT_DELTA(0.23373778, outputWS->readY(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.18742317, outputWS->readY(0).back(), delta);
  }

  void test_Workspace_With_Sample_And_Container() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SamplePlusCan,
                                       DeltaEMode::Elastic};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(0.22929866, outputWS->readY(0).front(), delta);
    TS_ASSERT_DELTA(0.21436937, outputWS->readY(0)[middle_index], delta);
    TS_ASSERT_DELTA(0.23038325, outputWS->readY(0).back(), delta);
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
  enum class Environment { SampleOnly, SamplePlusCan };

  struct TestWorkspaceDescriptor {
    int nspectra;
    int nbins;
    Environment sampleEnviron;
    unsigned int emode;
  };

  Mantid::API::MatrixWorkspace_const_sptr
  runAlgorithm(const TestWorkspaceDescriptor &wsProps) {
    auto inputWS = setUpWS(wsProps);
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
    mcabs->execute();
    return getOutputWorkspace(mcabs);
  }

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

    if (wsProps.sampleEnviron == Environment::SamplePlusCan) {
      const std::string id("container");
      const double radius(0.11);
      const double height(0.03);
      const V3D baseCentre(0.0, -height / 2.0, 0.0);
      const V3D axis(0.0, 1.0, 0.0);

      // Define a container shape. Use a simple cylinder
      std::ostringstream xml;
      xml << "<cylinder id=\"" << id << "\">"
          << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\""
          << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
          << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\""
          << axis.Z() << "\"/>"
          << "<radius val=\"" << radius << "\" />"
          << "<height val=\"" << height << "\" />"
          << "</cylinder>";

      ShapeFactory shapeMaker;
      Object_sptr containerShape = shapeMaker.createShape(xml.str());
      containerShape->setMaterial(Material(
          "CanMaterial", PhysicalConstants::getNeutronAtom(26, 0), 0.01));
      SampleEnvironment *can = new SampleEnvironment("can");
      can->add(*containerShape);
      space->mutableSample().setEnvironment(can);
    }
    return space;
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

#endif
