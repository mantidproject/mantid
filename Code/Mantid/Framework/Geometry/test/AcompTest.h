#ifndef MANTID_ACOMPTEST__
#define MANTID_ACOMPTEST__
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/Acomp.h"

using namespace Mantid;
using namespace Geometry;


class AcompTest: public CxxTest::TestSuite{
public:
	void testDefaultConstructor(){
		Acomp A;
		TS_ASSERT_EQUALS(A.display(),"");
		TS_ASSERT_EQUALS(A.displayDepth(),"");
		TS_ASSERT_EQUALS(A.isNull(),1);
		TS_ASSERT_EQUALS(A.isSimple(),1);
		TS_ASSERT_EQUALS(A.isSingle(),1); //Return 1 if its 0 or 1 items
		TS_ASSERT_EQUALS(A.isDNF(),1); //Return 1 if its 0 items
		TS_ASSERT_EQUALS(A.isCNF(),1); //Return 1 if its 0 items
		TS_ASSERT_EQUALS(A.size().first,0);
		TS_ASSERT_EQUALS(A.size().second,0);
	}

	void testCreateComp(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		TS_ASSERT_EQUALS(A.displayDepth(),"D0 a\'bcd 0E+D0 aD1 (D2 cd 2E+D2 fD3 (x+y+z) 3E 2E) 1E 0E");
		TS_ASSERT_EQUALS(A.isNull(),0);
		TS_ASSERT_EQUALS(A.isSimple(),0);
		TS_ASSERT_EQUALS(A.isSingle(),0);
		TS_ASSERT_EQUALS(A.isDNF(),0);
		TS_ASSERT_EQUALS(A.isCNF(),0);
		TS_ASSERT_EQUALS(A.size().first,0);
		TS_ASSERT_EQUALS(A.size().second,2);
		std::map<int,int> literals;
		A.getLiterals(literals);
		TS_ASSERT_EQUALS(literals.size(),9);
		std::vector<int> keys;
		keys=A.getKeys();
		TS_ASSERT_EQUALS(keys.size(),8);
	}

