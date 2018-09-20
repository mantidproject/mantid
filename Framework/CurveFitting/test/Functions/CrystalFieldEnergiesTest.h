#ifndef MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/CrystalFieldEnergies.h"
#include "MantidCurveFitting/FortranDefs.h"
#include <map>

using Mantid::CurveFitting::ComplexFortranMatrix;
using Mantid::CurveFitting::ComplexMatrix;
using Mantid::CurveFitting::ComplexType;
using Mantid::CurveFitting::CrystalFieldEnergies;
using Mantid::CurveFitting::DoubleFortranMatrix;
using Mantid::CurveFitting::DoubleFortranVector;
using Mantid::CurveFitting::GSLVector;

class CrystalFieldEnergiesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldEnergiesTest *createSuite() {
    return new CrystalFieldEnergiesTest();
  }
  static void destroySuite(CrystalFieldEnergiesTest *suite) { delete suite; }

  void test_Init() {
    CrystalFieldEnergies alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_C2v() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["B42"] = -3.8296;
    bkq["B44"] = -2.3210;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C2() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["IB42"] = -3.8296;
    bkq["IB44"] = -2.3210;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_Ci() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B21"] = 2.0;
    bkq["IB22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["IB41"] = -1.8296;
    bkq["IB42"] = -3.8296;
    bkq["IB43"] = -4.8296;
    bkq["IB44"] = -2.3210;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C4() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B40"] = 7.4851;
    bkq["B44"] = 0.4062;
    bkq["B60"] = -3.8296;
    bkq["IB64"] = -2.3210;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_D4() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B40"] = 7.4851;
    bkq["B44"] = 0.4062;
    bkq["B60"] = -3.8296;
    bkq["B64"] = -2.3210;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C3() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B40"] = 7.4851;
    bkq["B43"] = 0.4062;
    bkq["B60"] = -3.8296;
    bkq["IB63"] = -2.3210;
    bkq["IB66"] = 3.2310;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_D3() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B40"] = 7.4851;
    bkq["B43"] = 0.4062;
    bkq["B60"] = -3.8296;
    bkq["B63"] = -2.3210;
    bkq["B66"] = 3.2310;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C6() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B40"] = 7.4851;
    bkq["B60"] = -3.8296;
    bkq["B66"] = -2.3210;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_T() {
    std::map<std::string, double> bkq;
    bkq["B40"] = 0.3365;
    bkq["B44"] = 5 * bkq["B40"];
    bkq["B60"] = -3.8296;
    bkq["B64"] = -21 * bkq["B60"];

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, emptyBme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C2v_mol() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["B42"] = -3.8296;
    bkq["B44"] = -2.3210;

    std::map<std::string, double> bme;
    bme["BmolX"] = 1;
    bme["BmolY"] = 2;
    bme["BmolX"] = 3;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, bme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C2v_ext() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["B42"] = -3.8296;
    bkq["B44"] = -2.3210;

    std::map<std::string, double> bme;
    bme["BextX"] = 1;
    bme["BextY"] = 2;
    bme["BextZ"] = 3;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, bme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

  void test_C2v_mol_ext() {
    std::map<std::string, double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["B42"] = -3.8296;
    bkq["B44"] = -2.3210;

    std::map<std::string, double> bme;
    bme["BextX"] = 1;
    bme["BextY"] = 2;
    bme["BextZ"] = 3;
    bme["BmolX"] = 3;
    bme["BmolY"] = 2;
    bme["BmolZ"] = 1;

    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, bme, evalues, evectors, hamiltonian);
    doTestEigensystem(evalues, evectors, hamiltonian);
  }

private:
  bool run(int nre, const std::map<std::string, double> &bkq,
           const std::map<std::string, double> &bme, GSLVector &evalues,
           ComplexMatrix &evectors, ComplexMatrix &hamiltonian) {
    CrystalFieldEnergies alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Nre", nre));
    for (auto b = bkq.begin(); b != bkq.end(); ++b) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(b->first, b->second));
    }
    for (auto b = bme.begin(); b != bme.end(); ++b) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(b->first, b->second));
    }
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    if (!alg.isExecuted())
      return false;

    std::vector<double> ener = alg.getProperty("Energies");
    evalues = ener;
    auto n = ener.size();

    std::vector<double> eigenv = alg.getProperty("Eigenvectors");
    evectors.resize(n, n);
    evectors.unpackFromStdVector(eigenv);

    std::vector<double> ham = alg.getProperty("Hamiltonian");
    hamiltonian.resize(n, n);
    hamiltonian.unpackFromStdVector(ham);

    return true;
  }

private:
  std::map<std::string, double> emptyBme;

  void doTestEigensystem(GSLVector &en, ComplexMatrix &wf, ComplexMatrix &ham) {
    const size_t n = en.size();
    if (n <= 1)
      return;
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

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_ */
