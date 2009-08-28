#ifndef MANTID_TESTALGEBRA__
#define MANTID_TESTALGEBRA__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

//#include "MantidGeometry/Math/Matrix.h" 
#include "MantidGeometry/Math/Algebra.h" 

using namespace Mantid::Geometry;

class testAlgebra: public CxxTest::TestSuite
{

public:

void testcountLiterals() 
  /*!
    Test the number of literals in a string.
    Single test includes not.
   */
{ 
  Algebra A;

  A.setFunction("a'bcd+a(cd+ff(x+y+z))");
  int NLiteral=A.countLiterals();
  TS_ASSERT_EQUALS(NLiteral,9);
  return;
}

void testDNF()
  /*! 
    Test the DNF Structure 
   */
{

  Algebra A,B;
  std::vector<std::string> Func;
  Func.push_back("(a'b'c'd')+(a'b'c'd)+(a'b'cd')+(a'bc'd)+(a'bcd')+(a'bcd)+(ab'c'd')+(ab'c'd)+(ab'cd')+(abcd')");
  Func.push_back("(a'b'c')+(a'b'c)+(a'bc')+(ab'c)+(abc')+(abc)");
  Func.push_back("a'b'c'+d'e'");
  // This test takes about 20 second on an old PC. [Note: the g' : g  cyclic problem]
  //Func.push_back("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");

  std::vector<std::string>::const_iterator sv;
  for(sv=Func.begin();sv!=Func.end();sv++)
    {
      A.setFunction(*sv);
      B.setFunction(*sv);
      A.makeDNF();
      TS_ASSERT(A.logicalEqual(B))
      TS_ASSERT(A.getComp().isDNF())
    }
}


void testCNF()
  /*!
    Test the CNF Structure 
  */
{
  Algebra A;
  A.setFunction("(f+x)(x+y+z)");
  // This test sees that the function is in CNF & is 
  // obviously minimal. Hence it skips the CNF factoring
  // Change one of the x literals to x' and the same result
  // will occur BUT the program must factor. 
  TS_ASSERT_EQUALS(A.display(),"(f+x)(x+y+z)");
  A.makeCNF();
  TS_ASSERT_EQUALS(A.display(),"(f+x)(x+y+z)");
  
  // Start with a DNF form wiht common factor:
  A.setFunction("aq+acp+ace");  
  TS_ASSERT_EQUALS(A.display(),"(ace)+(acp)+(aq)");
  A.makeCNF();
  // Note: there are several possibles here:
  // This expands in DNF to a(ce+cp+cq+qe+qp+q):
  //                     :==  ac(e+p+q)+q
  //                     :==  ace+acp+aq
  TS_ASSERT_EQUALS(A.display(),"a(c+q)(e+p+q)")
  
  //Back to DNF (were we started)
  A.makeDNF();
  TS_ASSERT_EQUALS(A.display(),"(ace)+(acp)+(aq)")
  return;
}

void testAdditions()
  /*!
    add two different algebra's by union
  */
{
  Algebra A;
  A.setFunction("a'bcd+a(cd+ff(x+y+z))");
  Algebra B;
  B.setFunction("jxyzi(ad+sw)");
  B+=A;
  
  TS_ASSERT_EQUALS(B.display(),"(a'bcd)+(a((cd)+(f(x+y+z))))+(ijxyz((ad)+(sw)))");
}

void testmakeString()
  /*!
    Process algebra from a string
  */
{
  Algebra A;
  A.setFunction("a'bcd+a(cd+ff(x+y+z))");
  TS_ASSERT_EQUALS(A.display(),"(a'bcd)+(a((cd)+(f(x+y+z))))");

  A.Complement();
  TS_ASSERT_EQUALS(A.display(),"(d'+c'+b'+a)(a'+((f'+(z'y'x'))(d'+c')))");
  A.Complement();
  TS_ASSERT_EQUALS(A.display(),"(a'bcd)+(a((cd)+(f(x+y+z))))");
}


void testMult()
  /*!
    Test algebraic multiplication

  */
{
  Algebra A;
  A.setFunction("a+b'+(c)");
  TS_ASSERT_EQUALS(A.display(),"b'+a+c");


  Algebra B;
  B.setFunction("a+b");
  TS_ASSERT_EQUALS(B.display(),"a+b");
  
  A*=B;
  TS_ASSERT_EQUALS(A.display(),"(b'+a+c)(a+b)");
 
  Algebra C;
  C.setFunction("(a+b)(a+c+b')");
  TS_ASSERT_EQUALS(C.display(),"(b'+a+c)(a+b)");
  TS_ASSERT(C==A);
}  

void testWeakDiv()
  /*!
    Test weak division algorithm
  */
{

  Algebra F;
  F.setFunction("ad+abc+bcd");
  Algebra P;
  P.setFunction("a+bc");
  std::pair<Algebra,Algebra> X=F.algDiv(P);
  TS_ASSERT_EQUALS(X.first.display(),"d");
  TS_ASSERT_EQUALS(X.second.display(),"abc");
  //Multiply back up
  Algebra XY=X.first*P;
  XY+=X.second; 
  TS_ASSERT_EQUALS(XY.logicalEqual(F),1)
  XY.makeDNF();
  TS_ASSERT_EQUALS(F,XY)
}

void testComplementary()
  /*!
    Test the complementary rule:
   */
{
  Algebra A;

  A.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");
  TS_ASSERT_EQUALS(A.display(),"ab((j'g(l'+k)(m+n))+(i'h'g'c'(f'+d+e)))");
  A.Complement();
  TS_ASSERT_EQUALS(A.display(),"b'+a'+((g'+j+(n'm')+(k'l))(c+g+h+i+(e'd'f)))");
}


};

#endif
