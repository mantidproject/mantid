#ifndef TEST_IMPLICIT_FUNCTION_PARSERS_H_
#define TEST_IMPLICIT_FUNCTION_PARSERS_H_

//Abstract testing base class for function parsers.

#include <memory>
#include "NormalParameterParser.h"
#include "OriginParameterParser.h"
#include "InvalidParameterParser.h"
#include "PlaneFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionParser.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class  FunctionParserTest 
{
protected:

    //Mock function parser class.
    class MockFunctionParser : public Mantid::API::ImplicitFunctionParser
    {
    public:
        MockFunctionParser(Mantid::API::ImplicitFunctionParameterParser* paramParser) : Mantid::API::ImplicitFunctionParser(paramParser) { ; }
        MOCK_METHOD1(createFunctionBuilder, Mantid::API::ImplicitFunctionBuilder*(Poco::XML::Element* functionElement));
        MOCK_METHOD1(setSuccessorParser, void(Mantid::API::ImplicitFunctionParser* parameterElement));
    };

    //Mock parameter parser class.
    class MockParameterParser : public Mantid::API::ImplicitFunctionParameterParser
    {
    public:
        MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter*(Poco::XML::Element* parameterElement));
        MOCK_METHOD1(setSuccessorParser, void(Mantid::API::ImplicitFunctionParameterParser* parameterParser));
    };
	
    //helper method to construct real parameter parser chain.
    std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser> constructRootParameterParser()
    {
        using namespace Mantid::MDAlgorithms;
		using namespace Mantid::API;
        std::auto_ptr<ImplicitFunctionParameterParser> originParser = std::auto_ptr<ImplicitFunctionParameterParser>(new OriginParameterParser);
        std::auto_ptr<ImplicitFunctionParameterParser> normalParser = std::auto_ptr<ImplicitFunctionParameterParser>(new NormalParameterParser);
        std::auto_ptr<ImplicitFunctionParameterParser> invalidParser = std::auto_ptr<ImplicitFunctionParameterParser>(new InvalidParameterParser);

        originParser->setSuccessorParser(invalidParser.release());
        normalParser->setSuccessorParser(originParser.release());

        return normalParser;
    }

};

#endif