#ifndef MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"

using Mantid::CurveFitting::ComplexFortranMatrix;
using Mantid::CurveFitting::ComplexMatrix;
using Mantid::CurveFitting::ComplexType;
using Mantid::CurveFitting::DoubleFortranMatrix;
using Mantid::CurveFitting::DoubleFortranVector;
using Mantid::CurveFitting::GSLVector;
using Mantid::CurveFitting::IntFortranVector;

using namespace Mantid::CurveFitting::Functions;

class CrystalFieldTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldTest *createSuite() { return new CrystalFieldTest(); }
  static void destroySuite(CrystalFieldTest *suite) { delete suite; }

  // Conversion factor from barn to milibarn/steradian
  const double c_mbsr = 79.5774715459;

  void test_it_works() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);
    zeroAllEntries(bmol, bext, bkq);

    bkq(2, 0) = 0.3365;
    bkq(2, 2) = 7.4851;
    bkq(4, 0) = 0.4062;
    bkq(4, 2) = -3.8296;
    bkq(4, 4) = -2.3210;

    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix ham;
    calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq);
    doTestEigensystem(en, wf, ham);
  }

  void test_mol_on() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);
    zeroAllEntries(bmol, bext, bkq);

    bmol(1) = 10.;
    bkq(2, 0) = 0.3365;
    bkq(2, 2) = 7.4851;
    bkq(4, 0) = 0.4062;
    bkq(4, 2) = -3.8296;
    bkq(4, 4) = -2.3210;

    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix ham;
    calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq);
    doTestEigensystem(en, wf, ham);
  }

  void test_rotation() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);
    zeroAllEntries(bmol, bext, bkq);

    // The internal (molecular) field is not rotated. So we leave it at zero
    // else in the rotated case (en2) it will be in a different [physical]
    // direction and so give a different splitting. The external field
    // is rotated in the code, so we set it to some value to check the
    // rotation works.
    bext(1) = 10.;
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
      calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq);
      doTestEigensystem(en, wf, ham);
      en1 = en;
    }

    GSLVector en2;
    {
      DoubleFortranVector en;
      ComplexFortranMatrix wf;
      ComplexFortranMatrix ham;
      calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq, 10.0, 20, 73.0);
      doTestEigensystem(en, wf, ham);
      en2 = en;
    }
    for (size_t i = 1; i < en1.size(); ++i) {
      TS_ASSERT_DELTA(en1.get(i), en2.get(i), 1e-6);
    }
  }

  void test_calculateIntensities() {
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
    calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq);

    IntFortranVector degeneration;
    DoubleFortranVector e_energies;
    DoubleFortranMatrix i_energies;
    const double de = 1e-10;
    calculateIntensities(nre, en, wf, 25.0, de, degeneration, e_energies,
                         i_energies);

    int n_energies = int(e_energies.size());
    TS_ASSERT_EQUALS(n_energies, 3);
    TS_ASSERT_EQUALS(i_energies.size1(), 3);
    TS_ASSERT_EQUALS(i_energies.size2(), 3);

    TS_ASSERT_DELTA(e_energies(1), en(1), 1e-10);
    TS_ASSERT_DELTA(e_energies(1), en(2), 1e-10);
    TS_ASSERT_DELTA(e_energies(2), en(3), 1e-10);
    TS_ASSERT_DELTA(e_energies(2), en(4), 1e-10);
    TS_ASSERT_DELTA(e_energies(3), en(5), 1e-10);
    TS_ASSERT_DELTA(e_energies(3), en(6), 1e-10);
  }

  void test_calculateExcitations() {
    int nre = 1;
    DoubleFortranVector bmol(1, 3);
    DoubleFortranVector bext(1, 3);
    ComplexFortranMatrix bkq(0, 6, 0, 6);

    bkq(2, 0) = 0.37737;
    bkq(2, 2) = 3.9770;
    bkq(4, 0) = -0.031787;
    bkq(4, 2) = -0.11611;
    bkq(4, 4) = -0.12544;
    double temperature = 44.0;

    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix ham;
    calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq);

    IntFortranVector degeneration;
    DoubleFortranVector e_energies;
    DoubleFortranMatrix i_energies;
    const double de = 1e-10;
    const double di = 1e-3 * c_mbsr;
    calculateIntensities(nre, en, wf, temperature, de, degeneration, e_energies,
                         i_energies);

    DoubleFortranVector e_excitations;
    DoubleFortranVector i_excitations;
    calculateExcitations(e_energies, i_energies, de, di, e_excitations,
                         i_excitations);
    TS_ASSERT_EQUALS(e_excitations.size(), 3);
    TS_ASSERT_EQUALS(i_excitations.size(), 3);
    TS_ASSERT_DELTA(e_excitations(1), 0.0, 1e-10);
    TS_ASSERT_DELTA(e_excitations(2), 29.33, 0.01);
    TS_ASSERT_DELTA(e_excitations(3), 44.34, 0.01);
    TS_ASSERT_DELTA(i_excitations(1), 2.75 * c_mbsr, 0.01 * c_mbsr);
    TS_ASSERT_DELTA(i_excitations(2), 0.72 * c_mbsr, 0.01 * c_mbsr);
    TS_ASSERT_DELTA(i_excitations(3), 0.43 * c_mbsr, 0.01 * c_mbsr);
  }

private:
  void zeroAllEntries(DoubleFortranVector &bmol, DoubleFortranVector &bext,
                      ComplexFortranMatrix &bkq) {
    bmol.zero();
    bext.zero();
    bkq.zero();
  }

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
