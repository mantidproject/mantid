#ifndef IMPLICIT_FUNCTION_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include <boost/shared_ptr.hpp>


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

  class MockImplicitFunctionParserA : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParserA() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParserA) {}

    virtual Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      return new MockImplicitFunctionBuilder(new MockImplicitFunctionA);	
    }
    virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParser* successor)
    {
      m_successor = std::auto_ptr<Mantid::API::ImplicitFunctionParser>(successor);
    }
    virtual void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
    {
      m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
    }
  };

  class MockImplicitFunctionParserB : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParserB() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParserB) {}

    virtual Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      return new MockImplicitFunctionBuilder(new MockImplicitFunctionB);	
    }
    virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParser* successor)
    {
      m_successor = std::auto_ptr<Mantid::API::ImplicitFunctionParser>(successor);
    }
    virtual void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
    {
      m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
    }
  };

  class MockImplicitFunctionParameterParserA : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
    virtual Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* functionElement)
    {
      throw std::logic_error("Mock, so doesn't actually perform creation");
    }
    virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* successor)
    {
      m_successor = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(successor);
    }
  }; 

  class MockImplicitFunctionParameterParserB : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
    virtual Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* functionElement)
    {
      throw std::logic_error("Mock, so doesn't actually perform creation");	
    }
    virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* successor)
    {
      m_successor = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(successor);
    }
  };

  class MockImplicitFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
  {
  private:
    Mantid::API::ImplicitFunction* m_return;
  public:
    MockImplicitFunctionBuilder(Mantid::API::ImplicitFunction* preturn): m_return(preturn){}
    Mantid::API::ImplicitFunction* create() const
    {
      return m_return;
    }
  };

  //helper method;
  std::string getXMLInstructions()
  {
      return std::string("<Function>") +
      "<Name>PlaneImplicitFunction</Name>" +
      "<ParameterList>" +
      "<Parameter>" +
      "<Name>Normal</Name>" +
      "<Type>Vector</Type>" +
      "<Value>1 1 1</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Name>Origin</Name>" +
      "<Type>Vector</Type>" +
      "<Value>1 0 0</Value>" +
      "</Parameter>" +
      "</ParameterList>" +
      "</Function>";
  }

  //helper method
  std::string getXMLLanguageDef()
  {
      return std::string("<Factories>") +
      "<FunctionParserFactoryList>" +
      "<FunctionParserFactory>MockImplicitFunctionParserA1</FunctionParserFactory>" +
      "<FunctionParserFactory>MockImplicitFunctionParserB1</FunctionParserFactory>" +
      "</FunctionParserFactoryList>" +
      "<ParameterParserFactoryList>" +
      "<ParameterParser>MockImplicitFunctionParameterParserA1</ParameterParser>" +
      "<ParameterParser>MockImplicitFunctionParameterParserB1</ParameterParser>" +
      "</ParameterParserFactoryList>" +
      "</Factories>";
  }


public:
  void testSetup()
  {
    using namespace Mantid::Kernel;

    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<MockImplicitFunctionA>("MockImplicitFunctionA1");
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<MockImplicitFunctionB>("MockImplicitFunctionB1");
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<MockImplicitFunctionParameterParserA>("MockImplicitFunctionParameterParserA1");
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<MockImplicitFunctionParameterParserB>("MockImplicitFunctionParameterParserB1");
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserA>("MockImplicitFunctionParserA1");
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserB>("MockImplicitFunctionParserB1");
  }


  void testCreateunwrapped()
  {
    //std::string s;
    //std::getline(std::cin,s);

    Mantid::API::ImplicitFunction* function = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(getXMLLanguageDef(), "<Function>Function</Function>");
    
    TSM_ASSERT_EQUALS("The correct implicit function type has not been generated", "MockImplicitFunctionA", function->getName());
    delete function;
  }

  void testCreateThrows()
  {
    TSM_ASSERT_THROWS("Should have thrown exeption on use of create rather than createunwrapped.", Mantid::API::ImplicitFunctionFactory::Instance().create(""), std::runtime_error );
  }

  void testHandleInvalidDefXML()
  {
    //Mantid::API::ImplicitFunction* function = 
    TSM_ASSERT_THROWS("Should have thrown exeption on invalid definition xml.", Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped("<OtherXML></OtherXML>", "<Function></Function>"), std::runtime_error );
  }

};

#endif 
