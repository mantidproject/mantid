#ifndef MANTID_API_MODERATORMODELTEST_H_
#define MANTID_API_MODERATORMODELTEST_H_

#include "MantidAPI/ModeratorModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

class MockModerator : public Mantid::API::ModeratorModel {
public:
  boost::shared_ptr<ModeratorModel> clone() const override {
    return boost::shared_ptr<MockModerator>();
  }
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_CONST_METHOD0(emissionTimeMean, double());
  MOCK_CONST_METHOD0(emissionTimeVariance, double());
  MOCK_CONST_METHOD1(sampleTimeDistribution, double(const double));
  MOCK_METHOD2(setParameterValue,
               void(const std::string &, const std::string &));
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

class ModeratorModelTest : public CxxTest::TestSuite {

public:
  void test_Default_Object_Has_Zero_Tilt_Angle() {
    using namespace Mantid::API;
    ModeratorModel *moderator = new MockModerator;

    TS_ASSERT_DELTA(moderator->getTiltAngleInRadians(), 0.0, 1e-12);

    delete moderator;
  }

  void test_Setting_Zero_Tilt_Angle_Gives_Back_Angle_Converted_To_Radians() {
    using namespace Mantid::API;

    ModeratorModel *moderator = new MockModerator;
    const double tilt(0.6);
    moderator->setTiltAngleInDegrees(tilt);

    TS_ASSERT_DELTA(moderator->getTiltAngleInRadians(), tilt * M_PI / 180.0,
                    1e-12);

    delete moderator;
  }

  void test_emissionTimeMean_Is_Called_Expectedly() {
    MockModerator mockModerator;

    EXPECT_CALL(mockModerator, emissionTimeMean()).Times(1);
    const Mantid::API::ModeratorModel &moderator = mockModerator;
    moderator.emissionTimeMean();

    TS_ASSERT(::testing::Mock::VerifyAndClearExpectations(&mockModerator));
  }

  void test_emissionTimeVariance_Is_Called_Expectedly() {
    MockModerator mockModerator;

    EXPECT_CALL(mockModerator, emissionTimeVariance()).Times(1);
    const Mantid::API::ModeratorModel &moderator = mockModerator;
    moderator.emissionTimeVariance();

    TS_ASSERT(::testing::Mock::VerifyAndClearExpectations(&mockModerator));
  }

  void test_sampleTimeDistribution_Is_Called_Expectedly() {
    MockModerator mockModerator;

    EXPECT_CALL(mockModerator, sampleTimeDistribution(0.5)).Times(1);
    const Mantid::API::ModeratorModel &moderator = mockModerator;
    moderator.sampleTimeDistribution(0.5);

    TS_ASSERT(::testing::Mock::VerifyAndClearExpectations(&mockModerator));
  }
};

#endif /* MODERATORMODELTEST_H_ */
