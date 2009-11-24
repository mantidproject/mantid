#ifndef IFUNCTIONTEST_H_
#define IFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Function.h"

using namespace Mantid;
using namespace Mantid::API;

class IFT_Funct: public Function
{
public:
  IFT_Funct()
  {
    declareParameter("c0");
    declareParameter("c1");
    declareParameter("c2");
    declareParameter("c3");
  }

  std::string name()const{return "IFT_Funct";}

  void function(double* out, const double* xValues, const int& nData)
  {
    double c0 = getParameter("c0");
    double c1 = getParameter("c1");
    double c2 = getParameter("c2");
    double c3 = getParameter("c3");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i];
      out[i] = c0 + x*(c1 + x*(c2 + x*c3));
    }
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i];
      out->set(i,0,1.);
      out->set(i,1,x);
      out->set(i,2,x*x);
      out->set(i,3,x*x*x);
    }
  }

};

class IFunctionTest : public CxxTest::TestSuite
{
public:

  void testIFunction()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),4);

    TS_ASSERT_EQUALS(f.parameter(0),1.0);
    TS_ASSERT_EQUALS(f.parameter(1),1.1);
    TS_ASSERT_EQUALS(f.parameter(2),1.2);
    TS_ASSERT_EQUALS(f.parameter(3),1.3);

    TS_ASSERT_EQUALS(f.parameterName(0),"c0");
    TS_ASSERT_EQUALS(f.parameterName(1),"c1");
    TS_ASSERT_EQUALS(f.parameterName(2),"c2");
    TS_ASSERT_EQUALS(f.parameterName(3),"c3");

    TS_ASSERT_EQUALS(f.getParameter("c0"),1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),1.1);
    TS_ASSERT_EQUALS(f.getParameter("c2"),1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),1.3);

    TS_ASSERT_EQUALS(f.parameterIndex("c0"),0);
    TS_ASSERT_EQUALS(f.parameterIndex("c1"),1);
    TS_ASSERT_EQUALS(f.parameterIndex("c2"),2);
    TS_ASSERT_EQUALS(f.parameterIndex("c3"),3);

    std::string str = "name=IFT_Funct,c0=1,c1=1.1,c2=1.2,c3=1.3";

    TS_ASSERT_EQUALS(f.asString(),str);

    TS_ASSERT_EQUALS(f.activeParameter(0),1.0);
    TS_ASSERT_EQUALS(f.activeParameter(1),1.1);
    TS_ASSERT_EQUALS(f.activeParameter(2),1.2);
    TS_ASSERT_EQUALS(f.activeParameter(3),1.3);

    TS_ASSERT_EQUALS(f.nameOfActive(0),"c0");
    TS_ASSERT_EQUALS(f.nameOfActive(1),"c1");
    TS_ASSERT_EQUALS(f.nameOfActive(2),"c2");
    TS_ASSERT_EQUALS(f.nameOfActive(3),"c3");

    TS_ASSERT_EQUALS(f.indexOfActive(0),0);
    TS_ASSERT_EQUALS(f.indexOfActive(1),1);
    TS_ASSERT_EQUALS(f.indexOfActive(2),2);
    TS_ASSERT_EQUALS(f.indexOfActive(3),3);

    TS_ASSERT(   f.isActive(0));
    TS_ASSERT(   f.isActive(1));
    TS_ASSERT(   f.isActive(2));
    TS_ASSERT(   f.isActive(3));

    TS_ASSERT_EQUALS(f.activeIndex(0),0);
    TS_ASSERT_EQUALS(f.activeIndex(1),1);
    TS_ASSERT_EQUALS(f.activeIndex(2),2);
    TS_ASSERT_EQUALS(f.activeIndex(3),3);

  }

  void testRemoveActive()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.removeActive(1);
    f.removeActive(3);

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    TS_ASSERT_EQUALS(f.activeParameter(0),1.0);
    TS_ASSERT_EQUALS(f.activeParameter(1),1.2);

    TS_ASSERT_EQUALS(f.nameOfActive(0),"c0");
    TS_ASSERT_EQUALS(f.nameOfActive(1),"c2");

    TS_ASSERT_EQUALS(f.indexOfActive(0),0);
    TS_ASSERT_EQUALS(f.indexOfActive(1),2);

    TS_ASSERT(   f.isActive(0));
    TS_ASSERT( ! f.isActive(1));
    TS_ASSERT(   f.isActive(2));
    TS_ASSERT( ! f.isActive(3));

    TS_ASSERT_EQUALS(f.activeIndex(0),0);
    TS_ASSERT_EQUALS(f.activeIndex(1),-1);
    TS_ASSERT_EQUALS(f.activeIndex(2),1);
    TS_ASSERT_EQUALS(f.activeIndex(3),-1);

  }

  void testRestoreActive()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.removeActive(1);
    f.removeActive(3);

    f.restoreActive(3);

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),3);

    TS_ASSERT_EQUALS(f.activeParameter(0),1.0);
    TS_ASSERT_EQUALS(f.activeParameter(1),1.2);
    TS_ASSERT_EQUALS(f.activeParameter(2),1.3);

    TS_ASSERT_EQUALS(f.nameOfActive(0),"c0");
    TS_ASSERT_EQUALS(f.nameOfActive(1),"c2");
    TS_ASSERT_EQUALS(f.nameOfActive(2),"c3");

    TS_ASSERT_EQUALS(f.indexOfActive(0),0);
    TS_ASSERT_EQUALS(f.indexOfActive(1),2);
    TS_ASSERT_EQUALS(f.indexOfActive(2),3);

    TS_ASSERT(   f.isActive(0));
    TS_ASSERT( ! f.isActive(1));
    TS_ASSERT(   f.isActive(2));
    TS_ASSERT(   f.isActive(3));

    TS_ASSERT_EQUALS(f.activeIndex(0),0);
    TS_ASSERT_EQUALS(f.activeIndex(1),-1);
    TS_ASSERT_EQUALS(f.activeIndex(2),1);
    TS_ASSERT_EQUALS(f.activeIndex(3),2);

  }

  void testSetActiveParameter()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.removeActive(1);
    f.removeActive(3);

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    f.setActiveParameter(0,2.0);
    f.setActiveParameter(1,2.1);

    TS_ASSERT_EQUALS(f.activeParameter(0),2.0);
    TS_ASSERT_EQUALS(f.activeParameter(1),2.1);

    TS_ASSERT_EQUALS(f.parameter(0),2.0);
    TS_ASSERT_EQUALS(f.parameter(1),1.1);
    TS_ASSERT_EQUALS(f.parameter(2),2.1);
    TS_ASSERT_EQUALS(f.parameter(3),1.3);

    TS_ASSERT_EQUALS(f.getParameter("c0"),2.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),1.1);
    TS_ASSERT_EQUALS(f.getParameter("c2"),2.1);
    TS_ASSERT_EQUALS(f.getParameter("c3"),1.3);

  }

  void testTie()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.tie("c1","0");
    f.tie("c3","0");

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    TS_ASSERT_EQUALS(f.activeParameter(0),1.0);
    TS_ASSERT_EQUALS(f.activeParameter(1),1.2);

    TS_ASSERT_EQUALS(f.nameOfActive(0),"c0");
    TS_ASSERT_EQUALS(f.nameOfActive(1),"c2");

    TS_ASSERT_EQUALS(f.indexOfActive(0),0);
    TS_ASSERT_EQUALS(f.indexOfActive(1),2);

    TS_ASSERT(   f.isActive(0));
    TS_ASSERT( ! f.isActive(1));
    TS_ASSERT(   f.isActive(2));
    TS_ASSERT( ! f.isActive(3));

    TS_ASSERT_EQUALS(f.activeIndex(0),0);
    TS_ASSERT_EQUALS(f.activeIndex(1),-1);
    TS_ASSERT_EQUALS(f.activeIndex(2),1);
    TS_ASSERT_EQUALS(f.activeIndex(3),-1);

    TS_ASSERT_THROWS(f.tie("c1","c0+4"),std::logic_error);

  }

  void testApplyTies()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.tie("c1","c0+4");
    f.tie("c3","c2/2");

    f.applyTies();

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    TS_ASSERT_EQUALS(f.getParameter("c0"),1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"),1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),0.6);

  }

  void testRemoveTie()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.tie("c1","c0+4");
    f.tie("c3","c2/2");

    f.applyTies();

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    TS_ASSERT_EQUALS(f.getParameter("c0"),1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"),1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),0.6);

    f.removeTie("c3");
    f.getParameter("c3") = 3.3;

    f.applyTies();

    TS_ASSERT_EQUALS(f.nActive(),3);

    TS_ASSERT_EQUALS(f.getParameter("c0"),1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"),1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),3.3);

    TS_ASSERT(   f.isActive(0));
    TS_ASSERT( ! f.isActive(1));
    TS_ASSERT(   f.isActive(2));
    TS_ASSERT(   f.isActive(3));

    TS_ASSERT_EQUALS(f.activeIndex(0),0);
    TS_ASSERT_EQUALS(f.activeIndex(1),-1);
    TS_ASSERT_EQUALS(f.activeIndex(2),1);
    TS_ASSERT_EQUALS(f.activeIndex(3),2);

  }

  void testClearTies()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.tie("c1","c0+4");
    f.tie("c3","c2/2");

    f.applyTies();

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    TS_ASSERT_EQUALS(f.getParameter("c0"),1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"),1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),0.6);

    f.clearTies();
    f.getParameter("c1") = 3.1;
    f.getParameter("c3") = 3.3;

    f.applyTies();

    TS_ASSERT_EQUALS(f.nActive(),4);

    TS_ASSERT_EQUALS(f.getParameter("c0"),1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),3.1);
    TS_ASSERT_EQUALS(f.getParameter("c2"),1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),3.3);

    TS_ASSERT(   f.isActive(0));
    TS_ASSERT(   f.isActive(1));
    TS_ASSERT(   f.isActive(2));
    TS_ASSERT(   f.isActive(3));

    TS_ASSERT_EQUALS(f.activeIndex(0),0);
    TS_ASSERT_EQUALS(f.activeIndex(1),1);
    TS_ASSERT_EQUALS(f.activeIndex(2),2);
    TS_ASSERT_EQUALS(f.activeIndex(3),3);

  }

  void testUpdateActive()
  {
    IFT_Funct f;

    f.getParameter("c0") = 1.0;
    f.getParameter("c1") = 1.1;
    f.getParameter("c2") = 1.2;
    f.getParameter("c3") = 1.3;

    f.tie("c1","c0+4");
    f.tie("c3","c2/2");

    double in[] = {6.,22.2};

    f.updateActive(in);

    TS_ASSERT_EQUALS(f.nParams(),4);
    TS_ASSERT_EQUALS(f.nActive(),2);

    TS_ASSERT_EQUALS(f.getParameter("c0"),6.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"),10.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"),22.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"),11.1);

  }

  private:
  void interrupt()
  {
    int iii;
    std::cerr<<"Enter a number:";
    std::cin>>iii;
  }

};

#endif /*IFUNCTIONTEST_H_*/
