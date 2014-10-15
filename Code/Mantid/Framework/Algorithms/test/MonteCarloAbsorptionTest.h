#ifndef MONTECARLOABSORPTIONTEST_H_
#define MONTECARLOABSORPTIONTEST_H_

#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class MonteCarloAbsorptionTest : public CxxTest::TestSuite
{
public:

  void test_That_Workspace_With_No_Instrument_Is_Not_Accepted()
  {
    using namespace Mantid::API;

    auto mcAbsorb = createAlgorithm();
    // Create a simple test workspace that has no instrument
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace(10, 5);

    TS_ASSERT_THROWS(mcAbsorb->setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS),
                     std::invalid_argument);
  }

  void test_That_Workspace_With_An_Invalid_Sample_Shape_Is_Not_Accepted()
  {
    using namespace Mantid::API;

    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(9, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    auto mcAbsorb = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    const std::string outputName("mctest-workspace");
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setPropertyValue("OutputWorkspace",outputName));
    TS_ASSERT_THROWS(mcAbsorb->execute(), std::invalid_argument);
  }

  void test_That_Workspace_With_A_Correctly_Defined_Sample_Shape_And_Material_Succeeds()
  {
    using namespace Mantid::API;

    const std::string inputName("mcabsorb-input");
    setUpWS(inputName);

    // Run the algorithm
    auto mcAbsorb = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setPropertyValue("InputWorkspace", inputName));
    const std::string outputName("mcabsorb-factors");
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setPropertyValue("OutputWorkspace",outputName));
    const int numOMPThreads = FrameworkManager::Instance().getNumOMPThreads();
    FrameworkManager::Instance().setNumOMPThreads(1); // To ensure reproducible results
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());
    FrameworkManager::Instance().setNumOMPThreads(numOMPThreads);

    AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
    MatrixWorkspace_sptr factorWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(outputName));

    TS_ASSERT(factorWS);
    if( !factorWS ) TS_FAIL("Cannot retrieve output workspace");

    const size_t nbins(factorWS->blocksize());

    // Pick out some random values
    const double delta(1e-08);
    const size_t middle_index = (nbins/2) - 1;
    TS_ASSERT_DELTA(factorWS->readY(0).front(), 0.005869405757, delta);
    TS_ASSERT_DELTA(factorWS->readY(0)[middle_index], 0.000104368636, delta);
    TS_ASSERT_DELTA(factorWS->readY(0).back(), 0.000004337609, delta);

    // Different spectra
    TS_ASSERT_DELTA(factorWS->readY(2).front(), 0.007355971026, delta);
    TS_ASSERT_DELTA(factorWS->readY(2)[middle_index], 0.000092901957, delta);
    TS_ASSERT_DELTA(factorWS->readY(2).back(), 0.000003265731, delta);

    TS_ASSERT_DELTA(factorWS->readY(4).front(), 0.004037809093, delta);
    TS_ASSERT_DELTA(factorWS->readY(4)[middle_index], 0.000190782521, delta);
    TS_ASSERT_DELTA(factorWS->readY(4).back(), 0.000019473169, delta);

    dataStore.remove(inputName);
    dataStore.remove(outputName);
  }

  void test_That_Workspace_With_A_Defined_Sample_Shape_And_Container_Succeeds()
  {
    using namespace Mantid::API;

    AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
    const std::string inputName("mcabsorb-input");
    setUpWS(inputName, 1, 10, true);

    // Run the algorithm
    auto mcAbsorb = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setPropertyValue("InputWorkspace", inputName));
    const std::string outputName("mcabsorb-factors");
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->setPropertyValue("OutputWorkspace",outputName));
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->execute());

    auto factorWS = boost::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(outputName));

    TS_ASSERT(factorWS);
    if( !factorWS ) TS_FAIL("Cannot retrieve output workspace");

    const size_t nbins(factorWS->blocksize());

    // Pick out some random values
    const double delta(1e-08);
    const size_t middle_index = (nbins/2) - 1;
    TS_ASSERT_DELTA(factorWS->readY(0).front(), 0.005122949, delta);
    TS_ASSERT_DELTA(factorWS->readY(0)[middle_index], 0.000238143162, delta);
    TS_ASSERT_DELTA(factorWS->readY(0).back(), 0.000003069996, delta);

    dataStore.remove(inputName);
    dataStore.remove(outputName);
  }


private:

  void setUpWS(const std::string & name, const int nspectra = 5, const int nbins = 10,
               bool addContainer = false)
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;
    namespace PhysicalConstants = Mantid::PhysicalConstants;

    auto space = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nspectra, nbins);
    // Needs to have units of wavelength
    space->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

    // Define a sample shape
    Object_sptr sampleShape = ComponentCreationHelper::createSphere(0.1, V3D(), "sample-sphere");
    // And a material
    sampleShape->setMaterial(Material("Vanadium", PhysicalConstants::getNeutronAtom(23,0), 0.072));
    space->mutableSample().setShape(*sampleShape);

    if( addContainer )
    {
      const std::string id("container");
      const double radius(0.11);
      const double height(0.03);
      const V3D baseCentre(0.0, -height/2.0, 0.0);
      const V3D axis(0.0, 1.0,0.0);

      // Define a container shape. Use a simple cylinder
      std::ostringstream xml;
      xml << "<cylinder id=\"" << id << "\">"
          << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
          << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
          << "<radius val=\"" << radius << "\" />"
          << "<height val=\"" << height << "\" />"
          << "</cylinder>";

      ShapeFactory shapeMaker;
      Object_sptr containerShape = shapeMaker.createShape(xml.str());
      containerShape->setMaterial(Material("CanMaterial", PhysicalConstants::getNeutronAtom(26, 0), 0.01));
      SampleEnvironment *can = new SampleEnvironment("can");
      can->add(*containerShape);
      space->mutableSample().setEnvironment(can);
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(name, space);
  }

  Mantid::API::IAlgorithm_sptr createAlgorithm()
  {
    auto mcAbsorb = boost::shared_ptr<Mantid::API::IAlgorithm>(new Mantid::Algorithms::MonteCarloAbsorption());
    TS_ASSERT_THROWS_NOTHING(mcAbsorb->initialize());
    TS_ASSERT_EQUALS(mcAbsorb->isInitialized(), true);
    mcAbsorb->setRethrows(true);
    return mcAbsorb;
  }

};

#endif
