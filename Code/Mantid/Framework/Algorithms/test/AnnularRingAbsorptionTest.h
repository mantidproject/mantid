#ifndef MANTID_ALGORITHMS_ANNULARRINGABSORPTIONTEST_H_
#define MANTID_ALGORITHMS_ANNULARRINGABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/AnnularRingAbsorption.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::AnnularRingAbsorption;

class AnnularRingAbsorptionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AnnularRingAbsorptionTest *createSuite() { return new AnnularRingAbsorptionTest(); }
  static void destroySuite( AnnularRingAbsorptionTest *suite ) { delete suite; }

  void test_Init()
  {
    AnnularRingAbsorption alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  //-------------------- Success cases --------------------------------

  void test_Algorithm_Attaches_Sample_To_InputWorkspace_And_Produces_Correct_Result()
  {
    using namespace Mantid::API;

    auto alg = createAlgorithmForTestCan();
    auto inputWS = createInputWorkspace();

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "UnusedForChild") );
    const int numOMPThreads = FrameworkManager::Instance().getNumOMPThreads();
    FrameworkManager::Instance().setNumOMPThreads(1); // To ensure reproducible results
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    FrameworkManager::Instance().setNumOMPThreads(numOMPThreads);
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS);

    const double delta(1e-08);
    const size_t middle_index = 4;
    TS_ASSERT_DELTA(outWS->readY(0).front(), 0.984770748517, delta);
    TS_ASSERT_DELTA(outWS->readY(0)[middle_index], 0.896084505371, delta);
    TS_ASSERT_DELTA(outWS->readY(0).back(), 0.807794634447, delta);
  }

  //-------------------- Failure cases --------------------------------

  void test_Workspace_With_No_Instrument_Is_Not_Accepted()
  {
    using Mantid::API::MatrixWorkspace_sptr;

    auto alg = createAlgorithm();
    // Create a simple test workspace that has no instrument
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace(10, 5);

    TS_ASSERT_THROWS(alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS),
                     std::invalid_argument);
  }

  void test_Workspace_With_Units_Not_In_Wavelength_Is_Not_Accepted()
  {
    using Mantid::API::MatrixWorkspace_sptr;

    auto alg = createAlgorithm();
    // Create a simple test workspace that has no instrument
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 5);

    TS_ASSERT_THROWS(alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS),
                     std::invalid_argument);
  }

  void test_Invalid_Sample_Material_Throws_Error()
  {
    using Mantid::API::MatrixWorkspace_sptr;

    auto alg = createAlgorithmForTestCan();
    auto inputWS = createInputWorkspace();

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleChemicalFormula", "A-lO") );
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument );
    TS_ASSERT( !alg->isExecuted() );
  }

  //-------------------- Helpers --------------------------------------

private:

  Mantid::API::IAlgorithm_sptr createAlgorithmForTestCan()
  {
    auto alg = createAlgorithm();

    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "UnusedForChild") );

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanOuterRadius", 1.1) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanInnerRadius", 0.92) );

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleHeight", 3.8) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleThickness", 0.05) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleChemicalFormula", "Li2-Ir-O3") );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleNumberDensity", 0.004813) );

    TS_ASSERT_THROWS_NOTHING( alg->setProperty<int>("NumberOfWavelengthPoints", 5000) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty<int>("EventsPerPoint", 300) );

    return alg;
  }


  Mantid::API::IAlgorithm_sptr createAlgorithm()
  {
    auto alg = boost::shared_ptr<Mantid::API::IAlgorithm>(new AnnularRingAbsorption());
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createInputWorkspace()
  {
    const int nspectra(1), nbins(9);
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nspectra, nbins);
    // Needs to have units of wavelength
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    return inputWS;
  }

};


#endif /* MANTID_ALGORITHMS_ANNULARRINGABSORPTIONTEST_H_ */
