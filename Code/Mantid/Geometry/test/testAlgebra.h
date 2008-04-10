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
  // sum m(0,1,2,5, 6,7,8,9,10,14)
  std::string Func("(a'b'c'd')+(a'b'c'd)+(a'b'cd')+(a'bc'd)+(a'bcd')+(a'bcd)+(ab'c'd')+(ab'c'd)+(ab'cd')+(abcd')");
  A.setFunction(Func);
  //  A.write(std::cout);

  A.makeDNF();
  std::cout<<"A == "<<A.display()<<std::endl;
  TS_ASSERT_EQUALS("(d'c)+(c'b')+(a'bd)",A.display());

  // sum m(0,1,2,5,6,7)
  std::cout<<"New simple function"<<std::endl;
  std::string FuncA("(a'b'c')+(a'b'c)+(a'bc')+(ab'c)+(abc')+(abc)");
  A.setFunction(FuncA);
  //  A.write(std::cout);

  A.makeDNF();
  TS_ASSERT_EQUALS("(c'a')+(b'c)+(ab)",A.display());

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
  Algebra A;

  A.setFunction("ac+ad+bc+bd+ae'");
  TS_ASSERT_EQUALS(A.display(),"(e'a)+(ac)+(ad)+(bc)+(bd)");

  Algebra B;
  B.setFunction("a+b");
  TS_ASSERT_EQUALS(B.display(),"a+b");

  std::pair<Algebra,Algebra> X=A.algDiv(B);
  TS_ASSERT_EQUALS(X.first.display(),"c+d");
  TS_ASSERT_EQUALS(X.second.display(),"e'a");
  
  // NOW CHECK that multiplication of divisor * factor + remainder 
  // is the same
  Algebra XY=X.first*B;
  XY+=X.second;
  TS_ASSERT_EQUALS(A,XY);

  TS_ASSERT_EQUALS(XY.display(),"(e'a)+((a+b)(c+d))");

  XY.makeDNF();

  TS_ASSERT_EQUALS(A,XY);
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
