#ifndef COMPOSITEFUNCTIONTEST_H_
#define COMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid;
using namespace Mantid::API;

class CompositeFunctionTest_IFunction;

class CompositeFunctionTest_MocSpectrum: public ISpectrum
{
public:
  
  CompositeFunctionTest_MocSpectrum(size_t nx, size_t ny):m_x(nx),m_y(ny),m_e(ny) {}

  void clearData() {}
  void setData(const MantidVec&){}
  void setData(const MantidVec&, const MantidVec&){}

  void setData(const MantidVecPtr&){}
  void setData(const MantidVecPtr&, const MantidVecPtr&){}

  void setData(const MantidVecPtr::ptr_type&){}
  void setData(const MantidVecPtr::ptr_type&, const MantidVecPtr::ptr_type&){}

  virtual MantidVec& dataX() {return m_x;}
  virtual MantidVec& dataY() {return m_y;}
  virtual MantidVec& dataE() {return m_e;}

  virtual const MantidVec& dataX() const {return m_x;}
  virtual const MantidVec& dataY() const {return m_y;}
  virtual const MantidVec& dataE() const {return m_e;}

  virtual size_t getMemorySize() const {return 0;}

  MantidVec m_x,m_y,m_e;
};

class CompositeFunctionTest_MocMatrixWorkspace : public MatrixWorkspace
{
public:
  
  CompositeFunctionTest_MocMatrixWorkspace(size_t nspec, size_t nx, size_t ny):m_blocksize(ny)
  {
    for(size_t i = 0; i < nspec; ++i)
    {
      CompositeFunctionTest_MocSpectrum sp(nx,ny);
      m_spectra.push_back(sp);
    }
  }

  ~CompositeFunctionTest_MocMatrixWorkspace(){}

  // Section required for iteration
  /// Returns the number of single indexable items in the workspace
  virtual std::size_t size() const {return m_spectra.size()*m_blocksize;}
  /// Returns the size of each block of data returned by the dataY accessors
  virtual std::size_t blocksize() const {return m_blocksize;}
  /// Returns the number of histograms in the workspace
  virtual std::size_t getNumberHistograms() const {return m_spectra.size();}

  /// Return the underlying ISpectrum ptr at the given workspace index.
  virtual ISpectrum * getSpectrum(const size_t index) {return &m_spectra[index];}

  /// Return the underlying ISpectrum ptr (const version) at the given workspace index.
  virtual const ISpectrum * getSpectrum(const size_t index) const {return &m_spectra[index];}
  const std::string id(void) const {return "";}
  void init(const size_t &,const size_t &,const size_t &) { }
private:
  std::vector<CompositeFunctionTest_MocSpectrum> m_spectra;
  size_t m_blocksize;
};

std::vector<CompositeFunctionTest_IFunction*> CompositeFunctionTest_FunctionDeleted;

class CompositeFunctionTest_IFunction
{
public:
  ~CompositeFunctionTest_IFunction()
  {
    CompositeFunctionTest_FunctionDeleted.push_back(this);
  }
};

class Gauss: public IPeakFunction, public CompositeFunctionTest_IFunction
{
public:
  Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "Gauss";}

  void functionLocal(double* out, const double* xValues, const size_t nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-0.5*x*x*w);
    }
  }
  void functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      double e = h*exp(-0.5*x*x*w);
      out->set(static_cast<int>(i),0,x*h*e*w);
      out->set(static_cast<int>(i),1,e);
      out->set(static_cast<int>(i),2,-0.5*x*x*h*e);
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

  double fwhm()const
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

  void setFwhm(const double w)
  {
    setParameter(2,w);
  }

};


class Linear: public ParamFunction, public IFunctionMW, public CompositeFunctionTest_IFunction
{
public:
  Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "Linear";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(size_t i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(size_t i=0;i<nData;i++)
    {
      out->set(static_cast<int>(i),0,1.);
      out->set(static_cast<int>(i),1,xValues[i]);
    }
  }

};

