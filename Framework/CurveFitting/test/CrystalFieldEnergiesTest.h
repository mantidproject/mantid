#ifndef MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/CrystalFieldEnergies.h"

using Mantid::CurveFitting::CrystalFieldEnergies;

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

  void test_Something()
  {
    double b20 = 0.3365;
    double b22 = 7.4851;
    double b40 = 0.4062;
    double b42 = -3.8296;
    double b44 = -2.3210;
    run(2, 3, b20, b22, b40, b42, b44);
  }

private:

  void run(int nre, int symm, double B20, double B22, double B40, double B42, double B44)
  {
    CrystalFieldEnergies alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Nre", nre) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Symmetry", symm) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Temperature", 25.0) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("B20", B20) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("B22", B22) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("B40", B40) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("B42", B42) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("B44", B44) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    std::vector<double> ener = alg.getProperty("Energies");
    std::cerr << "Energies=" << std::endl;
    for(size_t i = 0; i < ener.size(); ++i) {
      std::cerr << i << ' ' << ener[i] << std::endl;
    }

    //std::vector<double> eigenv = alg.getProperty("Eigenvectors");
    //std::cerr << "Eigenvectors=" << std::endl;
    //for(size_t i = 0; i < eigenv.size(); ++i) {
    //  std::cerr << i << ' ' << eigenv[i] << std::endl;
    //}
  }
  

};


#endif /* MANTID_CURVEFITTING_CRYSTALFIELDENERGIESTEST_H_ */
