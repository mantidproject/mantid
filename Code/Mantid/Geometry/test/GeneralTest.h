#ifndef MANTID_GENERALTEST__
#define MANTID_GENERALTEST__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/General.h"

using namespace Mantid;
using namespace Geometry;


class testGeneral: public CxxTest::TestSuite
{
public:
	void testConstructor(){
		General A;
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
	}

	void testSetSurface(){
		General A;
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
		A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1"); // A Sphere equation
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n"); 
	}

	void testConstructorGeneral(){
		General A;
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
		A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1");
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
		General B(A);
		TS_ASSERT_EQUALS(extractString(B),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");		
	}

	void testClone(){
		General A;
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
		A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1");
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
		General *B;
		B=A.clone();
		TS_ASSERT_EQUALS(extractString(*B),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
		delete B;
	}

	void testEqualOperator(){
		General A;
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
		A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1");
		TS_ASSERT_EQUALS(extractString(A),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
		General B;
		TS_ASSERT_EQUALS(extractString(B),"-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
		B=A;
		TS_ASSERT_EQUALS(extractString(B),"-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
	}
private:

std::string extractString(const Surface& pv)
  {
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }

};
#endif
