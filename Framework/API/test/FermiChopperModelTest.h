// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FERMICHOPPERMODELTEST_H_
#define FERMICHOPPERMODELTEST_H_

#include "MantidAPI/FermiChopperModel.h"
#include "MantidAPI/Run.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

class FermiChopperModelTest : public CxxTest::TestSuite {
  using FermiChopperModel_sptr =
      boost::shared_ptr<Mantid::API::FermiChopperModel>;

public:
  void test_Default_Object_Throws_When_Computing_Pulse_Variance() {
    using namespace Mantid::API;
    FermiChopperModel chopper;
    TS_ASSERT_THROWS(chopper.pulseTimeVariance(),
                     const std::invalid_argument &);
  }

  void test_Getters_Produce_Expected_Values_When_Set_By_Settings() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();
    doChopperCheck(*chopper);
  }

  void test_Getters_Produce_Expected_Values_When_Set_By_String() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperByString();
    doChopperCheck(*chopper);
  }

  void
  test_Object_Returns_Expected_Value_For_Time_Variance_For_Region_Below_Mean() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();

    double timeVariance(0.0);
    TS_ASSERT_THROWS_NOTHING(timeVariance = chopper->pulseTimeVariance());
    TS_ASSERT_DELTA(timeVariance, 1.02729824e-10, 1e-14);
  }

  void
  test_Object_Returns_Expected_Value_For_Time_Variance_For_Region_Above_Mean() {
    //
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();
    chopper->setChopperRadius(155.0 / 1000.);

    double timeVariance(0.0);
    TS_ASSERT_THROWS_NOTHING(timeVariance = chopper->pulseTimeVariance());
    TS_ASSERT_DELTA(timeVariance, 3.7125748341200776e-12, 1e-14);
  }

  void test_Chopper_Throws_When_Model_Becomes_Invalid() {
    // Here the chopper is large & rotating fast, the model is not valid
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();

    chopper->setAngularVelocityInHz(350);
    chopper->setChopperRadius(155.0 / 1000.);

    TS_ASSERT_THROWS(chopper->pulseTimeVariance(),
                     const std::invalid_argument &);
  }

  void
  test_sampleTimeDistribution_Throws_When_Given_Number_Outside_Zero_To_One() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();

    TS_ASSERT_THROWS(chopper->sampleTimeDistribution(-0.01),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(chopper->sampleTimeDistribution(1.01),
                     const std::invalid_argument &);
  }

  void
  test_sampleTimeDistribution_Gives_Expected_Value_For_Flat_Random_Number() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();

    TS_ASSERT_DELTA(chopper->sampleTimeDistribution(0.01), -2.13159150041e-05,
                    1e-10);
    TS_ASSERT_DELTA(chopper->sampleTimeDistribution(0.3), -5.59608403376e-06,
                    1e-10);
    TS_ASSERT_DELTA(chopper->sampleTimeDistribution(0.8), 9.12501923534e-06,
                    1e-10);
  }

  void
  test_sampleJitterDistribution_Gives_Zero_For_Zero_Jitter_Flat_Random_Number() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();
    chopper->setJitterFWHH(0.0);

    TS_ASSERT_DELTA(chopper->sampleJitterDistribution(0.01), 0.0, 1e-10);
  }

  void
  test_sampleJitterDistribution_Gives_Expected_Value_For_Flat_Random_Number() {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createTestChopperBySetters();

    TS_ASSERT_DELTA(chopper->sampleJitterDistribution(0.01), -0.0000010717,
                    1e-10);
    TS_ASSERT_DELTA(chopper->sampleJitterDistribution(0.3), -0.0000002814,
                    1e-10);
    TS_ASSERT_DELTA(chopper->sampleJitterDistribution(0.8), 0.0000004588,
                    1e-10);
  }

  void test_Attaching_Log_To_Ei_Takes_Log_Value() {
    using namespace Mantid::API;
    FermiChopperModel chopper;
    Run exptRun;
    chopper.setRun(exptRun);
    const std::string log = "Ei";
    const double logValue = 15.1;
    exptRun.addProperty(log, logValue);
    chopper.setIncidentEnergyLog(log);

    TS_ASSERT_DELTA(chopper.getIncidentEnergy(), logValue, 1e-10);
  }

  void
  test_Attaching_Log_To_ChopperSpeed_Takes_Log_Value_And_Return_It_In_Rads_A_Sec() {
    using namespace Mantid::API;
    FermiChopperModel chopper;
    Run exptRun;
    chopper.setRun(exptRun);
    const std::string log = "FermiSpeed";
    const double logValue = 150.;
    exptRun.addProperty(log, logValue);
    chopper.setAngularVelocityLog(log);

    TS_ASSERT_DELTA(chopper.getAngularVelocity(), logValue * 2.0 * M_PI, 1e-10);
  }

  void test_Clone_Produces_Object_With_Same_Propeties() {
    auto chopper = createTestChopperBySetters();
    auto cloned = boost::dynamic_pointer_cast<Mantid::API::FermiChopperModel>(
        chopper->clone());

    TS_ASSERT(cloned);
    TS_ASSERT_DIFFERS(cloned, chopper);
    doChopperCheck(*cloned);
  }

private:
  FermiChopperModel_sptr createTestChopperBySetters() {
    FermiChopperModel_sptr chopper =
        boost::make_shared<Mantid::API::FermiChopperModel>();
    chopper->setAngularVelocityInHz(150);
    chopper->setChopperRadius(49.0 / 1000.);
    chopper->setSlitRadius(1300. / 1000.);
    chopper->setSlitThickness(2.28 / 1000.);
    chopper->setJitterFWHH(1.2);
    chopper->setIncidentEnergy(45.0);
    return chopper;
  }

  FermiChopperModel_sptr createTestChopperByString() {
    FermiChopperModel_sptr chopper =
        boost::make_shared<Mantid::API::FermiChopperModel>();
    chopper->initialize("AngularVelocity=150,ChopperRadius=0.049,SlitRadius=1."
                        "3,SlitThickness=0.00228,JitterSigma=1.2,Ei=45");
    return chopper;
  }

  void doChopperCheck(const Mantid::API::FermiChopperModel &chopper) {
    TS_ASSERT_DELTA(chopper.getAngularVelocity(), 150 * 2.0 * M_PI, 1e-10);
    TS_ASSERT_DELTA(chopper.getChopperRadius(), 0.049, 1e-10);
    TS_ASSERT_DELTA(chopper.getSlitRadius(), 1.3, 1e-10);
    TS_ASSERT_DELTA(chopper.getSlitThickness(), 0.00228, 1e-10);
    TS_ASSERT_DELTA(chopper.getStdDevJitter(),
                    1.2 / 1e6 / std::sqrt(std::log(256.0)), 1e-10);
    TS_ASSERT_DELTA(chopper.getIncidentEnergy(), 45.0, 1e-10);
  }
};

#endif /* FermiChopperModelTEST_H_ */
