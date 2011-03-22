#ifndef IMPLICIT_FUNCTION_PARSER_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_PARSER_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidAPI/ImplicitFunctionParser.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include <boost/shared_ptr.hpp>


class ImplicitFunctionParserFactoryTest : public CxxTest::TestSuite
{
  private:
  
  //TODO, use mocking framework instead!
  class MockImplicitFunctionParameterParser : public Mantid::API::ImplicitFunctionParameterParser
  {
  public:
      virtual Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement)
	  {
	      return NULL;
	  }
      virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser)
	  {
	  }
  };
  
  class MockImplicitFunctionParserA : public Mantid::API::ImplicitFunctionParser
  {
    public:
	MockImplicitFunctionParserA() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParser) {}

    virtual Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
	{
        return NULL;	
	}
    virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParser* parser){}
	virtual void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser){}
  };
  
  class MockImplicitFunctionParserB : public Mantid::API::ImplicitFunctionParser
  {
    public:
	MockImplicitFunctionParserB() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParser) {}

    virtual Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
	{
        return NULL;	
	}
    virtual void setSuccessorParser(Mantid::API::ImplicitFunctionParser* parser){}
	virtual void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser){}
  };
  

public:
  
  void testSetup()
  {
	Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserA>("MockImplicitFunctionParserA");
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserB>("MockImplicitFunctionParserB");
  }
  
  void testGetFirstConcreteInstance()
  {
      Mantid::API::ImplicitFunctionParser* parser = Mantid::API::ImplicitFunctionParserFactory::Instance().createUnwrapped("MockImplicitFunctionParserA");
	  MockImplicitFunctionParserA* a = dynamic_cast<MockImplicitFunctionParserA*>(parser);
	  TSM_ASSERT("The correct implicit parserparameter parser type has not been generated",  NULL != a);
	  delete parser;
  }
  
  void testGetSecondConcreteInstance()
  {
      Mantid::API::ImplicitFunctionParser* parser = Mantid::API::ImplicitFunctionParserFactory::Instance().createUnwrapped("MockImplicitFunctionParserB");
	  MockImplicitFunctionParserB* b = dynamic_cast<MockImplicitFunctionParserB*>(parser);
	  TSM_ASSERT("The correct implicit parserparameter parser type has not been generated",  NULL != b);
	  delete parser;
  }
  
  void testCreateThrows()
  {
    TSM_ASSERT_THROWS("Should have thrown exception on use of create rather than createunwrapped.", Mantid::API::ImplicitFunctionParserFactory::Instance().create(""), std::runtime_error );
  }

};

#endif 
