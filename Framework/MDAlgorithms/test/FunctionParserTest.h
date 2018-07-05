#ifndef TEST_IMPLICIT_FUNCTION_PARSERS_H_
#define TEST_IMPLICIT_FUNCTION_PARSERS_H_

// Abstract testing base class for function parsers.

#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include "MantidMDAlgorithms/MDParameterParserDeclarations.h"
#include "MantidMDAlgorithms/PlaneFunctionBuilder.h"
#include "MantidMDAlgorithms/Vector3DParameterParser.h"
#include <boost/scoped_ptr.hpp>

#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidAPI/ImplicitFunctionParser.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class FunctionParserTest {
protected:
  // Mock function parser class.
  class MockFunctionParser : public Mantid::API::ImplicitFunctionParser {
  public:
    MockFunctionParser(
        Mantid::API::ImplicitFunctionParameterParser *paramParser)
        : Mantid::API::ImplicitFunctionParser(paramParser) {
      ;
    }
    MOCK_METHOD1(createFunctionBuilder,
                 Mantid::API::ImplicitFunctionBuilder *(
                     Poco::XML::Element *functionElement));
    MOCK_METHOD1(setSuccessorParser,
                 void(Mantid::API::ImplicitFunctionParser *parameterElement));
    MOCK_METHOD1(
        setParameterParser,
        void(Mantid::API::ImplicitFunctionParameterParser *parameterElement));
  };

  // Mock parameter parser class.
  class MockParameterParser
      : public Mantid::API::ImplicitFunctionParameterParser {
  public:
    MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter *(
                                      Poco::XML::Element *parameterElement));
    MOCK_METHOD1(
        setSuccessorParser,
        void(Mantid::API::ImplicitFunctionParameterParser *parameterParser));
  };

  // helper method to construct real parameter parser chain.
  Mantid::API::ImplicitFunctionParameterParser *constructRootParameterParser() {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;
    ImplicitFunctionParameterParser *originParser = new OriginParameterParser;
    ImplicitFunctionParameterParser *upParser = new UpParameterParser;
    ImplicitFunctionParameterParser *normalParser = new NormalParameterParser;
    ImplicitFunctionParameterParser *invalidParser = new InvalidParameterParser;
    ImplicitFunctionParameterParser *widthParser = new WidthParameterParser;

    widthParser->setSuccessorParser(invalidParser);
    upParser->setSuccessorParser(widthParser);
    originParser->setSuccessorParser(upParser);
    normalParser->setSuccessorParser(originParser);

    return normalParser;
  }
};

#endif
