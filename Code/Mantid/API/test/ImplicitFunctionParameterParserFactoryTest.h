#ifndef IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_TEST_H_
#define IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include <boost/shared_ptr.hpp>


class ImplicitFunctionParameterParserFactoryTest : public CxxTest::TestSuite
{
  private:
  
  class MockImplicitFunctionParameter : public Mantid::API::ImplicitFunctionParameter
  {
    std::string getName() const
	{
	  return "MockImplicitFunctionParameter";
	}
	bool isValid() const
	{
	  return true;
	}

    virtual std::string toXMLString() const
	{
	  return "";
	}	

	protected:
            virtual ImplicitFunctionParameter* cloneImp() const { return new MockImplicitFunctionParameter;}
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
      boost::shared_ptr<Mantid::API::ImplicitFunctionParameterParser> parser = Mantid::API::ImplicitFunctionParameterParserFactory::Instance().create("MockImplicitFunctionParameterParserA");
	  MockImplicitFunctionParameterParserA* a = dynamic_cast<MockImplicitFunctionParameterParserA*>(parser.get());
	  TSM_ASSERT("The correct implicit implicit function parameter parser type has not been generated",  NULL != a);
  }
  
  void testGetSecondConcreteInstance()
  {
      boost::shared_ptr<Mantid::API::ImplicitFunctionParameterParser> parser = Mantid::API::ImplicitFunctionParameterParserFactory::Instance().create("MockImplicitFunctionParameterParserB");
	  MockImplicitFunctionParameterParserB* b = dynamic_cast<MockImplicitFunctionParameterParserB*>(parser.get());
	  TSM_ASSERT("The correct implicit function parameter parser type has not been generated",  NULL != b);
  }

};

#endif 
