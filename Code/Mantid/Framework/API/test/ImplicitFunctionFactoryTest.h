#ifndef IMPLICIT_FUNCTION_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <memory>

class ImplicitFunctionFactoryTest : public CxxTest::TestSuite
{
private:

  class MockImplicitFunctionA : public Mantid::API::ImplicitFunction
  {
  public:
    MOCK_CONST_METHOD1(evaluate, bool(const Mantid::API::Point3D* pPoint3D));
    virtual std::string getName() const
    {
      return "MockImplicitFunctionA";
    }
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunctionA()   {;}
  };

  class MockImplicitFunctionB : public Mantid::API::ImplicitFunction
  {
  public:
    MOCK_CONST_METHOD1(evaluate, bool(const Mantid::API::Point3D* pPoint3D));
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunctionB()   {;}
  };

  class MockImplicitFunctionParserA : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParserA() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParserA)
    {
    }
    Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      return new MockImplicitFunctionBuilderA;
    }
     void setSuccessorParser(Mantid::API::ImplicitFunctionParser* successor)
    {
      m_successor = std::auto_ptr<Mantid::API::ImplicitFunctionParser>(successor);
    }
    void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
    {
      m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
    }
  };

  class MockImplicitFunctionParserB : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParserB() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParserB)
    {
    }
    Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      return new MockImplicitFunctionBuilderB;
    }
     void setSuccessorParser(Mantid::API::ImplicitFunctionParser* successor)
    {
      m_successor = std::auto_ptr<Mantid::API::ImplicitFunctionParser>(successor);
    }
    void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
    {
      m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
    }
  };

  class MockImplicitFunctionParameterParserA : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
    MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter*(Poco::XML::Element* functionElement));
    MOCK_METHOD1(setSuccessorParser, void(Mantid::API::ImplicitFunctionParameterParser* successor));
  }; 

  class MockImplicitFunctionParameterParserB : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
    MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter*(Poco::XML::Element* functionElement));
    MOCK_METHOD1(setSuccessorParser, void(Mantid::API::ImplicitFunctionParameterParser* successor));
  };

  class MockImplicitFunctionBuilderA : public Mantid::API::ImplicitFunctionBuilder
  {
  public:
    Mantid::API::ImplicitFunction* create() const
    {
      return new MockImplicitFunctionA;
    }
  };

  class MockImplicitFunctionBuilderB : public Mantid::API::ImplicitFunctionBuilder
  {
  public:
    Mantid::API::ImplicitFunction* create() const
    {
      return new MockImplicitFunctionA;
    }
  };

  //Helper method to generate as simple xml fragment.
  static std::string generateSimpleXML()
  {
    return std::string("<Function>")+
      "<Type>MockA1ImplicitFunction</Type>"+
      "<ParameterList>"+
      "<Parameter>"+
      "<Type>MockA1ImplicitFunctionParameter</Type>"+
      "<Value></Value>"+
      "</Parameter>"+
      "</ParameterList>"+
     "</Function>";
  }

  //Helper method providing a more complex xml fragment.
  static std::string generateComplexXML()
  {
    return std::string("<Function>")+
      "<Type>MockA1ImplicitFunction</Type>"+
      "<Function>"+
        "<Type>MockB1ImplicitFunction</Type>"+
        "<ParameterList>"+
           "<Parameter>"+
             "<Type>MockB1ImplicitFunctionParameter</Type>"+
             "<Value></Value>"+
           "</Parameter>"+
         "</ParameterList>"+
      "</Function>"+
      "<ParameterList>"+
        "<Parameter>"+
          "<Type>MockA1ImplicitFunctionParameter</Type>"+
          "<Value></Value>"+
        "</Parameter>"+
      "</ParameterList>"+
    "</Function>";
  }



public:
  void testSetup()
  {
    using namespace Mantid::Kernel;
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunctionA> >("MockA1ImplicitFunction"); //No warnings if used.
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunctionB> >("MockB1ImplicitFunction"); //Emit warnings if used.
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunctionParameterParserA> >("MockA1ImplicitFunctionParameterParser"); //No warnings if used.
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunctionParameterParserB> >("MockB1ImplicitFunctionParameterParser"); //Emit warnings if used.
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserA >("MockA1ImplicitFunctionParser"); //No warnings if used.
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunctionParserB> >("MockB1ImplicitFunctionParser"); //Emit warnings if used.

  }

  //Test a simple exaple using with one 
  void testCreateunwrappedSimple()
  {
    using Mantid::API::ImplicitFunction;
    boost::scoped_ptr<ImplicitFunction> function(Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(generateComplexXML()));
    
    TSM_ASSERT_EQUALS("The correct implicit function type has not been generated", "MockImplicitFunctionA", function->getName());
  }



  void testCreateThrows()
  {
    TSM_ASSERT_THROWS("Should have thrown exeption on use of create rather than createunwrapped.", Mantid::API::ImplicitFunctionFactory::Instance().create(""), std::runtime_error );
  }


};

#endif 
