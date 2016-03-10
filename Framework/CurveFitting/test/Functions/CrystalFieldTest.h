#ifndef MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/CrystalElectricField.h"

using Mantid::CurveFitting::Functions::DoubleFortranMatrix;
using Mantid::CurveFitting::Functions::DoubleFortranVector;
using Mantid::CurveFitting::Functions::ComplexFortranMatrix;
using Mantid::CurveFitting::GSLVector;
using Mantid::CurveFitting::ComplexMatrix;
using Mantid::CurveFitting::Functions::sc_crystal_field;

class CrystalFieldTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldTest *createSuite() { return new CrystalFieldTest(); }
  static void destroySuite(CrystalFieldTest *suite) { delete suite; }

  void test_stuff() {
    int nre = 1;
    const std::string &type = "?";
    int symmetry = 2;
    double temp = 25.0;
    const DoubleFortranMatrix sbkq(0,6, 0,6);
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0,6, 0,6);

    bmol(1) = 10.;
    bkq(2, 0) = 0.3365;
    bkq(2, 2) = 7.4851;
    bkq(4, 0) = 0.4062;
    bkq(4, 2) = -3.8296;
    bkq(4, 4) = -2.3210;

    GSLVector en;
    ComplexMatrix wf;

    std::tie(en, wf) = sc_crystal_field(nre, type, symmetry, sbkq, bmol, bext, bkq, temp);
    std::cerr << "Energies:" << std::endl;
    std::cerr << en << std::endl;
    std::cerr << "Wavefunctions:" << std::endl;
    std::cerr << wf<< std::endl;

    ComplexMatrix I = wf.ctr() * wf;
    std::cerr << I << std::endl;
  }

};

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_ */
