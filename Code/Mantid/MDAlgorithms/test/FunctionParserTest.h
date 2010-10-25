#ifndef TEST_IMPLICIT_FUNCTION_PARSERS_H_
#define TEST_IMPLICIT_FUNCTION_PARSERS_H_

//Abstract testing base class for function parsers.

#include <memory>
#include "FunctionParserTest.h"
#include "PlaneFunctionParser.h"
#include "NormalParameterParser.h"
#include "OriginParameterParser.h"
#include "InvalidParameterParser.h"
#include "PlaneFunctionBuilder.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class  FunctionParserTest 
{
protected:

    //Mock function parser class.
    class MockFunctionParser : public Mantid::MDAlgorithms::FunctionParser
    {
    public:
        MockFunctionParser(Mantid::MDAlgorithms::ParameterParser* paramParser) : Mantid::MDAlgorithms::FunctionParser(paramParser) { ; }
        MOCK_METHOD1(createFunctionBuilder, Mantid::MDAlgorithms::IFunctionBuilder*(Poco::XML::Element* functionElement));
        MOCK_METHOD1(setSuccessorParser, void(Mantid::MDAlgorithms::FunctionParser* parameterElement));
    };

    //Mock parameter parser class.
    class MockParameterParser : public Mantid::MDAlgorithms::ParameterParser 
    {
    public:
        MOCK_METHOD1(createParameter, Mantid::MDAlgorithms::IParameter*(Poco::XML::Element* parameterElement));
        MOCK_METHOD1(setSuccessorParser, void(Mantid::MDAlgorithms::ParameterParser* parameterParser));
    };
	
    //helper method to construct real parameter parser chain.
    std::auto_ptr<Mantid::MDAlgorithms::ParameterParser> constructRootParameterParser()
    {
        using namespace Mantid::MDAlgorithms;
        std::auto_ptr<ParameterParser> originParser = std::auto_ptr<ParameterParser>(new OriginParameterParser);
        std::auto_ptr<ParameterParser> normalParser = std::auto_ptr<ParameterParser>(new NormalParameterParser);
        std::auto_ptr<ParameterParser> invalidParser = std::auto_ptr<ParameterParser>(new InvalidParameterParser);

        originParser->setSuccessorParser(invalidParser.release());
        normalParser->setSuccessorParser(originParser.release());

        return normalParser;
    }

};

#endif