	void testConstructor(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		TS_ASSERT_EQUALS(A.displayDepth(),"D0 a\'bcd 0E+D0 aD1 (D2 cd 2E+D2 fD3 (x+y+z) 3E 2E) 1E 0E");
		TS_ASSERT_EQUALS(A.isNull(),0);
		TS_ASSERT_EQUALS(A.isSimple(),0);
		TS_ASSERT_EQUALS(A.isSingle(),0);
		TS_ASSERT_EQUALS(A.isDNF(),0);
		TS_ASSERT_EQUALS(A.isCNF(),0);
		TS_ASSERT_EQUALS(A.size().first,0);
		TS_ASSERT_EQUALS(A.size().second,2);
		Acomp B(A);
		TS_ASSERT_EQUALS(B.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		TS_ASSERT_EQUALS(B.displayDepth(),"D0 a\'bcd 0E+D0 aD1 (D2 cd 2E+D2 fD3 (x+y+z) 3E 2E) 1E 0E");
		TS_ASSERT_EQUALS(B.isNull(),0);
		TS_ASSERT_EQUALS(B.isSimple(),0);
		TS_ASSERT_EQUALS(B.isSingle(),0);
		TS_ASSERT_EQUALS(B.isDNF(),0);
		TS_ASSERT_EQUALS(B.isCNF(),0);
		TS_ASSERT_EQUALS(B.size().first,0);
		TS_ASSERT_EQUALS(B.size().second,2);
	}

	void testAssignment(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		TS_ASSERT_EQUALS(A.displayDepth(),"D0 a\'bcd 0E+D0 aD1 (D2 cd 2E+D2 fD3 (x+y+z) 3E 2E) 1E 0E");
		TS_ASSERT_EQUALS(A.isNull(),0);
		TS_ASSERT_EQUALS(A.isSimple(),0);
		TS_ASSERT_EQUALS(A.isSingle(),0);
		TS_ASSERT_EQUALS(A.isDNF(),0);
		TS_ASSERT_EQUALS(A.isCNF(),0);
		TS_ASSERT_EQUALS(A.size().first,0);
		TS_ASSERT_EQUALS(A.size().second,2);
		Acomp B;
		B=A;
		TS_ASSERT_EQUALS(B.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		TS_ASSERT_EQUALS(B.displayDepth(),"D0 a\'bcd 0E+D0 aD1 (D2 cd 2E+D2 fD3 (x+y+z) 3E 2E) 1E 0E");
		TS_ASSERT_EQUALS(B.isNull(),0);
		TS_ASSERT_EQUALS(B.isSimple(),0);
		TS_ASSERT_EQUALS(B.isSingle(),0);
		TS_ASSERT_EQUALS(B.isDNF(),0);
		TS_ASSERT_EQUALS(B.isCNF(),0);
		TS_ASSERT_EQUALS(B.size().first,0);
		TS_ASSERT_EQUALS(B.size().second,2);
	}

	void testMakeDNFObject(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeDNFobject();
		TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
		TS_ASSERT_EQUALS(A.isDNF(),1);
		TS_ASSERT_EQUALS(A.isCNF(),0);
		TS_ASSERT_EQUALS(A.size().first,0);
		TS_ASSERT_EQUALS(A.size().second,5);
	}

	void testMakeCNFObject(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeCNFobject();
		TS_ASSERT_EQUALS(A.display(),"(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)(d+x+y+z)");
		TS_ASSERT_EQUALS(A.isCNF(),1);
		TS_ASSERT_EQUALS(A.isDNF(),0);
		TS_ASSERT_EQUALS(A.size().first,0);
		TS_ASSERT_EQUALS(A.size().second,7);
	}

	void testGetItemC(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeDNFobject();
		TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
		const Acomp *Result=A.itemC(0);
		TS_ASSERT_EQUALS(Result->display(),"acd");
		Result=A.itemC(1);
		TS_ASSERT_EQUALS(Result->display(),"afx");
		Result=A.itemC(2);
		TS_ASSERT_EQUALS(Result->display(),"afy");
		Result=A.itemC(3);
		TS_ASSERT_EQUALS(Result->display(),"afz");
		Result=A.itemC(4);
		TS_ASSERT_EQUALS(Result->display(),"bcd");
	}

	void testGetItemN(){ //There is no way of adding units?????
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeDNFobject();
		TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
	}

	void testComplement(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeDNFobject();
		TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
		A.complement();
		TS_ASSERT_EQUALS(A.display(),"(z\'+f\'+a\')(y\'+f\'+a\')(x\'+f\'+a\')(d\'+c\'+b\')(d\'+c\'+a\')");
	}

	void testLogicalEqual(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeDNFobject();
		TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
		Acomp B;
		B.setString("(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)(d+x+y+z)");
		TS_ASSERT_EQUALS(A.logicalEqual(B),1);
		Acomp C;
		C.setString("(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)");
		TS_ASSERT_EQUALS(A.logicalEqual(C),0);
	}

	void testComparatorOperators(){
		Acomp A;
		A.setString("a'bcd+a(cd+ff(x+y+z))");
		TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		A.makeDNFobject();
		TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
		Acomp B;
		B.setString("(acd)+(afx)+(afy)+(afz)+(bcd)");
		TS_ASSERT_EQUALS(A==B,true);
		Acomp C;
		C.setString("(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)");
		TS_ASSERT_EQUALS(A==C,false);
		TS_ASSERT_EQUALS(A!=C,true);
		TS_ASSERT_EQUALS(A!=B,false);

		TS_ASSERT_EQUALS(A>C,true);
		TS_ASSERT_EQUALS(C<B,true);
		TS_ASSERT_EQUALS(A<B,false);
	}
	
	void testIncrementOperators(){
		Acomp A;
		A.setString("(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)(d+x+y+z)");
		Acomp B;
		B.setString("c");
		A+=B;
		TS_ASSERT_EQUALS(A.display(),"c+((a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)(d+x+y+z))");
	}

	void testAlgDiv(){ //Problem with Algebric division
		//Acomp A;
		//A.setString("a'bcd+a(cd+ff(x+y+z))");
		//TS_ASSERT_EQUALS(A.display(),"(a\'bcd)+(a((cd)+(f(x+y+z))))");
		//A.makeDNFobject();
		//TS_ASSERT_EQUALS(A.display(),"(acd)+(afx)+(afy)+(afz)+(bcd)");
		//Acomp B;
		//B.setString("(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)(d+x+y+z)");
		//std::pair<Acomp,Acomp> result=A.algDiv(B);
		//TS_ASSERT_EQUALS(result.first.display(),"");
		//TS_ASSERT_EQUALS(result.second.display(),"");
		//Acomp C;
		//C.setString("(a+b)(a+c)(a+d)(c+f)(c+x+y+z)(d+f)");
		//result=A.algDiv(C);
		//TS_ASSERT_EQUALS(result.first.display(),"");
		//TS_ASSERT_EQUALS(result.second.display(),"");
	}
};
#endif
