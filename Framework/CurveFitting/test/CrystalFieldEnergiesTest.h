#ifndef MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/CrystalFieldEnergies.h"
#include "MantidCurveFitting/FortranDefs.h"
#include <map>

using Mantid::CurveFitting::CrystalFieldEnergies;
using Mantid::CurveFitting::DoubleFortranMatrix;
using Mantid::CurveFitting::DoubleFortranVector;
using Mantid::CurveFitting::ComplexFortranMatrix;
using Mantid::CurveFitting::ComplexType;
using Mantid::CurveFitting::GSLVector;
using Mantid::CurveFitting::ComplexMatrix;

class CrystalFieldEnergiesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldEnergiesTest *createSuite() { return new CrystalFieldEnergiesTest(); }
  static void destroySuite( CrystalFieldEnergiesTest *suite ) { delete suite; }


  void test_Init()
  {
    CrystalFieldEnergies alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_it_works()
  {
    std::map<std::string,double> bkq;
    bkq["B20"] = 0.3365;
    bkq["B22"] = 7.4851;
    bkq["B40"] = 0.4062;
    bkq["B42"] = -3.8296;
    bkq["B44"] = -2.3210;
    
    GSLVector evalues;
    ComplexMatrix evectors;
    ComplexMatrix hamiltonian;
    run(1, bkq, evalues, evectors, hamiltonian);

    doTestEigensystem(evalues, evectors, hamiltonian);

  }

private:

  void run(int nre, const std::map<std::string, double> &bkq,
           GSLVector &evalues, ComplexMatrix &evectors,
           ComplexMatrix &hamiltonian) 
  {
    CrystalFieldEnergies alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Nre", nre) );
    for(auto b = bkq.begin(); b != bkq.end(); ++b) {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty(b->first, b->second) );
    }
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    std::vector<double> ener = alg.getProperty("Energies");
    evalues = ener;
    auto n = ener.size();

    std::vector<double> eigenv = alg.getProperty("Eigenvectors");
    evectors.resize(n, n);
    evectors.unpackFromStdVector(eigenv);

    std::vector<double> ham = alg.getProperty("Hamiltonian");
    hamiltonian.resize(n, n);
    hamiltonian.unpackFromStdVector(ham);
  }
  
private:

  void doTestEigensystem(GSLVector &en, ComplexMatrix &wf, ComplexMatrix &ham) {
    const size_t n = en.size();
    TS_ASSERT_DIFFERS(n, 0);
    TS_ASSERT_EQUALS(wf.size1(), n);
    TS_ASSERT_EQUALS(wf.size2(), n);
    TS_ASSERT_EQUALS(ham.size1(), n);
    TS_ASSERT_EQUALS(ham.size2(), n);

    ComplexMatrix I = wf.ctr() * wf;
    TS_ASSERT_EQUALS(I.size1(), n);
    TS_ASSERT_EQUALS(I.size2(), n);

    for(size_t i = 0; i < I.size1(); ++i) {
      for(size_t j = 0; j < I.size2(); ++j) {
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
    for(size_t i = 0; i < V.size1(); ++i) {
      ComplexType value = V(i, i);
      if (value.real() < minValue) {
        minValue = value.real();
      }
    }

    for(size_t i = 0; i < V.size1(); ++i) {
      for(size_t j = 0; j < V.size2(); ++j) {
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
