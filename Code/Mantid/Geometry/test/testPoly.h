#ifndef MANTID_TESTPOLY__
#define MANTID_TESTPOLY__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "../inc/Matrix.h" 
#include "../inc/Matrix.h" 

using namespace Mantid;
using namespace Geometry;

class testPoly: public CxxTest::TestSuite
{

public:

void testAddition()
  /*!
    Test the SetComponent to make an interesting function
  */ 
{
  PolyVar<3> FXY(2);
  FXY.setComp(1,4.0);
  FXY.setComp(2,3.0);
  std::cout<<"FXY:"<<FXY<<std::endl;

  PolyVar<2> GXY(2);
  GXY.setComp(1,5.3);
  GXY.setComp(2,2.2);
  std::cout<<"GXY:"<<GXY<<std::endl;
  
  PolyVar<3> HXY;
  std::cout<<"HXY Null ="<<HXY<<std::endl;
  HXY=FXY.operator+(GXY);
  std::cout<<"HXY = "<<HXY<<std::endl;
  return;
}

void testBase()
  /*!
    Test the distance of a point from the cone
    \retval -1 :: failed build a cone
    \retval 0 :: All passed
  */
{

  return;
}


void testBezout()
  /*!
    Test the distance of a point from the cone
    \retval -1 :: Vailes to process PolyVar
    \retval 0 :: All passed
  */ 
{
  PolyVar<2> FXY; 
  PolyVar<2> GXY; 

  // Setting by variable:

  FXY.read("y+x^2+x+1");
  GXY.read("xy+2x+5");
  std::cout<<"GXY "<<GXY<<std::endl;
  PolyVar<1> Out=FXY.reduce(GXY);
  
  std::cerr<<"Out ="<<Out<<std::endl;
  

  return;
}

void testEqualTemplate()
  /*!
    Test operator=
  */ 
{
  PolyVar<3> GXYZ(2); 
  PolyVar<1> FX(2);

  // Setting by variable:
  std::vector<double> C;
  C.push_back(1.1);
  C.push_back(2.2);
  C.push_back(3.3);
  (std::vector<double>&) FX=C;

  std::cout<<"FX == "<<FX<<std::endl;
  GXYZ=FX;
  std::cout<<"GXYZ == "<<GXYZ<<std::endl;
  return;
}

void testGetMaxSize()
  /*!
    Test of the getMaxSize routine
   */
{
  std::string Line="y+(x+3)y^3+(x+x^5)z^5+3.0y^2";
  const int Index=PolyFunction::getMaxSize(Line,'y');
  
  if (Index!=3)
    {
      std::cout<<"Index "<<Index<<std::endl;
      TS_ASSERT(-1==0);
    }

  return;
}

void testMultiplication()
  /*!
    Test the SetComponent to make an interesting function
    \retval 0 :: All passed
  */ 
{
  std::stringstream cx;
  PolyVar<1> FX;
  PolyVar<1> GX;
  PolyVar<1> HX;
  PolyVar<2> FXY;
  PolyVar<2> GXY;
  PolyVar<2> HXY;

  // SINGLE
  FX.read("x^2+5x+3");
  GX.read("x-2");
  HX=FX*GX;
  cx<<HX;
  if (cx.str()!="3x^2-7x-6")
    {
      std::cout<<"SINGLE MULT"<<std::endl;
      std::cout<<FX<<std::endl;
      std::cout<<GX<<std::endl;
      std::cout<<HX<<std::endl;
      TS_ASSERT(-1==0);
    }

  // DOUBLE
  cx.str("");
  FXY.read("x^2+5x+3");
  GXY.read("y-2");
  HXY=FXY*GXY;
  cx<<HXY;
  if (cx.str()!="(x^2+5x+3)y-2x^2-10x-6")
    {
      std::cout<<"Double MULT"<<std::endl;
      std::cout<<FXY<<std::endl;
      std::cout<<GXY<<std::endl;
      std::cout<<HXY<<std::endl;
      TS_ASSERT(-1==0);
    }

  // MORE DOUBLE [x+y and a zero sum]
  cx.str("");
  FXY.read("xy+x^2");
  GXY.read("y^2+y-1");
  HXY=FXY*GXY;
  cx<<HXY;
  if (cx.str()!="xy^3+(x^2+x)y^2+(x^2-x)y-x^2")
    {
      std::cout<<"Double MULT"<<std::endl;
      std::cout<<FXY<<std::endl;
      std::cout<<GXY<<std::endl;
      std::cout<<HXY<<std::endl;
      TS_ASSERT(-1==0);
    }



  return;
}

void testRead()
  /*!
    Test the read function
    \retval 0 :: scucess
   */
{
  std::stringstream cx;
  std::vector<std::string> TLine;
  std::vector<std::string> OLine;
  std::vector<double> Value;
  TLine.push_back("-1.0x^3-x+3.4");
  TLine.push_back("3y^2-6");
  TLine.push_back("(x^3+3.4)y^2-(x^4+3)y+x^2+6"); 
  TLine.push_back("-(x^3-3.4)y^2-y-x^2+6");       
  TLine.push_back("-1.0y-x^2+6");   

  TLine.push_back("z^2+xyz-1.0y-x^2+6");   
  TLine.push_back("z^2+y^2z-y-x^2+6");   

  OLine.push_back("-x^3-x+3.4");
  OLine.push_back("3y^2-6");
  OLine.push_back("(x^3+3.4)y^2+(-x^4-3)y+x^2+6"); 
  OLine.push_back("(-x^3+3.4)y^2-y-x^2+6");       
  OLine.push_back("-y-x^2+6");   

  OLine.push_back("z^2+xyz-y-x^2+6");   
  OLine.push_back("z^2+y^2z-y-x^2+6");   

  Value.push_back(-6.6);
  Value.push_back(21.0);
  Value.push_back(55.6);
  Value.push_back(-42.4);
  Value.push_back(-1.0);
  Value.push_back(39);
  Value.push_back(51);

  PolyVar<1> FX;
  PolyVar<2> GXY;
  PolyVar<3> HXYZ;
  // Test single value
  FX.read(TLine[0]);
  cx<<FX;
  if (cx.str()!=OLine[0] || FX(2.0)!=Value[0])
    {
      TestFunc::bracketTest("SINGLE",std::cout);
      std::cout<<"Input : "<<TLine[0]<<std::endl;
      std::cout<<"FX    : "<<FX<<std::endl;
      std::cout<<"Eval == "<<FX(2.0)<<std::endl;
      std::cout<<"Expect: "<<OLine[0]<<std::endl;
      TS_ASSERT(-1==0);
    }

  int i;
  for(i=1;i<5;i++)
    {
      cx.str("");
      const int flag=GXY.read(TLine[i]);
      cx<<GXY;
      double Val[]={2.0,3.0};
      
      if (cx.str()!=OLine[i] || fabs(GXY(Val)-Value[i])>1e-3 || flag)
        {
	  TestFunc::bracketTest("DOUBLE",std::cout);
	  std::cout<<"Flag  : "<<flag<<std::endl;
	  std::cout<<"Input  : "<<TLine[i]<<std::endl;
	  std::cout<<"GXY    : "<<GXY<<std::endl;
	  std::cout<<"Eval ==  "<<GXY(Val)<<" ("<<Value[i]<<")"<<std::endl;
	  std::cout<<"Expect : "<<OLine[i]<<std::endl;
	  TS_ASSERT(-1==0);
	}
    }
  i++;
  for(;i<7;i++)
    {

      TestFunc::bracketTest("TRIPLE",std::cout);

      cx.str("");
      const int flag=HXYZ.read(TLine[i]);
      cx<<HXYZ;
      double Val[]={2.0,3.0,4.0};
      if (cx.str()!=OLine[i] || HXYZ(Val)!=Value[i] || flag)
        {
	  std::cout<<"Flag  : "<<flag<<std::endl;
	  std::cout<<"Input  : "<<TLine[i]<<std::endl;
	  std::cout<<"HXYZ   : "<<HXYZ<<std::endl;
	  std::cout<<"Eval ==  "<<HXYZ(Val)<<" ("<<Value[i]<<")"<<std::endl;
	  std::cout<<"Expect : "<<OLine[i]<<std::endl;
	  TS_ASSERT(-1==0);
	}
    }

  return;
}

void testSetComp()
  /*!
    Test the SetComponent to make an interesting function

  */ 
{
  PolyVar<3> GXYZ(3);
  std::cout<<GXYZ<<std::endl;
  GXYZ.setComp(1,4.0);
  GXYZ.setComp(2,3.0);
  std::cout<<GXYZ<<std::endl;
  return;
}
  
void testVariable()
  /*!
    Test the distance of a point from the cone
  */ 
{
  PolyVar<2> FXY(2); 
  PolyVar<3> GXYZ(2); 
  PolyVar<1> XValue(2);           // 

  // Setting by variable:
  std::vector<double> C;
  C.push_back(1.1);
  C.push_back(2.2);
  C.push_back(3.3);
  (std::vector<double>&) XValue=C;
  std::cout<<"XValue == "<<XValue<<std::endl;

  FXY.setComp(1,XValue);

  std::cout<<"FXY == "<<FXY<<std::endl;
  PolyFunction* Gptr=&FXY;
  Gptr->operator+=(4.4);
  std::cout<<FXY<<std::endl;
  
  return;
}
  

};

#endif
