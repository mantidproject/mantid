#ifndef DYNAMICKUBOTOYABETEST_H_
#define DYNAMICKUBOTOYABETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/DynamicKuboToyabe.h"
#include "MantidCurveFitting/StaticKuboToyabe.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;


class DynamicKuboToyabeTest : public CxxTest::TestSuite
{
public:

  void testZFZNDKTFunction()
  {
    // Test Dynamic Kubo Toyabe (DKT) for Zero Field (ZF) and Zero Nu (ZN)
    // Function values must match exactly values from the Static Kubo Toyabe
    const double asym = 1.0;
    const double delta = 0.39;
    const double field = 0;
    const double nu = 0.0;

    DynamicKuboToyabe dkt; 
    dkt.initialize();
    dkt.setParameter("Asym", asym);
    dkt.setParameter("Delta",delta );
    dkt.setParameter("Field",field);
    dkt.setParameter("Nu",   nu);

    StaticKuboToyabe skt;
    skt.initialize();
    skt.setParameter("A", asym);
    skt.setParameter("Delta", delta);

    // define 1d domain of 10 points in interval [0,10]
    Mantid::API::FunctionDomain1DVector x(0,10,10);
    Mantid::API::FunctionValues y1(x);
    Mantid::API::FunctionValues y2(x);

    TS_ASSERT_THROWS_NOTHING(dkt.function(x,y1));
    TS_ASSERT_THROWS_NOTHING(skt.function(x,y2));

    for(size_t i = 0; i < x.size(); ++i)
    {
      TS_ASSERT_DELTA( y1[i], y2[i], 1e-6 );
    }
  }

  void testZFDKTFunction()
  {
    // Test Dynamic Kubo Toyabe (DKT) for Zero Field (ZF) (non-zero Nu)
    const double asym = 1.0;
    const double delta = 0.39;
    const double field = 0;
    const double nu = 1.0;

    DynamicKuboToyabe dkt; 
    dkt.initialize();
    dkt.setParameter("Asym", asym);
    dkt.setParameter("Delta",delta );
    dkt.setParameter("Field",field);
    dkt.setParameter("Nu",   nu);

    // define 1d domain of 5 points in interval [0,5]
    Mantid::API::FunctionDomain1DVector x(0,5,5);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(dkt.function(x,y));

    TS_ASSERT_DELTA( y[0], 1.000000, 0.000001);
    TS_ASSERT_DELTA( y[1], 0.849898, 0.000001);
    TS_ASSERT_DELTA( y[2], 0.621963, 0.000001);
    TS_ASSERT_DELTA( y[3], 0.443612, 0.000001);
    TS_ASSERT_DELTA( y[4], 0.317374, 0.000001);
  }

  void xtestDKTFunction()
  {
    // Test Dynamic Kubo Toyabe (DKT) (non-zero Field, non-zero Nu)
    const double asym = 1.0;
    const double delta = 0.39;
    const double field = 0.1;
    const double nu = 0.5;

    DynamicKuboToyabe dkt; 
    dkt.initialize();
    dkt.setParameter("Asym", asym);
    dkt.setParameter("Delta",delta );
    dkt.setParameter("Field",field);
    dkt.setParameter("Nu",   nu);

    // define 1d domain of 5 points in interval [0,5]
    Mantid::API::FunctionDomain1DVector x(0,5,5);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(dkt.function(x,y));

    TS_ASSERT_DELTA( y[0], 1.000000, 0.000001);
    TS_ASSERT_DELTA( y[1], 0.816422, 0.000001);
    TS_ASSERT_DELTA( y[2], 0.503185, 0.000001);
    TS_ASSERT_DELTA( y[3], 0.274738, 0.000001);
    TS_ASSERT_DELTA( y[4], 0.152792, 0.000001);
  }


};

#endif /*DYNAMICKUBOTOYABETEST_H_*/
