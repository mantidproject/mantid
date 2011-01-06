#ifndef MANTID_TESTTRIPLE__
#define MANTID_TESTTRIPLE__

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Math/Triple.h"

using namespace Mantid;

class TripleTest: public CxxTest::TestSuite
{

public:
	void testEmptyConstructor(){
		Triple<int> A;
		TS_ASSERT_EQUALS(A[0],0);
		TS_ASSERT_EQUALS(A[1],0);
		TS_ASSERT_EQUALS(A[2],0);
	}

	void testDefaultConstructor(){
		Triple<int> A(1,2,3);
		TS_ASSERT_EQUALS(A[0],1);
		TS_ASSERT_EQUALS(A[1],2);
		TS_ASSERT_EQUALS(A[2],3);
	}
	
	void testTripleConstructor(){
		Triple<int> A(1,2,3);
		Triple<int> B(A);
		TS_ASSERT_EQUALS(B[0],1);
		TS_ASSERT_EQUALS(B[1],2);
		TS_ASSERT_EQUALS(B[2],3);
	}

	void testAssignment(){
		Triple<int> A(1,2,3);
		Triple<int> B;
		TS_ASSERT_EQUALS(B[0],0);
		TS_ASSERT_EQUALS(B[1],0);
		TS_ASSERT_EQUALS(B[2],0);
		B=A;
		TS_ASSERT_EQUALS(B[0],1);
		TS_ASSERT_EQUALS(B[1],2);
		TS_ASSERT_EQUALS(B[2],3);
	}

	void testLessthan(){
		Triple<int> A(1,2,3);
		Triple<int> B(0,1,2);
		TS_ASSERT_EQUALS(A<B,0);
		TS_ASSERT_EQUALS(B<A,1);
	}

	void testGreaterThan(){
		Triple<int> A(1,2,3);
		Triple<int> B(0,1,2);
		TS_ASSERT_EQUALS(A>B,1);
		TS_ASSERT_EQUALS(B>A,0);
	}

	void testEquality(){
		Triple<int> A(1,2,3);
		Triple<int> B(0,1,2);
		Triple<int> C(1,2,3);
		TS_ASSERT_EQUALS(A==B,0);
		TS_ASSERT_EQUALS(A==C,1);
	}

	void testDTriple(){
		DTriple<int,int,std::string> A,B;
		TS_ASSERT_EQUALS(A==B,1);
		TS_ASSERT_EQUALS(A<B,0);
		TS_ASSERT_EQUALS(A>B,0);
		DTriple<int,int,std::string> C(1,2,"test");
		A=C;
		TS_ASSERT_EQUALS(A==C,1);
		TS_ASSERT_EQUALS(A==B,0);
		TS_ASSERT_EQUALS(A<B,0);
		TS_ASSERT_EQUALS(A>B,1);
		DTriple<int,int,std::string> D(2,3,"rest");
		B=D;
		TS_ASSERT_EQUALS(B==D,1);
		TS_ASSERT_EQUALS(A==B,0);
		TS_ASSERT_EQUALS(A<B,1);
		TS_ASSERT_EQUALS(A>B,0);
	}

};

#endif
