#ifndef FITTEST_H_
#define FITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class FitExpression
{
public:
  double operator()(double x)
  {return 1+0.3*x+exp(-0.5*(x-4)*(x-4)*2)+2*exp(-0.5*(x-6)*(x-6)*3);}
};

class FitExpression1
{
public:
  double operator()(double x)
  {return 1+0.3*x+0.01*exp(-0.5*(x-4)*(x-4)*2);}
};

class FitExp
{
public:
  double operator()(double x)
  {return exp(-0.5*(x-5)*(x-5)*2);}
};

class FitTest_Gauss: public IPeakFunction
{
public:
  FitTest_Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "Gauss";}

  void functionLocal(double* out, const double* xValues, const int& nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-0.5*x*x*w);
    }
  }
  void functionDerivLocal(Jacobian* out, const double* xValues, const int& nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      double e = h*exp(-0.5*x*x*w);
      out->set(i,0,x*h*e*w);
      out->set(i,1,e);
      out->set(i,2,-0.5*x*x*h*e);
    }
  }

  double centre()const
  {
    return getParameter(0);
  }

  double height()const
  {
    return getParameter(1);
  }

  double width()const
  {
    return getParameter(2);
  }

  void setCentre(const double c)
  {
    setParameter(0,c);
  }
  void setHeight(const double h)
  {
    setParameter(1,h);
  }

  void setWidth(const double w)
  {
    setParameter(2,w);
  }

};


class FitTest_Linear: public Function
{
public:
  FitTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "Linear";}

  void function(double* out, const double* xValues, const int& nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(int i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(int i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }

};

DECLARE_FUNCTION(FitTest_Gauss);
DECLARE_FUNCTION(FitTest_Linear);

class FitTest : public CxxTest::TestSuite
{
public:
  FitTest()
  {
    Kernel::ConfigService::Instance().setString("curvefitting.peakRadius","100");
    FrameworkManager::Instance();
  }

  void testFit()
  {

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";
    params += "name=FitTest_Gauss, c=4.1,h=1.1,s=0.5;";
    params += "name=FitTest_Gauss, c=6.1,h=3.1,s=0.5;";

    alg.setPropertyValue("Function",params);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);
    for(int i=0;i<Y.size();i++)
    {
      TS_ASSERT_EQUALS(Y00[i],Y0[i]);
      TS_ASSERT_DELTA(Y0[i],Y[i],0.001);
      TS_ASSERT_DIFFERS(R[i],0);
    }

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),1,0.00001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.3,0.00001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),4,0.00001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),1,0.00001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    TS_ASSERT_DELTA(row.Double(1),2,0.00001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6,0.00001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),2,0.0001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3,0.0005);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testTies()
  {

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";
    params += "name=FitTest_Gauss, c=4.1,h=1.1,s=2.2;";
    params += "name=FitTest_Gauss, c=6.1,h=3.1,s=3.3;";

    alg.setPropertyValue("Function",params);
    alg.setPropertyValue("Ties","f1.s=f2.s/3");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),0.9677,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.3036,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),4.1274,0.0001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),0.9456,0.0001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    double s1 = row.Double(1);
    TS_ASSERT_DELTA(row.Double(1),1.1476,0.0001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6.0547,0.0001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),1.9206,0.0001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.443,0.0001);
    double s2 = row.Double(1);

    TS_ASSERT_DELTA(s1,s2/3,1e-12);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testNotMasked()
  {

    WS_type ws = mkWS(FitExpression1(),1,0,10,0.1,1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";

    alg.setPropertyValue("Function",params);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),2);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"a");
    TS_ASSERT_DELTA(row.Double(1),1.0028,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"b");
    TS_ASSERT_DELTA(row.Double(1),0.2998,0.0001);



    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testMasked()
  {

    WS_type ws = mkWS(FitExpression1(),1,0,10,0.1,1);

    // Mask some bins
    for(int i=0;i<ws->blocksize();i++)
    {
      double x = ws->readX(0)[i];
      if (x > 2 && x < 6)
      {
        ws->maskBin(0,i,1.0);
      }
    }
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";

    alg.setPropertyValue("Function",params);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),2);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"a");
    TS_ASSERT_DELTA(row.Double(1),1.,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"b");
    TS_ASSERT_DELTA(row.Double(1),0.3,0.0001);



    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testFitConstrained()
  {

    WS_type ws = mkWS(FitExp(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "name=FitTest_Gauss, c=4.1,h=1.1,s=0.5,constraint=(4.0<c<4.2)";

    alg.setPropertyValue("Function",params);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);
    //for(int i=0;i<Y.size();i++)
    //{
    //  TS_ASSERT_EQUALS(Y00[i],Y0[i]);
    //  TS_ASSERT_DELTA(Y0[i],Y[i],0.001);
    //  TS_ASSERT_DIFFERS(R[i],0);
    //}

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),3);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"c");
    TS_ASSERT_DELTA(row.Double(1),4.1988,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"h");
    TS_ASSERT_DELTA(row.Double(1),1.0261,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"s");
    TS_ASSERT_DELTA(row.Double(1),0.5455,0.0001);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testFitConstrained1()
  {

    WS_type ws = mkWS(FitExp(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "name=FitTest_Gauss, c=4.1,h=1.1,s=0.5";

    alg.setPropertyValue("Function",params);

    alg.setPropertyValue("Constraints","4.0<c<4.2");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);
    //for(int i=0;i<Y.size();i++)
    //{
    //  TS_ASSERT_EQUALS(Y00[i],Y0[i]);
    //  TS_ASSERT_DELTA(Y0[i],Y[i],0.001);
    //  TS_ASSERT_DIFFERS(R[i],0);
    //}

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),3);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"c");
    TS_ASSERT_DELTA(row.Double(1),4.1988,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"h");
    TS_ASSERT_DELTA(row.Double(1),1.0261,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"s");
    TS_ASSERT_DELTA(row.Double(1),0.5455,0.0001);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testConstraints2()
  {
    //press_return();

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";
    params += "name=FitTest_Gauss, c=4.1,h=1.1,s=2.2;";
    params += "name=FitTest_Gauss, c=6.1,h=3.1,s=3.3;";

    alg.setPropertyValue("Function",params);
    alg.setPropertyValue("Constraints","4.1<f1.c<4.2");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),0.9998,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.3001,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),4.0999,0.0001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),0.9842,0.0001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    double s1 = row.Double(1);
    TS_ASSERT_DELTA(row.Double(1),1.6709,0.0001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6.0282,0.0001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),1.9832,0.0001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.2843,0.0001);
    double s2 = row.Double(1);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testConstraints3()
  {
    //press_return();

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";
    params += "name=FitTest_Gauss, c=4.1,h=1.1,s=2.2;";
    params += "name=FitTest_Gauss, c=6.1,h=3.1,s=3.3;";

    alg.setPropertyValue("Function",params);
    alg.setPropertyValue("Constraints","f1.c > 4.1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),0.9998,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.3001,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),4.0999,0.0001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),0.9842,0.0001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    double s1 = row.Double(1);
    TS_ASSERT_DELTA(row.Double(1),1.6709,0.0001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6.0282,0.0001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),1.9832,0.0001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.2842,0.0001);
    double s2 = row.Double(1);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testConstraints4()
  {
    //press_return();

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";
    params += "name=FitTest_Gauss, c=4.1,h=1.1,s=2.2;";
    params += "name=FitTest_Gauss, c=6.1,h=3.1,s=3.3;";

    alg.setPropertyValue("Function",params);
    alg.setPropertyValue("Constraints","f1.c < 3.9");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),2);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),0.9972,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.3002,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),3.8999,0.0001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),1.1712,0.0001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    double s1 = row.Double(1);
    TS_ASSERT_DELTA(row.Double(1),2.9998,0.0001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),5.9822,0.0001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),2.0322,0.0001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),2.8014,0.0001);
    double s2 = row.Double(1);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testConstraints5()
  {
    //press_return();

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "name=FitTest_Linear,a=1,b=0;";
    params += "name=FitTest_Gauss, c=4.1,h=1.1,s=2.2;";
    params += "name=FitTest_Gauss, c=6.1,h=3.1,s=3.3;";

    alg.setPropertyValue("Function",params);
    alg.setPropertyValue("Constraints","f1.c < 3.9,f2.c > 7");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WS_type outWS = getWS("out_Workspace");

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),1.0016,0.0001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.0003,0.0001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),3.9005,0.0001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),1.1018,0.0001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    double s1 = row.Double(1);
    TS_ASSERT_DELTA(row.Double(1),2.1878,0.0001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6.9998,0.0001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),3.1003,0.0001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.2970,0.0001);
    double s2 = row.Double(1);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

