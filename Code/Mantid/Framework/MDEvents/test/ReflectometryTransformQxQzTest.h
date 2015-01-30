#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/ReflectometryTransformQxQz.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class ReflectometryTransformQxQzTest : public CxxTest::TestSuite
{
private:

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformQxQzTest *createSuite() { return new ReflectometryTransformQxQzTest(); }
  static void destroySuite( ReflectometryTransformQxQzTest *suite ) { delete suite; }


  void test_qxmin_greater_than_qxmax_throws()
  {
    double qxMin = 2;
    double qxMax = 1; //Smaller than qxMin!
    double qzMin = 1;
    double qzMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }
  
  void test_qxmin_equal_to_qxmax_throws()
  {
    double qxMin = 1;
    double qxMax = 1; //Equal to qxMin!
    double qzMin = 1;
    double qzMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }

  void test_qzmin_greater_than_qzmax_throws()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 2;
    double qzMax = 1; //Smaller than qzMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }
  
  void test_qzmin_equal_to_qzmax_throws()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 1; //Equal to qzMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }

  void test_incident_theta_negative()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 3; 
    double incidentTheta = -0.001; //Negative
    TS_ASSERT_THROWS(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::out_of_range);
  }

  void test_incident_theta_too_large()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 3; 
    double incidentTheta = 90.001; //Too large
    TS_ASSERT_THROWS(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::out_of_range);
  }

  void test_valid_construction_inputs()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS_NOTHING(ReflectometryTransformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta));
  }

  //---- Tests for Qx Calculator ---- //

  void test_calculate_Qx()
  {
    //Set up calculation so that it collapses down to 2*M_PI/wavelength by setting initial theta to M_PI/2 and final theta to zero
    CalculateReflectometryQx calculator(90);
    double qx;
    const double wavelength = 0.1;
    TS_ASSERT_THROWS_NOTHING(calculator.setThetaFinal(0));
    TS_ASSERT_THROWS_NOTHING(qx = calculator.execute(wavelength));
    TS_ASSERT_DELTA(2*M_PI/wavelength, qx, 0.0001);
  }

  void test_recalculate_Qx()
  {
    CalculateReflectometryQx calculator(0);
    calculator.setThetaFinal(0);
    const double wavelength = 0.1;
    TS_ASSERT_DELTA(0, calculator.execute(wavelength), 0.0001);

    //Now reset the final theta and should be able to re-execute
    calculator.setThetaFinal(90);
    TS_ASSERT_DELTA(-2*M_PI/wavelength, calculator.execute(wavelength), 0.0001);
  }

  //---- End Tests for Qx Calculator ---- //

  //---- Tests for Qz Calculator ---- //

  void test_calculate_Qz()
  {
    //Set up calculation so that it collapses down to 2*M_PI/wavelength 
    CalculateReflectometryQz calculator(0);
    double qx;
    const double wavelength = 0.1;
    TS_ASSERT_THROWS_NOTHING(calculator.setThetaFinal(90));
    TS_ASSERT_THROWS_NOTHING(qx = calculator.execute(wavelength));
    TS_ASSERT_DELTA(2*M_PI/wavelength, qx, 0.0001);
  }

  void test_recalculate_Qz()
  {
    CalculateReflectometryQz calculator(90);
    calculator.setThetaFinal(90);
    const double wavelength = 0.1;
    TS_ASSERT_DELTA(2*(2*M_PI/wavelength), calculator.execute(wavelength), 0.001);

    //Now reset the final theta and should be able to re-execute
    calculator.setThetaFinal(0);
    TS_ASSERT_DELTA(2*M_PI/wavelength, calculator.execute(wavelength), 0.001);
  }

  //---- End Tests for Qz Calculator ---- //
};


#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZTEST_H_ */
