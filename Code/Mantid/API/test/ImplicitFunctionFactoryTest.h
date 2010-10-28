#ifndef IMPLICIT_FUNCTION_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ImplicitFunction.h"
#include "boost/smart_ptr/shared_ptr.hpp"


class ImplicitFunctionFactoryTest : public CxxTest::TestSuite
{
  private:
  
  //TODO, use mocking framework instead!
  class MockImplicitFunctionA : public Mantid::API::ImplicitFunction
  {
  public:
     bool evaluate(const Mantid::API::Point3D* pPoint3D) const
	 { 
	    return true;
	 }
     std::string getName() const
	 {
	    return "MockImplicitFunctionA";
	 }
     std::string toXMLString() const
	 {
	    return "";
	 }
     ~MockImplicitFunctionA()   {;}
  };
  
  //TODO, use mocking framework instead!
  class MockImplicitFunctionB : public Mantid::API::ImplicitFunction
  {
  public:
     bool evaluate(const Mantid::API::Point3D* pPoint3D) const
	 { 
	    return true;
	 }
     std::string getName() const
	 {
	    return "MockImplicitFunctionB";
	 }
     std::string toXMLString() const
	 {
	    return "";
	 }
     ~MockImplicitFunctionB()   {;}
  };


public:
  void testSetup()
  {
    using namespace Mantid::Kernel;
	
	Mantid::API::ImplicitFunctionFactory::Instance().subscribe<MockImplicitFunctionA>("MockImplicitFunctionA");
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<MockImplicitFunctionB>("MockImplicitFunctionB");
  }
  
  void testGetFirstConcreteInstance()
  {
      boost::shared_ptr<Mantid::API::ImplicitFunction> function = Mantid::API::ImplicitFunctionFactory::Instance().create("MockImplicitFunctionA");
	  TSM_ASSERT_EQUALS("The correct implicit function type has not been generated", "MockImplicitFunctionA", function->getName());
  }
  
  void testGetSecondConcreteInstance()
  {
      boost::shared_ptr<Mantid::API::ImplicitFunction> function = Mantid::API::ImplicitFunctionFactory::Instance().create("MockImplicitFunctionB");
	  TSM_ASSERT_EQUALS("The correct implicit function type has not been generated", "MockImplicitFunctionB", function->getName());
  }

};

#endif 
