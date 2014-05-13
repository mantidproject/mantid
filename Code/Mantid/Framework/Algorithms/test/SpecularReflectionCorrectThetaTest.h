#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONCORRECTTHETATEST_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONCORRECTTHETATEST_H_

#include <cxxtest/TestSuite.h>

#include "SpecularReflectionAlgorithmTest.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/SpecularReflectionCorrectTheta.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class SpecularReflectionCorrectThetaTest: public CxxTest::TestSuite,
    public SpecularReflectionAlgorithmTest
{

private:

  Mantid::API::IAlgorithm_sptr makeAlgorithm() const
  {
    IAlgorithm_sptr alg = boost::make_shared<SpecularReflectionCorrectTheta>();
    alg->setRethrows(true);
    alg->initialize();
    return alg;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionCorrectThetaTest *createSuite()
  {
    return new SpecularReflectionCorrectThetaTest();
  }
  static void destroySuite(SpecularReflectionCorrectThetaTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    SpecularReflectionCorrectTheta alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  SpecularReflectionCorrectThetaTest()
  {
  }

  void test_throws_if_SpectrumNumbersOfDetectors_less_than_zero()
  {
    IAlgorithm_sptr alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));

    SpecularReflectionAlgorithmTest::test_throws_if_SpectrumNumbersOfDetectors_less_than_zero(alg);
  }

  void test_throws_if_SpectrumNumbersOfDetectors_outside_range()
  {
    IAlgorithm_sptr alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));

    SpecularReflectionAlgorithmTest::test_throws_if_SpectrumNumbersOfDetectors_outside_range(alg);
  }

  void test_throws_if_DetectorComponentName_unknown()
  {
    IAlgorithm_sptr alg = makeAlgorithm();
    alg->setProperty("InputWorkspace",
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));

    SpecularReflectionAlgorithmTest::test_throws_if_DetectorComponentName_unknown(alg);
  }

};

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONCORRECTTHETATEST_H_ */
