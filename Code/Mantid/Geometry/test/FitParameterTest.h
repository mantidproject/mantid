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

    //operator>>(std::istream&,FitParameter&);
    std::istringstream input("9.1 bob");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("bob")==0 );
	}

	void test3()
	{
    FitParameter fitP;

    //operator>>(std::istream&,FitParameter&);
    std::istringstream input("9.1");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test4()
	{
    FitParameter fitP;

    //operator>>(std::istream&,FitParameter&);
    std::istringstream input("bob");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 0.0 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test5()
	{
    FitParameter fitP;

    //operator>>(std::istream&,FitParameter&);
    std::istringstream input("9.1 ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

	void test6()
	{
    FitParameter fitP;

    //operator>>(std::istream&,FitParameter&);
    std::istringstream input("9.1         ");

    input >> fitP;

    TS_ASSERT_DELTA( fitP.getValue(), 9.1 ,0.0001);
    TS_ASSERT( fitP.getTie().compare("")==0 );
	}

};

#endif /*FITPARAMETERTEST_H_*/
