#ifndef FERMICHOPPERMODELTEST_H_
#define FERMICHOPPERMODELTEST_H_

#include "MantidAPI/FermiChopperModel.h"
#include "MantidAPI/Run.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>
#include <iomanip>

class FermiChopperModelTest : public CxxTest::TestSuite
{
  typedef boost::shared_ptr<Mantid::API::FermiChopperModel> FermiChopperModel_sptr;

public:

  void test_Default_Object_Throws_When_Computing_Pulse_Variance()
  {
    using namespace Mantid::API;
    FermiChopperModel chopper;
    TS_ASSERT_THROWS(chopper.pulseTimeVariance(), std::invalid_argument);
  }

  void test_Object_Returns_Expected_Value_For_Time_Variance_For_Region_Below_Mean()
  {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createValidTestChopper();

    double timeVariance(0.0);
    TS_ASSERT_THROWS_NOTHING(timeVariance = chopper->pulseTimeVariance());
    TS_ASSERT_DELTA(timeVariance, 1.02729824e-10, 1e-14);
  }

  void test_Object_Returns_Expected_Value_For_Time_Variance_For_Region_Above_Mean()
  {
    //
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createValidTestChopper();
    chopper->setChopperRadius(155.0/1000.);

    double timeVariance(0.0);
    TS_ASSERT_THROWS_NOTHING(timeVariance = chopper->pulseTimeVariance());
    TS_ASSERT_DELTA(timeVariance, 3.7125748341200776e-12, 1e-14);
  }

  void test_Chopper_Throws_When_Model_Becomes_Invalid()
  {
    // Here the chopper is large & rotating fast, the model is not valid
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createValidTestChopper();

    chopper->setAngularVelocityInHz(350);
    chopper->setChopperRadius(155.0/1000.);

    TS_ASSERT_THROWS(chopper->pulseTimeVariance(), std::invalid_argument);
  }

  void test_sampleTimeDistribution_Throws_When_Given_Number_Outside_Zero_To_One()
  {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createValidTestChopper();

    TS_ASSERT_THROWS(chopper->sampleTimeDistribution(-0.01), std::invalid_argument);
    TS_ASSERT_THROWS(chopper->sampleTimeDistribution(1.01), std::invalid_argument);
  }

  void test_sampleTimeDistribution_Gives_Expected_Value_For_Flat_Random_Number()
  {
    using namespace Mantid::API;
    FermiChopperModel_sptr chopper = createValidTestChopper();

    TS_ASSERT_DELTA(chopper->sampleTimeDistribution(0.01), -2.13159150041e-05, 1e-10);
    TS_ASSERT_DELTA(chopper->sampleTimeDistribution(0.3), -5.59608403376e-06, 1e-10);
    TS_ASSERT_DELTA(chopper->sampleTimeDistribution(0.8), 9.12501923534e-06, 1e-10);
  }

  void test_Attaching_Log_To_Ei_Takes_Log_Value()
  {
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

  void test_Attaching_Log_To_ChopperSpeed_Takes_Log_Value_And_Return_It_In_Rads_A_Sec()
  {
    using namespace Mantid::API;
    FermiChopperModel chopper;
    Run exptRun;
    chopper.setRun(exptRun);
    const std::string log = "FermiSpeed";
    const double logValue = 150.;
    exptRun.addProperty(log, logValue);
    chopper.setAngularVelocityLog(log);

    TS_ASSERT_DELTA(chopper.getAngularVelocity(), logValue*2.0*M_PI, 1e-10);
  }

  void test_Clone_Produces_Object_With_Same_Propeties()
  {
    auto chopper = createValidTestChopper();
    auto cloned = chopper->clone();
    
    TS_ASSERT_DIFFERS(cloned, chopper);
    TS_ASSERT_EQUALS(cloned->getAngularVelocity(), chopper->getAngularVelocity());
  }
  
private:

  FermiChopperModel_sptr createValidTestChopper()
  {
    FermiChopperModel_sptr chopper = boost::make_shared<Mantid::API::FermiChopperModel>();
    chopper->setAngularVelocityInHz(150);
    chopper->setChopperRadius(49.0/1000.);
    chopper->setSlitRadius(1300./1000.);
    chopper->setSlitThickness(2.28/1000.);
    chopper->setIncidentEnergy(45.0);
    return chopper;
  }

};

#endif /* FermiChopperModelTEST_H_ */
