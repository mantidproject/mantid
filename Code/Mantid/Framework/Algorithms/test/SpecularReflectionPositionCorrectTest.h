#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECTTEST_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SpecularReflectionPositionCorrect.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Exception.h"

using Mantid::Algorithms::SpecularReflectionPositionCorrect;

class SpecularReflectionPositionCorrectTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionPositionCorrectTest *createSuite()
  {
    return new SpecularReflectionPositionCorrectTest();
  }
  static void destroySuite(SpecularReflectionPositionCorrectTest *suite)
  {
    delete suite;
  }

  void test_init()
  {
    SpecularReflectionPositionCorrect alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_theta_is_mandatory()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error&);
  }

  void test_theta_is_greater_than_zero_else_throws()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("ThetaIn", 0.0), std::invalid_argument&);
  }

  void test_theta_is_less_than_ninety_else_throws()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("ThetaIn", 90.0), std::invalid_argument&);
  }

  void test_throws_if_SpectrumNumbersOfGroupedDetectors_less_than_zero()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", 10.0);
    std::vector<int> invalid(1, -1);
    TS_ASSERT_THROWS(alg.setProperty("SpectrumNumbersOfGroupedDetectors", invalid),
        std::invalid_argument&);
  }

  void test_throws_if_SpectrumNumbersOfGroupedDetectors_outside_range()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", 10.0);
    std::vector<int> invalid(1, 1e7);
    alg.setProperty("SpectrumNumbersOfGroupedDetectors", invalid);// Well outside range
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument&);
  }

  void test_throws_if_DetectorComponentName_unknown()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", 10.0);
    std::vector<int> invalid(1, 1e7);
    alg.setProperty("DetectorComponentName", "junk_value");// Well outside range
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument&);
  }



};

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECTTEST_H_ */
