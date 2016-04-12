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
    TS_ASSERT_DELTA(outputWS->readY(0).front(), 0.005869405757, delta);
    TS_ASSERT_DELTA(outputWS->readY(0)[middle_index], 0.000104368636, delta);
    TS_ASSERT_DELTA(outputWS->readY(0).back(), 0.000004337609, delta);
    TS_ASSERT_DELTA(outputWS->readY(2).front(), 0.007355971026, delta);
    TS_ASSERT_DELTA(outputWS->readY(2)[middle_index], 0.000092901957, delta);
    TS_ASSERT_DELTA(outputWS->readY(2).back(), 0.000003265731, delta);
    TS_ASSERT_DELTA(outputWS->readY(4).front(), 0.004037809093, delta);
    TS_ASSERT_DELTA(outputWS->readY(4)[middle_index], 0.000190782521, delta);
    TS_ASSERT_DELTA(outputWS->readY(4).back(), 0.000019473169, delta);
  }

  void test_Workspace_With_Just_Sample_For_Direct() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Direct};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(outputWS->readY(0).front(), 0.00259928, delta);
    TS_ASSERT_DELTA(outputWS->readY(0)[middle_index], 0.00023240, delta);
    TS_ASSERT_DELTA(outputWS->readY(0).back(), 0.00010952, delta);
  }

  void test_Workspace_With_Just_Sample_For_Indirect() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SampleOnly,
                                       DeltaEMode::Indirect};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(outputWS->readY(0).front(), 0.00067034, delta);
    TS_ASSERT_DELTA(outputWS->readY(0)[middle_index], 3.877336011e-05, delta);
    TS_ASSERT_DELTA(outputWS->readY(0).back(), 6.604792751e-06, delta);
  }

  void test_Workspace_With_Sample_And_Container() {
    using Mantid::Kernel::DeltaEMode;
    TestWorkspaceDescriptor wsProps = {1, 10, Environment::SamplePlusCan,
                                       DeltaEMode::Elastic};
    auto outputWS = runAlgorithm(wsProps);

    verifyDimensions(wsProps, outputWS);
    const double delta(1e-08);
    const size_t middle_index(4);
    TS_ASSERT_DELTA(outputWS->readY(0).front(), 0.005122949, delta);
    TS_ASSERT_DELTA(outputWS->readY(0)[middle_index], 0.000238143162, delta);
    TS_ASSERT_DELTA(outputWS->readY(0).back(), 0.000003069996, delta);
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
    Environment environ;
    Mantid::Kernel::DeltaEMode::Type emode;
  };

  Mantid::API::MatrixWorkspace_const_sptr
  runAlgorithm(const TestWorkspaceDescriptor &wsProps) {
    auto inputWS = setUpWS(wsProps);
    auto mcabs = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcabs->setProperty("InputWorkspace", inputWS));
    // To ensure reproducible results we need to use a single thread
    TS_ASSERT_THROWS_NOTHING(executeOnSingleThread(mcabs));
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

    if (wsProps.environ == Environment::SamplePlusCan) {
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

  void executeOnSingleThread(Mantid::API::IAlgorithm_sptr alg) {
    using Mantid::API::FrameworkManager;
    auto &fmgr = FrameworkManager::Instance();
    const int ompThreadsOnEntry = fmgr.getNumOMPThreads();
    fmgr.setNumOMPThreads(1);
    try {
      alg->execute();
      fmgr.setNumOMPThreads(ompThreadsOnEntry);
    } catch (...) {
      fmgr.setNumOMPThreads(ompThreadsOnEntry);
      throw;
    }
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
