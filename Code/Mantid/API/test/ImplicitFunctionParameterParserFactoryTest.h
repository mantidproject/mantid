#ifndef IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include <boost/shared_ptr.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ImplicitFunctionParameterParserFactoryTest : public CxxTest::TestSuite
{
  private:
  
  class MockImplicitFunctionParameter : public Mantid::API::ImplicitFunctionParameter
  {
  public:
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(isValid, bool());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunctionParameter(){}
    protected:
    virtual ImplicitFunctionParameter* clone() const 
    { 
      return new MockImplicitFunctionParameter;
    }
  };
  
  //TODO, use mocking framework instead!
  class MockImplicitFunctionParameterParserA : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
      virtual Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement)
	  {
	      return new MockImplicitFunctionParameter;
	  }
      virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser)
	  {
	  }
  };
  
  class MockImplicitFunctionParameterParserB : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
      virtual Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement)
	  {
	      return new MockImplicitFunctionParameter;
	  }
      virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser)
	  {
	  }
  };
  

public:
  void testSetup()
  {

	Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<MockImplicitFunctionParameterParserA>("MockImplicitFunctionParameterParserA");
    Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<MockImplicitFunctionParameterParserB>("MockImplicitFunctionParameterParserB");
  }
  
  void testGetFirstConcreteInstance()
  {
      Mantid::API::ImplicitFunctionParameterParser* parser = Mantid::API::ImplicitFunctionParameterParserFactory::Instance().createUnwrapped("MockImplicitFunctionParameterParserA");
	  MockImplicitFunctionParameterParserA* a = dynamic_cast<MockImplicitFunctionParameterParserA*>(parser);
	  TSM_ASSERT("The correct implicit implicit function parameter parser type has not been generated",  NULL != a);
	  delete parser;
  }
  
  void testGetSecondConcreteInstance()
  {
      Mantid::API::ImplicitFunctionParameterParser* parser = Mantid::API::ImplicitFunctionParameterParserFactory::Instance().createUnwrapped("MockImplicitFunctionParameterParserB");
	  MockImplicitFunctionParameterParserB* b = dynamic_cast<MockImplicitFunctionParameterParserB*>(parser);
	  TSM_ASSERT("The correct implicit function parameter parser type has not been generated",  NULL != b);
	  delete parser;
  }
  
  void testCreateThrows()
  {
    TSM_ASSERT_THROWS("Should have thrown exception on use of create rather than createunwrapped.", Mantid::API::ImplicitFunctionParameterParserFactory::Instance().create(""), std::runtime_error );
  }

};

#endif 