class Cubic: public ParamFunction, public IFunctionMW, public CompositeFunctionTest_IFunction
{
public:
  Cubic()
  {
    declareParameter("c0");
    declareParameter("c1");
    declareParameter("c2");
    declareParameter("c3");
  }

  std::string name()const{return "Cubic";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    double c0 = getParameter("c0");
    double c1 = getParameter("c1");
    double c2 = getParameter("c2");
    double c3 = getParameter("c3");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i];
      out[i] = c0 + x*(c1 + x*(c2 + x*c3));
    }
  }
  void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i];
      out->set(static_cast<int>(i),0,1.);
      out->set(static_cast<int>(i),1,x);
      out->set(static_cast<int>(i),2,x*x);
      out->set(static_cast<int>(i),3,x*x*x);
    }
  }

};

class CompositeFunctionTest : public CxxTest::TestSuite
{
public:
  void testAdd()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    TS_ASSERT_EQUALS(mfun->nParams(),12);
    TS_ASSERT_EQUALS(mfun->nActive(),12);

    TS_ASSERT_EQUALS(mfun->getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.3);
    TS_ASSERT_EQUALS(mfun->getParameter(5),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),2.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7),2.3);
    TS_ASSERT_EQUALS(mfun->getParameter(8),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10),3.2);
    TS_ASSERT_EQUALS(mfun->getParameter(11),3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1),"f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2),"f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3),"f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4),"f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5),"f2.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(6),"f2.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(7),"f2.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(8),"f2.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(9),"f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(10),"f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(11),"f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"),1.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"),2.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"),2.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"),3.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"),3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"),0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"),1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"),2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"),3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"),4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c0"),5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c1"),6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c2"),7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c3"),8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"),9);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"),10);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"),11);

    std::string str = "name=Linear,a=0.8,b=0;";
    str += "name=Gauss,c=1.1,h=1.2,s=1.3;";
    str += "name=Cubic,c0=2.1,c1=2.2,c2=2.3,c3=2.4;";
    str += "name=Gauss,c=3.1,h=3.2,s=3.3";

    TS_ASSERT_EQUALS(mfun->asString(),str);

    clearDeleted();
    delete mfun;
    TS_ASSERT(isDeleted(bk));
    TS_ASSERT(isDeleted(g1));
    TS_ASSERT(isDeleted(g2));
    TS_ASSERT(isDeleted(cub));
  }

  void testTies()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.a","0");
    mfun->tie("f0.b","0");
    mfun->tie("f1.s","0");
    mfun->tie("f2.c1","0");
    mfun->tie("f2.c2","0");
    mfun->tie("f3.h","0");

    TS_ASSERT_EQUALS(mfun->activeParameter(0),1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),3.3);

    TS_ASSERT_EQUALS(mfun->nameOfActive(0),"f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(1),"f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(2),"f2.c0");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3),"f2.c3");
    TS_ASSERT_EQUALS(mfun->nameOfActive(4),"f3.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5),"f3.s");

    TS_ASSERT(   mfun->isFixed(0));
    TS_ASSERT(   mfun->isFixed(1));
    TS_ASSERT( ! mfun->isFixed(2));
    TS_ASSERT( ! mfun->isFixed(3));
    TS_ASSERT(   mfun->isFixed(4));
    TS_ASSERT( ! mfun->isFixed(5));
    TS_ASSERT(   mfun->isFixed(6));
    TS_ASSERT(   mfun->isFixed(7));
    TS_ASSERT( ! mfun->isFixed(8));
    TS_ASSERT( ! mfun->isFixed(9));
    TS_ASSERT(   mfun->isFixed(10));
    TS_ASSERT( ! mfun->isFixed(11));

    TS_ASSERT_EQUALS(mfun->nParams(),12);
    TS_ASSERT_EQUALS(mfun->nActive(),6);

    TS_ASSERT_EQUALS(mfun->getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.3);
    TS_ASSERT_EQUALS(mfun->getParameter(5),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),2.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7),2.3);
    TS_ASSERT_EQUALS(mfun->getParameter(8),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10),3.2);
    TS_ASSERT_EQUALS(mfun->getParameter(11),3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1),"f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2),"f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3),"f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4),"f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5),"f2.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(6),"f2.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(7),"f2.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(8),"f2.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(9),"f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(10),"f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(11),"f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"),1.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"),2.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"),2.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"),3.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"),3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"),0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"),1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"),2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"),3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"),4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c0"),5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c1"),6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c2"),7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c3"),8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"),9);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"),10);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"),11);

    delete mfun;
  }

  void testSetActive()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.a","-1");
    mfun->tie("f0.b","-2");
    mfun->tie("f1.s","-3");
    mfun->tie("f2.c1","-4");
    mfun->tie("f2.c2","-5");
    mfun->tie("f3.h","-6");

    mfun->setActiveParameter(0,100);
    mfun->setActiveParameter(1,101);
    mfun->setActiveParameter(2,102);
    mfun->setActiveParameter(3,103);
    mfun->setActiveParameter(4,104);
    mfun->setActiveParameter(5,105);

    TS_ASSERT_EQUALS(mfun->activeParameter(0),100);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),101);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),102);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),103);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),104);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),105);

    TS_ASSERT_EQUALS(mfun->nParams(),12);
    TS_ASSERT_EQUALS(mfun->nActive(),6);

    TS_ASSERT_EQUALS(mfun->getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2),100);
    TS_ASSERT_EQUALS(mfun->getParameter(3),101);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.3);
    TS_ASSERT_EQUALS(mfun->getParameter(5),102);
    TS_ASSERT_EQUALS(mfun->getParameter(6),2.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7),2.3);
    TS_ASSERT_EQUALS(mfun->getParameter(8),103);
    TS_ASSERT_EQUALS(mfun->getParameter(9),104);
    TS_ASSERT_EQUALS(mfun->getParameter(10),3.2);
    TS_ASSERT_EQUALS(mfun->getParameter(11),105);
    
    delete mfun;
  }

  void testFix()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->fix(0);
    mfun->fix(1);
    mfun->fix(4);
    //g1->fix(2);  // This doesn't work
    mfun->fix(6);
    mfun->fix(7);
    mfun->fix(10);
    //g2->fix(1);  // This doesn't work

    mfun->setActiveParameter(0,100);
    mfun->setActiveParameter(1,101);
    mfun->setActiveParameter(2,102);
    mfun->setActiveParameter(3,103);
    mfun->setActiveParameter(4,104);
    mfun->setActiveParameter(5,105);

    TS_ASSERT_EQUALS(mfun->activeParameter(0),100);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),101);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),102);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),103);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),104);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),105);

    TS_ASSERT_EQUALS(mfun->nParams(),12);
    TS_ASSERT_EQUALS(mfun->nActive(),6);

    TS_ASSERT_EQUALS(mfun->getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2),100);
    TS_ASSERT_EQUALS(mfun->getParameter(3),101);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.3);
    TS_ASSERT_EQUALS(mfun->getParameter(5),102);
    TS_ASSERT_EQUALS(mfun->getParameter(6),2.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7),2.3);
    TS_ASSERT_EQUALS(mfun->getParameter(8),103);
    TS_ASSERT_EQUALS(mfun->getParameter(9),104);
    TS_ASSERT_EQUALS(mfun->getParameter(10),3.2);
    TS_ASSERT_EQUALS(mfun->getParameter(11),105);

    delete mfun;
  }

  void testApplyTies()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.b","77");
    mfun->tie("f0.a","2*f0.b");
    mfun->tie("f1.s","f3.s/2");
    mfun->tie("f2.c1","f2.c3^2");
    mfun->tie("f2.c2","sqrt(f2.c3)");
    mfun->tie("f3.h","f2.c0+f0.b");

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->activeParameter(0),1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),3.3);

    TS_ASSERT_EQUALS(mfun->nParams(),12);
    TS_ASSERT_EQUALS(mfun->nActive(),6);

    TS_ASSERT_EQUALS(mfun->getParameter(0),154);
    TS_ASSERT_EQUALS(mfun->getParameter(1),77);
    TS_ASSERT_EQUALS(mfun->getParameter(2),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.65);
    TS_ASSERT_EQUALS(mfun->getParameter(5),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),2.4*2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(7),sqrt(2.4));
    TS_ASSERT_EQUALS(mfun->getParameter(8),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10),79.1);
    TS_ASSERT_EQUALS(mfun->getParameter(11),3.3);

    delete mfun;
  }

  void testApplyTiesInWrongOrder()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.a","2*f0.b");
    mfun->tie("f0.b","77");
    mfun->tie("f1.s","f3.s/2");
    mfun->tie("f2.c1","f2.c3^2");
    mfun->tie("f2.c2","sqrt(f2.c3)");
    mfun->tie("f3.h","f2.c0+f0.b");

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->activeParameter(0),1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),3.3);

    TS_ASSERT_EQUALS(mfun->nParams(),12);
    TS_ASSERT_EQUALS(mfun->nActive(),6);

    TS_ASSERT_DIFFERS(mfun->getParameter(0),154);
    TS_ASSERT_EQUALS(mfun->getParameter(1),77);
    TS_ASSERT_EQUALS(mfun->getParameter(2),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.65);
    TS_ASSERT_EQUALS(mfun->getParameter(5),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),2.4*2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(7),sqrt(2.4));
    TS_ASSERT_EQUALS(mfun->getParameter(8),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10),79.1);
    TS_ASSERT_EQUALS(mfun->getParameter(11),3.3);

    delete mfun;
  }

  void testRemoveFunction()
  {

    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.a","101");
    mfun->tie("f0.b","102");
    mfun->tie("f1.s","103");
    mfun->tie("f2.c1","104");
    mfun->tie("f2.c2","105");
    mfun->tie("f3.h","106");

    clearDeleted();
    mfun->removeFunction(2);
    TS_ASSERT(isDeleted(cub));

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->nFunctions(),3);

    TS_ASSERT_EQUALS(mfun->nParams(),8);
    TS_ASSERT_EQUALS(mfun->nActive(),4);

    TS_ASSERT_EQUALS(mfun->getParameter(0),101);
    TS_ASSERT_EQUALS(mfun->getParameter(1),102);
    TS_ASSERT_EQUALS(mfun->getParameter(2),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4),103);
    TS_ASSERT_EQUALS(mfun->getParameter(5),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),106);
    TS_ASSERT_EQUALS(mfun->getParameter(7),3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1),"f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2),"f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3),"f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4),"f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5),"f2.c");
    TS_ASSERT_EQUALS(mfun->parameterName(6),"f2.h");
    TS_ASSERT_EQUALS(mfun->parameterName(7),"f2.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"),101);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"),102);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"),103);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c"),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.h"),106);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.s"),3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"),0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"),1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"),2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"),3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"),4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c"),5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.h"),6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.s"),7);

    TS_ASSERT_EQUALS(mfun->nameOfActive(0),"f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(1),"f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(2),"f2.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3),"f2.s");

    TS_ASSERT_EQUALS(mfun->activeParameter(0),1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),3.3);

    TS_ASSERT(   mfun->isFixed(0));
    TS_ASSERT(   mfun->isFixed(1));
    TS_ASSERT( ! mfun->isFixed(2));
    TS_ASSERT( ! mfun->isFixed(3));
    TS_ASSERT(   mfun->isFixed(4));
    TS_ASSERT( ! mfun->isFixed(5));
    TS_ASSERT(   mfun->isFixed(6));
    TS_ASSERT( ! mfun->isFixed(7));

    delete mfun;
  }

  // replacing function has fewer parameters
  void testReplaceFunction()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.a","101");
    mfun->tie("f0.b","102");
    mfun->tie("f1.s","103");
    mfun->tie("f2.c1","104");
    mfun->tie("f2.c2","105");
    mfun->tie("f3.h","106");

    Linear* bk1 = new Linear();
    bk1->setParameter("a",4.1);
    bk1->setParameter("b",4.2);

    clearDeleted();
    mfun->replaceFunction(2,bk1);
    TS_ASSERT(isDeleted(cub));

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->nFunctions(),4);

    TS_ASSERT_EQUALS(mfun->nParams(),10);
    TS_ASSERT_EQUALS(mfun->nActive(),6);

    TS_ASSERT_EQUALS(mfun->getParameter(0),101);
    TS_ASSERT_EQUALS(mfun->getParameter(1),102);
    TS_ASSERT_EQUALS(mfun->getParameter(2),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4),103);
    TS_ASSERT_EQUALS(mfun->getParameter(5),4.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),4.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(8),106);
    TS_ASSERT_EQUALS(mfun->getParameter(9),3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1),"f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2),"f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3),"f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4),"f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5),"f2.a");
    TS_ASSERT_EQUALS(mfun->parameterName(6),"f2.b");
    TS_ASSERT_EQUALS(mfun->parameterName(7),"f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(8),"f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(9),"f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"),101);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"),102);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"),103);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.a"),4.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.b"),4.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"),106);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"),3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"),0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"),1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"),2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"),3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"),4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.a"),5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.b"),6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"),7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"),8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"),9);

    TS_ASSERT_EQUALS(mfun->nameOfActive(0),"f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(1),"f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(2),"f2.a");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3),"f2.b");
    TS_ASSERT_EQUALS(mfun->nameOfActive(4),"f3.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5),"f3.s");

    TS_ASSERT_EQUALS(mfun->activeParameter(0),1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),4.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),4.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),3.3);

    TS_ASSERT(   mfun->isFixed(0));
    TS_ASSERT(   mfun->isFixed(1));
    TS_ASSERT( ! mfun->isFixed(2));
    TS_ASSERT( ! mfun->isFixed(3));
    TS_ASSERT(   mfun->isFixed(4));
    TS_ASSERT( ! mfun->isFixed(5));
    TS_ASSERT( ! mfun->isFixed(6));
    TS_ASSERT( ! mfun->isFixed(7));
    TS_ASSERT(   mfun->isFixed(8));
    TS_ASSERT( ! mfun->isFixed(9));

    clearDeleted();
    delete mfun;
    TS_ASSERT(isDeleted(bk));
    TS_ASSERT(isDeleted(g1));
    TS_ASSERT(isDeleted(g2));
    TS_ASSERT(isDeleted(bk1));
  }

  // replacing function has more parameters
  void testReplaceFunction1()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);

    g1->setParameter("c",1.1);
    g1->setParameter("h",1.2);
    g1->setParameter("s",1.3);

    cub->setParameter("c0",2.1);
    cub->setParameter("c1",2.2);
    cub->setParameter("c2",2.3);
    cub->setParameter("c3",2.4);

    g2->setParameter("c",3.1);
    g2->setParameter("h",3.2);
    g2->setParameter("s",3.3);

    mfun->tie("f0.a","101");
    mfun->tie("f0.b","102");
    mfun->tie("f1.s","103");
    mfun->tie("f2.c1","104");
    mfun->tie("f2.c2","105");
    mfun->tie("f3.h","106");

    Cubic* cub1 = new Cubic();
    cub1->setParameter("c0",4.1);
    cub1->setParameter("c1",4.2);
    cub1->setParameter("c2",4.3);
    cub1->setParameter("c3",4.4);

    clearDeleted();
    mfun->replaceFunction(0, cub1);
    TS_ASSERT(isDeleted(bk));

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->nFunctions(),4);

    TS_ASSERT_EQUALS(mfun->nParams(),14);
    TS_ASSERT_EQUALS(mfun->nActive(),10);

    TS_ASSERT_EQUALS(mfun->getParameter(0),4.1);
    TS_ASSERT_EQUALS(mfun->getParameter(1),4.2);
    TS_ASSERT_EQUALS(mfun->getParameter(2),4.3);
    TS_ASSERT_EQUALS(mfun->getParameter(3),4.4);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(5),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(6),103);
    TS_ASSERT_EQUALS(mfun->getParameter(7),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(8),104);
    TS_ASSERT_EQUALS(mfun->getParameter(9),105);
    TS_ASSERT_EQUALS(mfun->getParameter(10),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(11),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(12),106);
    TS_ASSERT_EQUALS(mfun->getParameter(13),3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0),"f0.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(1),"f0.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(2),"f0.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(3),"f0.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(4),"f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(5),"f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(6),"f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(7),"f2.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(8),"f2.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(9),"f2.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(10),"f2.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(11),"f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(12),"f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(13),"f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.c0"),4.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.c1"),4.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.c2"),4.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.c3"),4.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"),103);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"),2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"),104);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"),105);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"),2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"),106);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"),3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c0"),0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c1"),1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c2"),2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c3"),3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"),4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"),5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"),6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c0"),7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c1"),8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c2"),9);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c3"),10);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"),11);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"),12);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"),13);

    TS_ASSERT_EQUALS(mfun->activeParameter(0),4.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1),4.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2),4.3);
    TS_ASSERT_EQUALS(mfun->activeParameter(3),4.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(4),1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(5),1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(6),2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(7),2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(8),3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(9),3.3);

    TS_ASSERT_EQUALS(mfun->nameOfActive(0),"f0.c0");
    TS_ASSERT_EQUALS(mfun->nameOfActive(1),"f0.c1");
    TS_ASSERT_EQUALS(mfun->nameOfActive(2),"f0.c2");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3),"f0.c3");
    TS_ASSERT_EQUALS(mfun->nameOfActive(4),"f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5),"f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(6),"f2.c0");
    TS_ASSERT_EQUALS(mfun->nameOfActive(7),"f2.c3");
    TS_ASSERT_EQUALS(mfun->nameOfActive(8),"f3.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(9),"f3.s");

    TS_ASSERT( ! mfun->isFixed(0));
    TS_ASSERT( ! mfun->isFixed(1));
    TS_ASSERT( ! mfun->isFixed(2));
    TS_ASSERT( ! mfun->isFixed(3));
    TS_ASSERT( ! mfun->isFixed(4));
    TS_ASSERT( ! mfun->isFixed(5));
    TS_ASSERT(   mfun->isFixed(6));
    TS_ASSERT( ! mfun->isFixed(7));
    TS_ASSERT(   mfun->isFixed(8));
    TS_ASSERT(   mfun->isFixed(9));
    TS_ASSERT( ! mfun->isFixed(10));
    TS_ASSERT( ! mfun->isFixed(11));
    TS_ASSERT(   mfun->isFixed(12));
    TS_ASSERT( ! mfun->isFixed(13));

    clearDeleted();
    delete mfun;
    TS_ASSERT(isDeleted(g1));
    TS_ASSERT(isDeleted(g2));
    TS_ASSERT(isDeleted(cub));
    TS_ASSERT(isDeleted(cub1));
  }

  void testAddFunctionsWithTies()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g = new Gauss();
    Linear *bk = new Linear();

    bk->setParameter("a",0.1);
    bk->setParameter("b",0.2);

    bk->tie("b","a/2");

    g->setParameter("c",1.1);
    g->setParameter("h",1.2);
    g->setParameter("s",1.3);
    g->tie("s","1.33");

    mfun->addFunction(bk);
    mfun->addFunction(g);

    mfun->tie("f1.h","f0.b*4");

    TS_ASSERT_EQUALS(mfun->nParams(),5);
    TS_ASSERT_EQUALS(mfun->nActive(),2);

    TS_ASSERT( ! mfun->isFixed(0));
    TS_ASSERT(   mfun->isFixed(1));
    TS_ASSERT( ! mfun->isFixed(2));
    TS_ASSERT(   mfun->isFixed(3));
    TS_ASSERT(   mfun->isFixed(4));

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"),0.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"),0.05);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"),0.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"),1.33);

    delete mfun;
  }

  void testRemoveFunctionWithTies()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g = new Gauss();
    Linear *bk = new Linear();

    bk->setParameter("a",0.1);
    bk->setParameter("b",0.2);

    g->setParameter("c",1.1);
    g->setParameter("h",1.2);
    g->setParameter("s",1.3);

    mfun->addFunction(bk);
    mfun->addFunction(g);

    mfun->tie("f1.h","f0.b*4");
    mfun->tie("f1.s","f1.h/4");

    TS_ASSERT_EQUALS(mfun->nParams(),5);
    TS_ASSERT_EQUALS(mfun->nActive(),3);

    mfun->removeFunction(0);

    TS_ASSERT_EQUALS(mfun->nParams(),3);
    TS_ASSERT_EQUALS(mfun->nActive(),2);

    TS_ASSERT( ! mfun->isFixed(0));
    TS_ASSERT( ! mfun->isFixed(1));
    TS_ASSERT(   mfun->isFixed(2));

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->getParameter("f0.c"),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.h"),1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.s"),0.3);

    delete mfun;
  }

  void testReplaceEmptyFunction()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g = new Gauss();
    CompositeFunction *cf = new CompositeFunction();
    Linear *bk = new Linear();
    Cubic *cub = new Cubic();

    mfun->addFunction(bk);
    mfun->addFunction(cf);
    mfun->addFunction(cub);

    mfun->replaceFunctionPtr(cf,g);

    TS_ASSERT_EQUALS(mfun->asString(),"name=Linear,a=0,b=0;name=Gauss,c=0,h=1,s=1;name=Cubic,c0=0,c1=0,c2=0,c3=0");

    delete mfun;
  }

  void test_setWorkspaceWorks()
  {
    CompositeFunction *mfun = new CompositeFunction();
    Gauss *g1 = new Gauss();
    Linear *bk = new Linear();

    mfun->addFunction(bk);
    mfun->addFunction(g1);

    MatrixWorkspace_sptr ws(new CompositeFunctionTest_MocMatrixWorkspace(10,11,10));

    MantidVec& x = ws->dataX(3);
    MantidVec& y = ws->dataY(3);
    for(size_t i=0; i < y.size(); ++i)
    {
      x[i] = 0.1 * static_cast<double>(i);
      y[i] = static_cast<double>(i);
    }
    x.back() = 0.1 * static_cast<double>(y.size());

    // Failing to explicitly construct a string here from the char* leads to the
    //   setWorkspace(Workspace_sptr, bool) overload getting called!!!
    //TS_ASSERT_THROWS_NOTHING(mfun->setWorkspace(ws,std::string("WorkspaceIndex=3,StartX=0.2,EndX = 0.8")));
  }

private:
  bool isDeleted(CompositeFunctionTest_IFunction* f)
  {
    std::vector<CompositeFunctionTest_IFunction*>::iterator it;
    it = std::find(CompositeFunctionTest_FunctionDeleted.begin(),CompositeFunctionTest_FunctionDeleted.end(),f);
    return it != CompositeFunctionTest_FunctionDeleted.end();
  }
  void clearDeleted()
  {
    CompositeFunctionTest_FunctionDeleted.clear();
  }

};

#endif /*COMPOSITEFUNCTIONTEST_H_*/
