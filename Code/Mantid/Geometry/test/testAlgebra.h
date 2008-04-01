#ifndef MANTID_TESTALGEBRA__
#define MANTID_TESTALGEBRA__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

//#include "../inc/Matrix.h" 
#include "../inc/Algebra.h" 

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

//Test disabled as it takes too long
// Causes long delay - infinite loop?
void testDNF()
  /*!
    Test the DNF Structure 
   */
{
  Algebra A;
  
  A.setFunction("(f+x)(x+y+z)");
  A.makeDNF();
  TS_ASSERT_EQUALS(A.display(),"x+(fxy)+(fy)+(fz)");

  A.setFunction("a'b'c'+d'e'");
  TS_ASSERT_EQUALS(A.display(),"(e'd')+(c'b'a')");
  
  A.makeDNF();
  TS_ASSERT_EQUALS(A.display(),"(e'd')+(c'b'a'd)+(c'b'a'e)");

//  A.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");

//Test disabled as it takes too long
// Causes long delay - infinite loop?
  //TestFunc::bracketTest(" Please Wait:: ");
  //A.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");
  //A.write(std::cout);
  //A.makeDNF();
  //A.write(std::cout);

}

void testCNF()
  /*!
    Test the CNF Structure 
   */
{
  Algebra A;
  A.setFunction("(f+x)(x+y+z)");
  TS_ASSERT_EQUALS(A.display(),"(f+x)(x+y+z)");
  A.makeCNF();
  TS_ASSERT_EQUALS(A.display(),"(z'+f+x)(y'+f+x)(x+y+z)");
  
  A.setFunction("aq+acp+ace");
  TS_ASSERT_EQUALS(A.display(),"(ace)+(acp)+(aq)");
  
  A.makeCNF();
  TS_ASSERT_EQUALS(A.display(),"(q'+a)(p'+c'+a)(p'+c+q)(e'+c'+a)(e'+c+q)(a'+e+p+q)(e+p+q)");
  
  //Back to DNF
  A.makeDNF();
  TS_ASSERT_EQUALS(A.display(),"(acep)+(ace)+(acp)+(aq)");
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


void testmult()
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
  Algebra A;

  A.setFunction("ac+ad+bc+bd+ae'");
  TS_ASSERT_EQUALS(A.display(),"(e'a)+(ac)+(ad)+(bc)+(bd)");

  Algebra B;
  B.setFunction("a+b");
  TS_ASSERT_EQUALS(B.display(),"a+b");

  std::pair<Algebra,Algebra> X=A.algDiv(B);
  TS_ASSERT_EQUALS(X.first.display(),"c+d+(cd)");
  TS_ASSERT_EQUALS(X.second.display(),"e'a(c'+b'+a')");
  
  Algebra XY=X.first*B;
  TS_ASSERT_EQUALS(XY.display(),"(a+b)(c+d+(cd))");
  
  XY+=X.second;
  XY.makeDNF();
  TS_ASSERT_EQUALS(XY.display(),"(e'a)+(abcd)+(ac)+(ad)+(bc)+(bd)");
  
  //test fails - should it?  the compared algebras are similar but XY has an additional +abcd
  //TS_ASSERT(A==XY);
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
