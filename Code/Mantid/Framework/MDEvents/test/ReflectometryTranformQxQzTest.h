#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/ReflectometryTranformQxQz.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class ReflectometryTranformQxQzTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTranformQxQzTest *createSuite() { return new ReflectometryTranformQxQzTest(); }
  static void destroySuite( ReflectometryTranformQxQzTest *suite ) { delete suite; }


  void test_qxmin_greater_than_qxmax_throws()
  {
    double qxMin = 2;
    double qxMax = 1; //Smaller than qxMin!
    double qzMin = 1;
    double qzMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }
  
  void test_qxmin_equal_to_qxmax_throws()
  {
    double qxMin = 1;
    double qxMax = 1; //Equal to qxMin!
    double qzMin = 1;
    double qzMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }

  void test_qzmin_greater_than_qzmax_throws()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 2;
    double qzMax = 1; //Smaller than qzMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }
  
  void test_qzmin_equal_to_qzmax_throws()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 1; //Equal to qzMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::invalid_argument);
  }

  void test_incident_theta_negative()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 3; 
    double incidentTheta = -0.001; //Negative
    TS_ASSERT_THROWS(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::out_of_range);
  }

  void test_incident_theta_too_large()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 3; 
    double incidentTheta = 90.001; //Too large
    TS_ASSERT_THROWS(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta), std::out_of_range);
  }

  void test_valid_construction_inputs()
  {
    double qxMin = 1;
    double qxMax = 2; 
    double qzMin = 1;
    double qzMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS_NOTHING(ReflectometryTranformQxQz(qxMin, qxMax, qzMin, qzMax, incidentTheta));
  }

  void 


};


#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZTEST_H_ */