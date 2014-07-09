#ifndef MANTID_ALGORITHMS_HOLLOWCANMONTECARLOABSORPTIONTEST_H_
#define MANTID_ALGORITHMS_HOLLOWCANMONTECARLOABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/HollowCanMonteCarloAbsorption.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::HollowCanMonteCarloAbsorption;

class HollowCanMonteCarloAbsorptionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HollowCanMonteCarloAbsorptionTest *createSuite() { return new HollowCanMonteCarloAbsorptionTest(); }
  static void destroySuite( HollowCanMonteCarloAbsorptionTest *suite ) { delete suite; }

  void test_Init()
  {
    HollowCanMonteCarloAbsorption alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  //-------------------- Success cases --------------------------------

  void test_Algorithm_Attaches_Environment_To_InputWorkspace()
  {
    using Mantid::API::MatrixWorkspace_sptr;

    auto alg = createAlgorithmForAluminumTestCan();
    auto inputWS = createInputWorkspace();

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanMaterialFormula", "Al") );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "UnusedForChild") );
    TS_ASSERT_THROWS_NOTHING( alg->execute(); );
    TS_ASSERT( alg->isExecuted() );

    //MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    // Does the input workspace have a defined environment
    auto sample = inputWS->sample();
    TS_ASSERT_THROWS_NOTHING(sample.getEnvironment());
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

    auto alg = createAlgorithmForAluminumTestCan();
    auto inputWS = createInputWorkspace();

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleChemicalFormula", "A-lO") );
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument );
    TS_ASSERT( !alg->isExecuted() );
  }

  void test_Can_Material_With_More_Than_One_Atom_Is_Not_Allowed()
  {
    using Mantid::API::MatrixWorkspace_sptr;

    auto alg = createAlgorithmForAluminumTestCan();
    auto inputWS = createInputWorkspace();

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanMaterialFormula", "AlO") );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "UnusedForChild") );
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument );
    TS_ASSERT( !alg->isExecuted() );
  }

  //-------------------- Helpers --------------------------------------

private:

  Mantid::API::IAlgorithm_sptr createAlgorithmForAluminumTestCan()
  {
    auto alg = createAlgorithm();

    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "UnusedForChild") );

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanOuterRadius", 1.1) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanInnerRadius", 0.92) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanSachetHeight", 4.0) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanSachetThickness", 0.09) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("CanMaterialFormula", "Al") );

    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleHeight", 3.8) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleThickness", 0.05) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleChemicalFormula", "Li2-Ir-O3") );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SampleNumberDensity", 0.004813) );

    return alg;
  }


  Mantid::API::IAlgorithm_sptr createAlgorithm()
  {
    auto alg = boost::shared_ptr<Mantid::API::IAlgorithm>(new HollowCanMonteCarloAbsorption());
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createInputWorkspace()
  {
    const int nspectra(9), nbins(10);
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nspectra, nbins);
    // Needs to have units of wavelength
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    return inputWS;
  }

};


#endif /* MANTID_ALGORITHMS_HOLLOWCANMONTECARLOABSORPTIONTEST_H_ */
