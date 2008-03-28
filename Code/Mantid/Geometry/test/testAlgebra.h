#ifndef MANTID_TESTALGEBRA__
#define MANTID_TESTALGEBRA__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "../inc/Matrix.h" 

using namespace Mantid;
using namespace Geometry;

class testMatrix: public CxxTest::TestSuite
{

public:


void testcountLiterals()
  /*!
    Test the number of literals in a string.
    Single test includes not.
   */
{
  A.setFunction("a'bcd+a(cd+ff(x+y+z))");
  A.write(std::cerr);
  int NLiteral=A.countLiterals();
  TS_ASSERT_EQUAL(NLiteral,9);
  return;
}

void testDNF()
  /*!
    Test teh DNF Structure 
   */
{
  Algebra A;
  
  A.setFunction("(f+x)(x+y+z)");
  A.write(std::cout);

  std::cout<<"DNF[(f+x)(x+y+z)] == ";
  A.makeDNF();
  A.write(std::cout);

  std::cout<<"New simple function"<<std::endl;
  A.setFunction("a'b'c'+d'e'");
  A.write(std::cout);
  std::cout<<"DNF[a'b'c'+d'e'] == "<<std::endl;
  A.makeDNF();
  A.write(std::cout);
  std::cout<<std::endl;
//  A.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");

  TestFunc::bracketTest(" Please Wait:: ");
  A.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");
  A.write(std::cout);
  A.makeDNF();
  A.write(std::cout);
  return;
}

void testCNF()
  /*!
    Test the CNF Structure 
   */
{
  Algebra A;
  A.setFunction("(f+x)(x+y+z)");
  A.write(std::cout);

  std::cout<<"CNF[(f+x)(x+y+z)] == "<<std::endl;
  A.makeCNF();
  A.write(std::cout);

  std::cout<<"New simple function"<<std::endl;
  std::string Tfunc="aq+acp+ace";
  A.setFunction(Tfunc);
  A.write(std::cout);
  std::cout<<"CNF["<<Tfunc<<"] == "<<std::endl;
  A.makeCNF();
  A.write(std::cout);
  std::cout<<"Back to DNF"<<std::endl;
  A.makeDNF();
  A.write(std::cout);
  return;
}

void testAdditions()
  /*!
    add two different algebra's by union
  */
{
  std::cout<<"Test Additions "<<std::endl;
  Algebra A;
  A.setFunction("a'bcd+a(cd+ff(x+y+z))");
  Algebra B;
  B.setFunction("jxyzi(ad+sw)");
  B+=A;
  B.write(std::cout);
  std::cout<<"---- End of Additions "<<std::endl<<std::endl;
  return;
}

void testmakeString()
  /*!
    Process algebra from a string
  */
{
  Algebra A;
  A.setFunction("a'bcd+a(cd+ff(x+y+z))");
  //  A.setFunction("(cd+ff)+b");
  A.write(std::cout);
  A.Complement();
  A.write(std::cout);
  A.Complement();
  A.write(std::cout);
  return;
}


void testmult()
  /*!
    Test algebraic multiplication

  */
{
  Algebra A;
  std::cout<<"A[a+b'+(c)] :: ";
  A.setFunction("a+b'+(c)");
  A.write(std::cout);
  std::cout<<std::endl;

  Algebra B;
  B.setFunction("a+b");
  std::cout<<"B[a+b] == ";
  B.write(std::cout);
  std::cout<<std::endl;
  A*=B;
  std::cout<<"A*B == "<<std::endl;
  A.write(std::cout);
  std::cout<<std::endl;

  Algebra C;
  C.setFunction("(a+b)(a+c+b')");
  C.write(std::cout);
  TS_ASSERT(C==A);

}  

void testWeakDiv()
  /*!
    Test weak division algorithm
  */
{
  Algebra A;
  std::cout<<"A == ";
//  A.setFunction("a'cd+afc+afy+fz+abc");
  A.setFunction("ac+ad+bc+bd+ae'");
  std::cout<<std::endl;

  Algebra B;
  B.setFunction("a+b");
  std::cout<<"B == ";
  B.write(std::cout);
  std::cout<<std::endl;

  std::pair<Algebra,Algebra> X=A.algDiv(B);
  std::cout<<"A == "<<std::endl;
  X.first.write(std::cout);
  X.second.write(std::cout);
  Algebra XY=X.first*B;
  XY.write(std::cout);
  XY+=X.second;
  XY.makeDNF();
  XY.write(std::cout);
  TS_ASSERT(A==XY);
  return;
}

void testComplementary()
  /*!
    Test the complementary rule:
   */
{
  Algebra A;
  // 110  -125  ((-212  (105:107:-106)  -217  -201 -200)
  //                     : (217  -227  (122:-123)  (220 : 219)))

  A.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");
  A.write(std::cout);
  A.Complement();
  A.write(std::cout);
  A.makeDNF();
  
  Algebra B;
  B.setFunction("ab((c'(d+e+f')g'h'i')+(gj'(k+l')(m+n)))");
  B.makeDNF();
  B.Complement();
  B.write(std::cout);

  return;
}


};

#endif
