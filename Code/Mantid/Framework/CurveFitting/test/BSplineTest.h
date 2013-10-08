#ifndef BSPLINETEST_H_
#define BSPLINETEST_H_

#include "MantidCurveFitting/BSpline.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <cxxtest/TestSuite.h>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class BSplineTest: public CxxTest::TestSuite
{
public:

    void test_defaults()
    {
        BSpline bsp;
        int order = bsp.getAttribute("Order").asInt();
        int nbreak = bsp.getAttribute("NBreak").asInt();
        size_t nparams = bsp.nParams();

        TS_ASSERT_EQUALS( order, 3 );
        TS_ASSERT_EQUALS( nbreak, 10 );
        TS_ASSERT_EQUALS( nparams, 11 );
        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), 0.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), 1.0 );

    }

};

#endif /*BSPLINETEST_H_*/
