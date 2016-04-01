#ifndef MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/FortranDefs.h"

using Mantid::CurveFitting::DoubleFortranMatrix;
using Mantid::CurveFitting::DoubleFortranVector;
using Mantid::CurveFitting::ComplexFortranMatrix;
using Mantid::CurveFitting::ComplexType;
using Mantid::CurveFitting::GSLVector;
using Mantid::CurveFitting::ComplexMatrix;
using Mantid::CurveFitting::Functions::calculateEigesystem;

class CrystalFieldTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldTest *createSuite() { return new CrystalFieldTest(); }
  static void destroySuite(CrystalFieldTest *suite) { delete suite; }

  void test_it_works() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);

    bkq(2, 0) = 0.3365;
    bkq(2, 2) = 7.4851;
    bkq(4, 0) = 0.4062;
    bkq(4, 2) = -3.8296;
    bkq(4, 4) = -2.3210;

    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix ham;
    calculateEigesystem(en, wf, ham, nre, bmol, bext, bkq);
    doTestEigensystem(en, wf, ham);
  }

  void test_mol_on() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);

    bmol(1) = 10.;
    bkq(2, 0) = 0.3365;
    bkq(2, 2) = 7.4851;
    bkq(4, 0) = 0.4062;
    bkq(4, 2) = -3.8296;
    bkq(4, 4) = -2.3210;

    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix ham;
    calculateEigesystem(en, wf, ham, nre, bmol, bext, bkq);
    doTestEigensystem(en, wf, ham);
  }

  void test_rotation() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);

    bmol(1) = 10.;
    bkq(2, 0) = 0.3365;
    bkq(2, 2) = 7.4851;
    bkq(4, 0) = 0.4062;
    bkq(4, 2) = -3.8296;
    bkq(4, 4) = -2.3210;

    GSLVector en1;
    {
      DoubleFortranVector en;
      ComplexFortranMatrix wf;
      ComplexFortranMatrix ham;
      calculateEigesystem(en, wf, ham, nre, bmol, bext, bkq);
      doTestEigensystem(en, wf, ham);
      en1 = en;
    }

    GSLVector en2;
    {
      DoubleFortranVector en;
      ComplexFortranMatrix wf;
      ComplexFortranMatrix ham;
      calculateEigesystem(en, wf, ham, nre, bmol, bext, bkq, 10.0, 20, 73.0);
      doTestEigensystem(en, wf, ham);
      en2 = en;
    }
    for (size_t i = 1; i < en1.size(); ++i) {
      TS_ASSERT_LESS_THAN(1.0, fabs(en1.get(i) - en2.get(i)));
    }
  }

private:
  void doTestEigensystem(DoubleFortranVector &en, ComplexFortranMatrix &wf,
                         ComplexFortranMatrix &ham) {
    const size_t n = en.size();
    TS_ASSERT_DIFFERS(n, 0);
    TS_ASSERT_EQUALS(wf.size1(), n);
    TS_ASSERT_EQUALS(wf.size2(), n);
    TS_ASSERT_EQUALS(ham.size1(), n);
    TS_ASSERT_EQUALS(ham.size2(), n);

    ComplexMatrix I = wf.ctr() * wf;
    TS_ASSERT_EQUALS(I.size1(), n);
    TS_ASSERT_EQUALS(I.size2(), n);

    for (size_t i = 0; i < I.size1(); ++i) {
      for (size_t j = 0; j < I.size2(); ++j) {
        ComplexType value = I(i, j);
        if (i == j) {
          TS_ASSERT_DELTA(value.real(), 1.0, 1e-10);
          TS_ASSERT_DELTA(value.imag(), 0.0, 1e-10);
        } else {
          TS_ASSERT_DELTA(value.real(), 0.0, 1e-10);
          TS_ASSERT_DELTA(value.imag(), 0.0, 1e-10);
        }
      }
    }

    ComplexMatrix V = wf.ctr() * ham * wf;
    TS_ASSERT_EQUALS(V.size1(), n);
    TS_ASSERT_EQUALS(V.size2(), n);

    double minValue = 1e100;
    for (size_t i = 0; i < V.size1(); ++i) {
      ComplexType value = V(i, i);
      if (value.real() < minValue) {
        minValue = value.real();
      }
    }

    for (size_t i = 0; i < V.size1(); ++i) {
      for (size_t j = 0; j < V.size2(); ++j) {
        ComplexType value = V(i, j);
        if (i == j) {
          value -= minValue;
          TS_ASSERT_DELTA(value.real(), en.get(i), 1e-10);
          TS_ASSERT_DELTA(value.imag(), 0.0, 1e-10);
        } else {
          TS_ASSERT_DELTA(value.real(), 0.0, 1e-10);
          TS_ASSERT_DELTA(value.imag(), 0.0, 1e-10);
        }
      }
    }
  }
};

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_ */
