#ifndef FITPARAMETERTEST_H_
#define FITPARAMETERTEST_H_

#include <cxxtest/TestSuite.h>
#include <sstream>

#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class FitParameterTest : public CxxTest::TestSuite
{
public:


	void test1()
	{
    FitParameter fitP;

    fitP.setValue() = 9.1;
    fitP.setTie() = "bob";

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("bob")==0 );
	}
	
	void test2()
	{
    FitParameter fitP;

    std::istringstream input("9.1 , fido , , bob , boevs");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getFunction().compare("fido")==0 );
    TS_ASSERT( fitP.getTie().compare("bob")==0 );
    TS_ASSERT( fitP.getFormula().compare("boevs")==0 );
	}

	void test3()
	{
    FitParameter fitP;

    std::istringstream input("9.1 , , , , ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test4()
	{
    FitParameter fitP;

    std::istringstream input("bob , , , ,    ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 0.0 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test5()
	{
    FitParameter fitP;

    std::istringstream input("9.1 , , , , ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test6()
	{
    FitParameter fitP;

    std::istringstream input("9.1 , , ,  ,      ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test7()
	{
    FitParameter fitP;

    std::istringstream input("9.1 , , 0 < 3 < 8 , ,       ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
    TS_ASSERT( fitP.getConstraint().compare("0 < 3 < 8")==0 );
	}

};

#endif /*FITPARAMETERTEST_H_*/
