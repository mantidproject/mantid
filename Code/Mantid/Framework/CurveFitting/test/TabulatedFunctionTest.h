#ifndef TABULATEDFUNCTIONTEST_H_
#define TABULATEDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/TabulatedFunction.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>


#include <fstream>

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

namespace
{
  struct Fun
  {
    double operator()(double x,int)
    {
      return exp(-x*x);
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
  m_nexusFileName("TabulatedFunctionTest_testNexusFile.nxs")
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

    //  SaveNexusProcessed cannnot be found, I don't know why
    //auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),1,-5.0,5.0,0.1,false);
    //if ( !ws ) throw std::runtime_error("WS not created");
    //auto save = AlgorithmFactory::Instance().create("SaveNexusProcessed");
    //if ( !save ) throw std::runtime_error("Algo not created");
    //save->setProperty("Filename",m_nexusFileName);
    //save->setProperty("InputWorkspace",ws);
    //save->execute();
  }
  
  ~TabulatedFunctionTest()
  {
    Poco::File hAscii(m_asciiFileName);
    if( hAscii.exists() ) 
    {
      hAscii.remove();
    }
    //Poco::File hNexus(m_nexusFileName);
    //if( hNexus.exists() ) 
    //{
    //  hNexus.remove();
    //}
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
      //std::cerr << xx << ' ' << y[i] << std::endl;
    }
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), "");
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspaceWhichDoesNotExist()
  {
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace","SomeWorkspace");
    TS_ASSERT_EQUALS( fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS( fun.getAttribute("FileName").asUnquotedString(), "");
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 1.0 );
    FunctionDomain1DVector x(-5.0, 5.0, 10);
    FunctionValues y( x );
    fun.function( x, y );
    for(size_t i = 0; i < x.size(); ++i)
    {
      TS_ASSERT_EQUALS( y[i], 0 );
    }
  }

  void test_Derivatives()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),1,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "TABULATEDFUNCTIONTEST_WS", ws );
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace","TABULATEDFUNCTIONTEST_WS");
    fun.setParameter( "Scaling", 3.3 );
    TS_ASSERT_EQUALS( fun.getParameter( "Scaling" ), 3.3 );
    FunctionDomain1DVector x(-5.0, 5.0, 83);

    FunctionValues y( x );
    fun.function( x, y );

    Mantid::CurveFitting::Jacobian jac(x.size(),1);
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

private:
  const std::string m_asciiFileName;
  const std::string m_nexusFileName;
};

#endif /*TABULATEDFUNCTIONTEST_H_*/
