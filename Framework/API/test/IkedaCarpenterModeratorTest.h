// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IKEDACARPENTERMODERATORTEST_H_
#define IKEDACARPENTERMODERATORTEST_H_

#include "MantidAPI/IkedaCarpenterModerator.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

class IkedaCarpenterModeratorTest : public CxxTest::TestSuite {
public:
  using IkedaCarpenterModerator_sptr =
      boost::shared_ptr<Mantid::API::IkedaCarpenterModerator>;

  void test_Default_Object_Returns_Zero_Mean_Time() {
    Mantid::API::IkedaCarpenterModerator ikmod;
    TS_ASSERT_DELTA(ikmod.emissionTimeMean(), 0.0, 1e-12);
  }

  void test_Default_Object_Returns_Zero_Stddev() {
    Mantid::API::IkedaCarpenterModerator ikmod;
    TS_ASSERT_DELTA(ikmod.emissionTimeVariance(), 0.0, 1e-12);
  }

  void test_Default_Object_Returns_Zero_Tilt_Angle() {
    Mantid::API::IkedaCarpenterModerator ikmod;

    TS_ASSERT_DELTA(ikmod.getTiltAngleInRadians(), 0.0, 1e-12);
  }

  void test_Initializing_With_Empty_String_Throws() {
    Mantid::API::IkedaCarpenterModerator ikmod;

    TS_ASSERT_THROWS(ikmod.initialize(""), const std::invalid_argument &);
  }

  void test_Initializing_With_Invalid_String_Throws() {
    Mantid::API::IkedaCarpenterModerator ikmod;

    TS_ASSERT_THROWS(ikmod.initialize("TiltAngle"), const std::invalid_argument &);
  }

  void test_Initializing_With_String_Containing_Unknown_Parameter_Throws() {
    Mantid::API::IkedaCarpenterModerator ikmod;

    TS_ASSERT_THROWS(ikmod.initialize("unknown=6.3"), const std::invalid_argument &);
  }

  void
  test_Initializing_With_String_Containing_Some_Parameters_Leaves_Others_At_Default_Values() {
    Mantid::API::IkedaCarpenterModerator ikmod;
    ikmod.initialize("TiltAngle=27,TauF=13.55");

    checkParametersAreSet(ikmod, 27.0 * M_PI / 180., 13.55, 0.0, 0.0);
  }

  void
  test_Initializing_With_String_Containing_All_Parameters_Gives_Correct_Coefficients() {
    Mantid::API::IkedaCarpenterModerator ikmod;
    ikmod.initialize("TiltAngle=27,TauF=13.55,TauS=45,R=0.01");

    checkParametersAreSet(ikmod, 27.0 * M_PI / 180., 13.55, 45, 0.01);
  }

  void test_Setting_Tilt_Angle_On_Object_Converts_To_Radians() {
    Mantid::API::IkedaCarpenterModerator ikmod;
    const double tilt = 31.51;
    ikmod.setTiltAngleInDegrees(tilt);

    TS_ASSERT_DELTA(ikmod.getTiltAngleInRadians(), tilt * M_PI / 180.0, 1e-12);
  }

  void test_Mean_And_Variance_Are_Returned_As_Expected() {
    IkedaCarpenterModerator_sptr ikmod = createTestModerator();

    const double expectedMean = 40.65942;
    TS_ASSERT_DELTA(ikmod->emissionTimeMean(), expectedMean, 1e-10);
    const double expectedVar = 551.0628115788001;
    TS_ASSERT_DELTA(ikmod->emissionTimeVariance(), expectedVar, 1e-10);
  }

  void test_sampleTimeDistribution_With_Values_Less_Than_Zero_Throws() {
    IkedaCarpenterModerator_sptr ikmod = createTestModerator();

    TS_ASSERT_THROWS(ikmod->sampleTimeDistribution(-0.01),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(ikmod->sampleTimeDistribution(-1.5),
                     const std::invalid_argument &);
  }

  void
  test_sampleTimeDistribution_With_Value_Equal_To_Zero_Returns_Negative_Of_Mean() {
    IkedaCarpenterModerator_sptr ikmod = createTestModerator();
    const double expected = -ikmod->emissionTimeMean();

    TS_ASSERT_DELTA(ikmod->sampleTimeDistribution(0.0), expected, 1e-10);
  }

  void
  test_sampleTimeDistribution_With_Values_Within_Range_Return_Expected_Numbers() {
    IkedaCarpenterModerator_sptr ikmod = createTestModerator();

    TS_ASSERT_DELTA(ikmod->sampleTimeDistribution(0.01), -34.7497173585, 1e-10);
    TS_ASSERT_DELTA(ikmod->sampleTimeDistribution(0.1), -25.7229939652, 1e-10);
    TS_ASSERT_DELTA(ikmod->sampleTimeDistribution(0.7), 8.3428814324, 1e-10);
  }

  void
  test_sampleTimeDistribution_With_Value_Equal_To_One_Returns_998_Times_Mean() {
    IkedaCarpenterModerator_sptr ikmod = createTestModerator();
    const double expected = ikmod->emissionTimeMean() * 998;

    TS_ASSERT_DELTA(ikmod->sampleTimeDistribution(1.0), expected, 1e-10);
  }

  void test_sampleTimeDistribution_With_Value_Greater_Than_One_Throws() {
    IkedaCarpenterModerator_sptr ikmod = createTestModerator();

    TS_ASSERT_THROWS(ikmod->sampleTimeDistribution(1.01),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(ikmod->sampleTimeDistribution(5.5), const std::invalid_argument &);
  }

private:
  void checkParametersAreSet(const Mantid::API::IkedaCarpenterModerator &ikmod,
                             const double tilt, const double alpha,
                             const double beta, const double rmix) {
    TS_ASSERT_DELTA(ikmod.getTiltAngleInRadians(), tilt, 1e-10);
    TS_ASSERT_DELTA(ikmod.getFastDecayCoefficent(), alpha, 1e-10);
    TS_ASSERT_DELTA(ikmod.getSlowDecayCoefficent(), beta, 1e-10);
    TS_ASSERT_DELTA(ikmod.getMixingCoefficient(), rmix, 1e-10);
  }

  boost::shared_ptr<Mantid::API::IkedaCarpenterModerator>
  createTestModerator() {
    using namespace Mantid::API;
    IkedaCarpenterModerator_sptr moderator =
        boost::make_shared<IkedaCarpenterModerator>();
    moderator->setFastDecayCoefficent(13.55314);
    moderator->setSlowDecayCoefficent(50.0);
    moderator->setMixingCoefficient(0.0);
    return moderator;
  }
};

#endif /* IKEDACARPENTERMODERATORTEST_H_ */
