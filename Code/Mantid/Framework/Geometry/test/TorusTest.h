#ifndef MANTID_TESTTORUS__
#define MANTID_TESTTORUS__

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Torus.h"

using namespace Mantid;
using namespace Geometry;

class TorusTest: public CxxTest::TestSuite
{

public:
	void testEmptyConstructor(){
		TS_ASSERT_THROWS( Torus A;, Mantid::Kernel::Exception::NotImplementedError )
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 0 0 0\n");
//		TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
//		TS_ASSERT_EQUALS(A.getNormal(),V3D(1.0,0.0,0.0));
	}

//	void testSetSurface(){
//		Torus A;
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 0 0 0\n");
//		TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
//		TS_ASSERT_EQUALS(A.getNormal(),V3D(1.0,0.0,0.0));
//		TS_ASSERT(A.setSurface("t/x 1 1 1 2 3 4")==0);
//		TS_ASSERT_EQUALS(A.getCentre(),V3D(1,1,1));
//		TS_ASSERT_EQUALS(A.getNormal(),V3D(1.0,0.0,0.0));
//	}
//
//	void testConstructorTorus(){
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 1 1 1 2 3 4"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [1,1,1] 2 3 4\n");
//		Torus B(A);
//		TS_ASSERT_EQUALS(extractString(B),"-1 tx [1,1,1] 2 3 4\n");
//	}
//
//	void testAssignment(){
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 1 1 1 2 3 4"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [1,1,1] 2 3 4\n");
//		Torus B;
//		B=A;
//		TS_ASSERT_EQUALS(extractString(B),"-1 tx [1,1,1] 2 3 4\n");		
//	}
//
//	void testClone(){
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 1 1 1 2 3 4"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [1,1,1] 2 3 4\n");
//		Torus *B;
//		B=A.clone();
//		TS_ASSERT_EQUALS(extractString(*B),"-1 tx [1,1,1] 2 3 4\n");
//	}
//
//	void testEquality(){
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 1 1 1 2 3 4"),0);
//		Torus B;
//		TS_ASSERT_EQUALS(B.setSurface("t/x 1 1 1 2 3 4"),0);
//		TS_ASSERT(A==B);
//	}
//
//	void testSide(){ //Not yet implemented
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 0 0 0 1 2 3"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 1 2 3\n");
//		
//
//	}
//	void testOnSurface() //Not yet implemented
//	{
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 0 0 0 1 2 3"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 1 2 3\n");
//
//	}
//
//	void testDistance(){ //The function is not implemented properly
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 0 0 0 1 2 3"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 1 2 3\n");
//
//	}
//
//	void testSurfaceNormal(){//This function is not implemented in the class
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 0 0 0 1 2 3"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 1 2 3\n");
//	}
//
//	void testSetCentre(){
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 0 0 0 1 2 3"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 1 2 3\n");
//		A.setCentre(V3D(1,1,1));
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [1,1,1] 1 2 3\n")
//	}
//
//	void testSetNorm(){
//		Torus A;
//		TS_ASSERT_EQUALS(A.setSurface("t/x 0 0 0 1 2 3"),0);
//		TS_ASSERT_EQUALS(extractString(A),"-1 tx [0,0,0] 1 2 3\n");
//		TS_ASSERT_EQUALS(A.getNormal(),V3D(1.0,0.0,0.0));
//		A.setNorm(V3D(0.0,1.0,0.0));
//		TS_ASSERT_EQUALS(extractString(A),"-1 ty [0,0,0] 1 2 3\n");
//		TS_ASSERT_EQUALS(A.getNormal(),V3D(0.0,1.0,0.0));		
//	}
//private:
//
//	std::string extractString(const Surface& pv)
//	{
//		//dump output to sting
//		std::ostringstream output;
//		output.exceptions( std::ios::failbit | std::ios::badbit );
//		TS_ASSERT_THROWS_NOTHING(pv.write(output));
//		return output.str();
//	}
};
#endif
