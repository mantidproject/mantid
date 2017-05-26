#ifndef CRYSTALFIELDFUNCTIONTEST_H_
#define CRYSTALFIELDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

//#include "MantidAPI/FunctionDomain1D.h"
//#include "MantidAPI/FunctionValues.h"
//#include "MantidAPI/FunctionFactory.h"
//#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Functions/CrystalFieldFunction.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldFunctionTest : public CxxTest::TestSuite {
public:

  void test_stuff() {
  }

//:GEN_TEST:CrystalField:params1
void test_params1_1() {
Mantid::CurveFitting::Functions::CrystalFieldFunction cf;
cf.setAttributeValue("Ions", "Ce");
cf.setAttributeValue("Symmetries", "C2v");
cf.setAttributeValue("Temperatures", std::vector<double>({44}));
cf.setAttributeValue("FWHMs", std::vector<double>({1}));
cf.setParameter("B20", 0.37737);
cf.setParameter("B22", 3.977);
cf.setParameter("B40", -0.031787);
cf.setParameter("B42", -0.11611);
cf.setParameter("B44", -0.12544);
cf.setParameter("IntensityScaling", 2);
 ;
double B20 = cf.getParameter("B20");
double B22 = cf.getParameter("B22");
double B40 = cf.getParameter("B40");
double B42 = cf.getParameter("B42");
double B44 = cf.getParameter("B44");
double BmolX = cf.getParameter("BmolX");
double BmolY = cf.getParameter("BmolY");
double BmolZ = cf.getParameter("BmolZ");
double BextX = cf.getParameter("BextX");
double BextY = cf.getParameter("BextY");
double BextZ = cf.getParameter("BextZ");
double IntensityScaling = cf.getParameter("IntensityScaling");
TS_ASSERT_EQUALS(B20, 3.7736999999999998e-01);
TS_ASSERT_EQUALS(B22, 3.9769999999999999e+00);
TS_ASSERT_EQUALS(B40, -3.1787000000000003e-02);
TS_ASSERT_EQUALS(B42, -1.1611000000000000e-01);
TS_ASSERT_EQUALS(B44, -1.2544000000000000e-01);
TS_ASSERT_EQUALS(BmolX, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolY, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolZ, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextX, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextY, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextZ, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(IntensityScaling, 2.0000000000000000e+00);
}

void test_params1_2() {
Mantid::CurveFitting::Functions::CrystalFieldFunction cf;
cf.setAttributeValue("Ions", "Ce");
cf.setAttributeValue("Symmetries", "C2v");
cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
cf.setAttributeValue("FWHMs", std::vector<double>({1}));
cf.setParameter("B20", 0.37737);
cf.setParameter("B22", 3.977);
cf.setParameter("B40", -0.031787);
cf.setParameter("B42", -0.11611);
cf.setParameter("B44", -0.12544);
cf.setParameter("IntensityScaling", 2);
 ;
double B20 = cf.getParameter("B20");
double B22 = cf.getParameter("B22");
double B40 = cf.getParameter("B40");
double B42 = cf.getParameter("B42");
double B44 = cf.getParameter("B44");
double BmolX = cf.getParameter("BmolX");
double BmolY = cf.getParameter("BmolY");
double BmolZ = cf.getParameter("BmolZ");
double BextX = cf.getParameter("BextX");
double BextY = cf.getParameter("BextY");
double BextZ = cf.getParameter("BextZ");
double IntensityScaling = cf.getParameter("IntensityScaling");
TS_ASSERT_EQUALS(B20, 3.7736999999999998e-01);
TS_ASSERT_EQUALS(B22, 3.9769999999999999e+00);
TS_ASSERT_EQUALS(B40, -3.1787000000000003e-02);
TS_ASSERT_EQUALS(B42, -1.1611000000000000e-01);
TS_ASSERT_EQUALS(B44, -1.2544000000000000e-01);
TS_ASSERT_EQUALS(BmolX, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolY, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolZ, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextX, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextY, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextZ, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(IntensityScaling, 2.0000000000000000e+00);
}

void test_params1_3() {
Mantid::CurveFitting::Functions::CrystalFieldFunction cf;
cf.setAttributeValue("Ions", "Ce");
cf.setAttributeValue("Symmetries", "C2v");
cf.setAttributeValue("Temperatures", std::vector<double>({44}));
cf.setAttributeValue("FWHMs", std::vector<double>({1}));
cf.setParameter("BmolX", 1);
cf.setParameter("BmolY", 2);
cf.setParameter("BmolZ", 3);
cf.setParameter("B20", 0.37737);
cf.setParameter("B22", 3.977);
cf.setParameter("B40", -0.031787);
cf.setParameter("B42", -0.11611);
cf.setParameter("B44", -0.12544);
cf.setParameter("IntensityScaling", 2);
 ;
double B20 = cf.getParameter("B20");
double B22 = cf.getParameter("B22");
double B40 = cf.getParameter("B40");
double B42 = cf.getParameter("B42");
double B44 = cf.getParameter("B44");
double BmolX = cf.getParameter("BmolX");
double BmolY = cf.getParameter("BmolY");
double BmolZ = cf.getParameter("BmolZ");
double BextX = cf.getParameter("BextX");
double BextY = cf.getParameter("BextY");
double BextZ = cf.getParameter("BextZ");
double IntensityScaling = cf.getParameter("IntensityScaling");
TS_ASSERT_EQUALS(B20, 3.7736999999999998e-01);
TS_ASSERT_EQUALS(B22, 3.9769999999999999e+00);
TS_ASSERT_EQUALS(B40, -3.1787000000000003e-02);
TS_ASSERT_EQUALS(B42, -1.1611000000000000e-01);
TS_ASSERT_EQUALS(B44, -1.2544000000000000e-01);
TS_ASSERT_EQUALS(BmolX, 1.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolY, 2.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolZ, 3.0000000000000000e+00);
TS_ASSERT_EQUALS(BextX, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextY, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextZ, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(IntensityScaling, 2.0000000000000000e+00);
}

void test_params1_4() {
Mantid::CurveFitting::Functions::CrystalFieldFunction cf;
cf.setAttributeValue("Ions", "Ce");
cf.setAttributeValue("Symmetries", "C2v");
cf.setAttributeValue("Temperatures", std::vector<double>({44}));
cf.setAttributeValue("FWHMs", std::vector<double>({1}));
cf.setParameter("BextX", 0.1);
cf.setParameter("BextY", 0.2);
cf.setParameter("BextZ", 0.3);
cf.setParameter("B20", 0.37737);
cf.setParameter("B22", 3.977);
cf.setParameter("B40", -0.031787);
cf.setParameter("B42", -0.11611);
cf.setParameter("B44", -0.12544);
cf.setParameter("IntensityScaling", 2);
 ;
double B20 = cf.getParameter("B20");
double B22 = cf.getParameter("B22");
double B40 = cf.getParameter("B40");
double B42 = cf.getParameter("B42");
double B44 = cf.getParameter("B44");
double BmolX = cf.getParameter("BmolX");
double BmolY = cf.getParameter("BmolY");
double BmolZ = cf.getParameter("BmolZ");
double BextX = cf.getParameter("BextX");
double BextY = cf.getParameter("BextY");
double BextZ = cf.getParameter("BextZ");
double IntensityScaling = cf.getParameter("IntensityScaling");
TS_ASSERT_EQUALS(B20, 3.7736999999999998e-01);
TS_ASSERT_EQUALS(B22, 3.9769999999999999e+00);
TS_ASSERT_EQUALS(B40, -3.1787000000000003e-02);
TS_ASSERT_EQUALS(B42, -1.1611000000000000e-01);
TS_ASSERT_EQUALS(B44, -1.2544000000000000e-01);
TS_ASSERT_EQUALS(BmolX, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolY, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BmolZ, 0.0000000000000000e+00);
TS_ASSERT_EQUALS(BextX, 1.0000000000000001e-01);
TS_ASSERT_EQUALS(BextY, 2.0000000000000001e-01);
TS_ASSERT_EQUALS(BextZ, 2.9999999999999999e-01);
TS_ASSERT_EQUALS(IntensityScaling, 2.0000000000000000e+00);
}

//:GEN_TEST:CrystalField:params1

};

#endif /*CRYSTALFIELDFUNCTIONTEST_H_*/
