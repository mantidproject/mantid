#ifndef MANTID_TESTPOLY__
#define MANTID_TESTPOLY__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidGeometry/Math/PolyVar.h" 
#include "MantidGeometry/Math/Matrix.h" 

using namespace Mantid;
using namespace Geometry;
using namespace mathLevel;

class PolyTest: public CxxTest::TestSuite
{

public:

  void testAddition()
    /**
    Test the SetComponent to make an interesting function
    */ 
  {
    PolyVar<3> FXY(2);
    FXY.setComp(1,4.0);
    FXY.setComp(2,3.0);
    TS_ASSERT_EQUALS(extractString(FXY),"3z^2+4z");

    PolyVar<2> GXY(2);
    GXY.setComp(1,5.3);
    GXY.setComp(2,2.2);
    TS_ASSERT_EQUALS(extractString(GXY),"2.2y^2+5.3y");

    PolyVar<3> HXY;
    TS_ASSERT_EQUALS(extractString(HXY),"0");
    HXY=FXY.operator+(GXY);
    TS_ASSERT_EQUALS(extractString(HXY),"3z^2+4z+2.2y^2+5.3y");
  }


  void testBezout() 
  {
    PolyVar<2> FXY; 
    PolyVar<2> GXY; 

    // Setting by variable:

    FXY.read("y+x^2+x+1");
    TS_ASSERT_EQUALS(extractString(FXY),"y+x^2+x+1");
    GXY.read("xy+2x+5");
    TS_ASSERT_EQUALS(extractString(GXY),"xy+2x+5");
    PolyVar<1> Out=FXY.reduce(GXY);
    TS_ASSERT_EQUALS(extractString(Out),"x+5");
  }

  void testEqualTemplate()
    /**
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

    TS_ASSERT_EQUALS(extractString(FX),"3.3x^2+2.2x+1.1");
    GXYZ=FX;
    TS_ASSERT_EQUALS(extractString(GXYZ),"3.3x^2+2.2x+1.1");
    return;
  }

  void testGetMaxSize()
    /**
    Test of the getMaxSize routine
    */
  {
    std::string Line="y+(x+3)y^3+(x+x^5)z^5+3.0y^2";
    const int Index=PolyFunction::getMaxSize(Line,'y');
    TS_ASSERT_EQUALS(Index,3)
  }

  void testMultiplication()
    /**
    Test the SetComponent to make an interesting function
    */ 
  {
    PolyVar<1> FX;
    PolyVar<1> GX;
    PolyVar<1> HX;
    PolyVar<2> FXY;
    PolyVar<2> GXY;
    PolyVar<2> HXY;

    // SINGLE
    FX.read("x^2+5x+3");
    TS_ASSERT_EQUALS(extractString(FX),"x^2+5x+3");
    GX.read("x-2");
    TS_ASSERT_EQUALS(extractString(GX),"x-2");
    HX=FX*GX;
    TS_ASSERT_EQUALS(extractString(HX),"3x^2-7x-6");


    // DOUBLE
    FXY.read("x^2+5x+3");
    TS_ASSERT_EQUALS(extractString(FXY),"x^2+5x+3");
    GXY.read("y-2");
    TS_ASSERT_EQUALS(extractString(GXY),"y-2");
    HXY=FXY*GXY;
    TS_ASSERT_EQUALS(extractString(HXY),"(x^2+5x+3)y-2x^2-10x-6");

    // MORE DOUBLE [x+y and a zero sum]
    FXY.read("xy+x^2");
    TS_ASSERT_EQUALS(extractString(FXY),"xy+x^2");
    GXY.read("y^2+y-1");
    TS_ASSERT_EQUALS(extractString(GXY),"y^2+y-1");
    HXY=FXY*GXY;
    TS_ASSERT_EQUALS(extractString(HXY),"xy^3+(x^2+x)y^2+(x^2-x)y-x^2");
  }

  void testRead()
    /**
    Test the read function
    */
  {
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
    TS_ASSERT_EQUALS(extractString(FX),OLine[0]);
    TS_ASSERT_EQUALS(FX(2.0),Value[0]);

    for(int i=1;i<5;i++)
    {
      const int flag=GXY.read(TLine[i]);
      TS_ASSERT_EQUALS(flag,0);
      TS_ASSERT_EQUALS(extractString(GXY),OLine[i]);

      double Val[]={2.0,3.0};
      TS_ASSERT_DELTA(GXY(Val),Value[i], 0.001);
    }

    for(int i=6;i<7;i++)
    {
      const int flag=HXYZ.read(TLine[i]);
      TS_ASSERT_EQUALS(flag,0);
      TS_ASSERT_EQUALS(extractString(HXYZ),OLine[i]);
      double Val[]={2.0,3.0,4.0};
      TS_ASSERT_DELTA(HXYZ(Val),Value[i], 0.001);
    }
  }

  void testSetComp()
    /**
    Test the SetComponent to make an interesting function
    */ 
  {
    PolyVar<3> GXYZ(3);
    TS_ASSERT_EQUALS(extractString(GXYZ),"0");
    GXYZ.setComp(1,4.0);
    GXYZ.setComp(2,3.0);
    TS_ASSERT_EQUALS(extractString(GXYZ),"3z^2+4z");
  }

  void testVariable()
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
    TS_ASSERT_EQUALS(extractString(XValue),"3.3x^2+2.2x+1.1");

    FXY.setComp(1,XValue);

    TS_ASSERT_EQUALS(extractString(FXY),"(3.3x^2+2.2x+1.1)y");
    PolyFunction* Gptr=&FXY;
    Gptr->operator+=(4.4);
    TS_ASSERT_EQUALS(extractString(FXY),"(3.3x^2+2.2x+1.1)y+4.4");
  }

private:

  template<int VCount>
  std::string extractString(PolyVar<VCount>& pv)
  {
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(output << pv);
    return output.str();
  }

};

#endif
