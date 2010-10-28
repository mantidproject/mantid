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
#include "boost/smart_ptr/shared_ptr.hpp"


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
    virtual void setSuccessorParser(ImplicitFunctionParser* parser)
	{
	}
  };
  
  class MockImplicitFunctionParserB : public Mantid::API::ImplicitFunctionParser
  {
    public:
	MockImplicitFunctionParserB() : Mantid::API::ImplicitFunctionParser(new MockImplicitFunctionParameterParser) {}

    virtual Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
	{
        return NULL;	
	}
    virtual void setSuccessorParser(ImplicitFunctionParser* parser)
	{
	}
  };
  

public:
  
  void testSetup()
  {
	Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserA>("MockImplicitFunctionParserA");
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParserB>("MockImplicitFunctionParserB");
  }
  
  void testGetFirstConcreteInstance()
  {
      boost::shared_ptr<Mantid::API::ImplicitFunctionParser> parser = Mantid::API::ImplicitFunctionParserFactory::Instance().create("MockImplicitFunctionParserA");
	  MockImplicitFunctionParserA* a = dynamic_cast<MockImplicitFunctionParserA*>(parser.get());
	  TSM_ASSERT("The correct implicit parserparameter parser type has not been generated",  NULL != a);
  }
  
  void testGetSecondConcreteInstance()
  {
      boost::shared_ptr<Mantid::API::ImplicitFunctionParser> parser = Mantid::API::ImplicitFunctionParserFactory::Instance().create("MockImplicitFunctionParserB");
	  MockImplicitFunctionParserB* b = dynamic_cast<MockImplicitFunctionParserB*>(parser.get());
	  TSM_ASSERT("The correct implicit parserparameter parser type has not been generated",  NULL != b);
  }

};

#endif 