private:

  template<class Funct>
  WS_type mkWS(Funct f,int nSpec,double x0,double x1,double dx,bool isHist=false)
  {
    int nX = int((x1 - x0)/dx) + 1;
    int nY = nX - (isHist?1:0);
    if (nY <= 0)
      throw std::invalid_argument("Cannot create an empty workspace");

    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",nSpec,nX,nY));

    double spec;
    double x;

    for(int iSpec=0;iSpec<nSpec;iSpec++)
    {
      spec = iSpec;
      Mantid::MantidVec& X = ws->dataX(iSpec);
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(int i=0;i<nY;i++)
      {
        x = x0 + dx*i;
        X[i] = x;
        Y[i] = f(x);
        E[i] = 1;
      }
      if (isHist)
        X.back() = X[nY-1] + dx;
    }
    return ws;
  }

  void storeWS(const std::string& name,WS_type ws)
  {
    AnalysisDataService::Instance().add(name,ws);
  }

  void removeWS(const std::string& name)
  {
    AnalysisDataService::Instance().remove(name);
  }

  WS_type getWS(const std::string& name)
  {
    return boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(name));
  }

  TWS_type getTWS(const std::string& name)
  {
    return boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(name));
  }

  void addNoise(WS_type ws,double noise)
  {
    for(int iSpec=0;iSpec<ws->getNumberHistograms();iSpec++)
    {
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(int i=0;i<Y.size();i++)
      {
        Y[i] += noise*(-.5 + double(rand())/RAND_MAX);
        E[i] += noise;
      }
    }
  }

  void press_return()
  {
    std::cerr<<"Press Return";
    std::string str;
    getline(std::cin,str);
  }

};

#endif /*FITTEST_H_*/
