#ifndef TABULATEDFUNCTIONTEST_H_
#define TABULATEDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/TabulatedFunction.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FunctionFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>


#include <fstream>

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

namespace
{
  struct Fun
  {
    double operator()(double x,int i)
    {
        return exp(-x*x) + double(i);
    }
  };
}

class TabulatedFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TabulatedFunctionTest *createSuite() { return new TabulatedFunctionTest(); }
  static void destroySuite( TabulatedFunctionTest *suite ) { delete suite; }

  TabulatedFunctionTest():
  m_asciiFileName("TabulatedFunctionTest_testAsciiFile.txt"),
  m_nexusFileName(Mantid::API::FileFinder::Instance().getFullPath("argus0026287.nxs"))
  {
    FunctionDomain1DVector x(-5.0, 5.0, 100);
    FunctionValues y( x );
    UserFunction fun;
    fun.setAttributeValue("Formula","exp(-x*x)");
    fun.function( x, y );
    
    std::ofstream fil(m_asciiFileName.c_str());
    for(size_t i = 0; i < x.size(); ++i)
    {
      fil << x[i] << ' ' << y[i] << std::endl;
    }

  }
  
  ~TabulatedFunctionTest()
  {
    Poco::File hAscii(m_asciiFileName);
    if( hAscii.exists() ) 
    {
      hAscii.remove();
    }
  }

  void test_loadAscii()
  {
    TS_ASSERT( true );
    TabulatedFunction fun;
    fun.setAttributeValue("FileName",m_asciiFileName);
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 1.0 );
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y( x );
    fun.function( x, y );
    for(size_t i = 0; i < x.size(); ++i)
    {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.06;
      TS_ASSERT_DELTA( fabs(y[i] - exp(-xx*xx))/y[i], 0, tol );
    }
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), m_asciiFileName);
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS( fun.getAttribute("WorkspaceIndex").asInt(), 0);
  }

  void test_loadNexus()
  {
    TS_ASSERT( true );
    TabulatedFunction fun;
    fun.setAttributeValue("FileName",m_nexusFileName);
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 1.0 );
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), m_nexusFileName);
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS( fun.getAttribute("WorkspaceIndex").asInt(), 0);

    FunctionDomain1DVector x(1.0, 30.0, 100);
    FunctionValues y( x );
    fun.function( x, y );

    TS_ASSERT_DELTA( y[5], 304.8886, 1e-4 );
    TS_ASSERT_DELTA( y[10], 136.7575, 1e-4 );
    TS_ASSERT_DELTA( y[20], 32.4847, 1e-4 );
    TS_ASSERT_DELTA( y[25], 16.8940, 1e-4 );
    TS_ASSERT_DELTA( y[30], 9.2728, 1e-4 );
  }

  void test_loadNexus_nondefault_index()
  {
    TS_ASSERT( true );
    TabulatedFunction fun;
    fun.setAttributeValue("FileName",m_nexusFileName);
    fun.setAttributeValue("WorkspaceIndex",10);
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 1.0 );
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), m_nexusFileName);
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS( fun.getAttribute("WorkspaceIndex").asInt(), 10);

    FunctionDomain1DVector x(1.0, 30.0, 100);
    FunctionValues y( x );
    fun.function( x, y );

    TS_ASSERT_DELTA( y[5], 367.2980, 1e-4 );
    TS_ASSERT_DELTA( y[10], 179.5151, 1e-4 );
    TS_ASSERT_DELTA( y[20], 50.4847, 1e-4 );
    TS_ASSERT_DELTA( y[25], 21.2980, 1e-4 );
    TS_ASSERT_DELTA( y[30], 17.4847, 1e-4 );
  }

  void test_loadWorkspace()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),1,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "TABULATEDFUNCTIONTEST_WS", ws );
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace","TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 1.0 );
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y( x );
    fun.function( x, y );
    for(size_t i = 0; i < x.size(); ++i)
    {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.07;
      TS_ASSERT_DELTA( fabs(y[i] - exp(-xx*xx))/y[i], 0, tol );
    }
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), "");
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspace_nondefault_index()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),3,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "TABULATEDFUNCTIONTEST_WS", ws );
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace","TABULATEDFUNCTIONTEST_WS");
    fun.setAttributeValue("WorkspaceIndex",2);
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 1.0 );
    TS_ASSERT_EQUALS( fun.getParameter( "Shift" ), 0.0 );
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y( x );
    fun.function( x, y );
    for(size_t i = 0; i < x.size(); ++i)
    {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.07;
      TS_ASSERT_DELTA( fabs(y[i] - exp(-xx*xx) - 2.0 )/y[i], 0, tol );
    }
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS( fun.getAttribute("WorkspaceIndex").asInt(), 2);
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), "");
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspace_nondefault_wrong_index()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),3,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "TABULATEDFUNCTIONTEST_WS", ws );
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace","TABULATEDFUNCTIONTEST_WS");
    fun.setAttributeValue("WorkspaceIndex",20);
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y( x );
    TS_ASSERT_THROWS( fun.function( x, y ), std::range_error );
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspaceWhichDoesNotExist()
  {
    TabulatedFunction fun;
    TS_ASSERT_THROWS( fun.setAttributeValue("Workspace","SomeWorkspace"), Mantid::Kernel::Exception::NotFoundError );
  }

  void test_Derivatives()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),1,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "TABULATEDFUNCTIONTEST_WS", ws );
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace","TABULATEDFUNCTIONTEST_WS");
    fun.setParameter( "Scaling", 3.3 );
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 3.3 );
    fun.setParameter( "Shift", 0.0 );
    TS_ASSERT_EQUALS( fun.getParameter( "Shift" ), 0.0 );
    FunctionDomain1DVector x(-5.0, 5.0, 83);

    FunctionValues y( x );
    fun.function( x, y );

    Mantid::CurveFitting::Jacobian jac(x.size(),2);
    fun.functionDeriv(x, jac);

    for(size_t i = 0; i < x.size(); ++i)
    {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.07;
      TS_ASSERT_DELTA( fabs(y[i] - 3.3*exp(-xx*xx))/y[i], 0, tol );
      TS_ASSERT_DELTA( fabs(jac.get(i,0) - exp(-xx*xx))/y[i], 0, tol );
      //std::cerr << xx << ' ' << y[i] << std::endl;
    }
    AnalysisDataService::Instance().clear();
  }

  void test_Attributes()
  {
    TabulatedFunction fun;
    auto names = fun.getAttributeNames();
    TS_ASSERT_EQUALS( names.size(), 3 );
    TS_ASSERT_EQUALS( names[0], "FileName" );
    TS_ASSERT_EQUALS( names[1], "Workspace" );
    TS_ASSERT_EQUALS( names[2], "WorkspaceIndex" );
    TS_ASSERT( fun.hasAttribute("FileName") );
    TS_ASSERT( fun.hasAttribute("Workspace") );
    TS_ASSERT( fun.hasAttribute("WorkspaceIndex") );
  }

  void test_factory_create_from_file()
  {
      std::string inif = "name=TabulatedFunction,FileName=\"" + m_nexusFileName + "\",WorkspaceIndex=17,Scaling=2,Shift=0.02";
      auto funf = Mantid::API::FunctionFactory::Instance().createInitialized(inif);
      TS_ASSERT( funf );
      TS_ASSERT_EQUALS( funf->getAttribute("Workspace").asString(), "");
      TS_ASSERT_EQUALS( funf->getAttribute("WorkspaceIndex").asInt(), 17);
      TS_ASSERT_EQUALS( funf->getAttribute("FileName").asUnquotedString(), m_nexusFileName);
      TS_ASSERT_EQUALS( funf->getParameter("Scaling"), 2.0);
      TS_ASSERT_EQUALS( funf->getParameter("Shift"), 0.02);
  }

  void test_factory_create_from_workspace()
  {
      auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),1,-5.0,5.0,0.1,false);
      AnalysisDataService::Instance().add( "TABULATEDFUNCTIONTEST_WS", ws );
      std::string inif = "name=TabulatedFunction,Workspace=TABULATEDFUNCTIONTEST_WS,WorkspaceIndex=71,Scaling=3.14,Shift=0.02";
      auto funf = Mantid::API::FunctionFactory::Instance().createInitialized(inif);
      TS_ASSERT( funf );
      TS_ASSERT_EQUALS( funf->getAttribute("Workspace").asString(), "TABULATEDFUNCTIONTEST_WS");
      TS_ASSERT_EQUALS( funf->getAttribute("WorkspaceIndex").asInt(), 71);
      TS_ASSERT_EQUALS( funf->getAttribute("FileName").asUnquotedString(), "");
      TS_ASSERT_EQUALS( funf->getParameter("Scaling"), 3.14);
      TS_ASSERT_EQUALS( funf->getParameter("Shift"), 0.02);
      AnalysisDataService::Instance().clear();
  }

private:
  const std::string m_asciiFileName;
  const std::string m_nexusFileName;
};

#endif /*TABULATEDFUNCTIONTEST_H_*/
