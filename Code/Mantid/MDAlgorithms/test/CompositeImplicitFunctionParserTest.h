#ifndef TEST_COMPOSITE_FUNCTION_PARSER_H_
#define TEST_COMPOSITE_FUNCTION_PARSER_H_

#include "FunctionParserTest.h"
#include <vector>
#include <memory>
#include <boost/scoped_ptr.hpp>

#include "InvalidParameterParser.h"
#include "CompositeImplicitFunctionParser.h"
#include "PlaneImplicitFunctionParser.h"
#include "CompositeImplicitFunction.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

class  CompositeImplicitFunctionParserTest : public CxxTest::TestSuite, FunctionParserTest 
{

public:


  void testBadXMLSchemaThrows(void)
  {
    using namespace Mantid::MDAlgorithms;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><X><Type>CompositeImplicitFunction</Type><ParameterList></ParameterList></X>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    CompositeImplicitFunctionParser functionParser;
    TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as Function element was expected, but not found.", functionParser.createFunctionBuilder(pRootElem), std::invalid_argument );
  }

  void testNoSuccessorFunctionParserThrows(void)
  {
    using namespace Mantid::MDAlgorithms;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>CompositeImplicitFunction</Type><ParameterList></ParameterList></Function>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    CompositeImplicitFunctionParser functionParser;
    TSM_ASSERT_THROWS("There is no successor parser setup for the PlaneFunctionParser", functionParser.createFunctionBuilder(pRootElem), std::runtime_error );
  }


  void testCallsFunctionParserChain()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>OtherFunctionType</Type><ParameterList></ParameterList></Function>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    MockFunctionParser* mockFuncParser = new MockFunctionParser(constructRootParameterParser());
    EXPECT_CALL(*mockFuncParser, createFunctionBuilder(testing::_))
      .Times(1);

    CompositeImplicitFunctionParser functionParser;
    functionParser.setSuccessorParser(mockFuncParser);
    ImplicitFunctionBuilder* builder = functionParser.createFunctionBuilder(pRootElem);
    delete builder;

    TSM_ASSERT("Incorrect calling of nested successor function parsers", testing::Mock::VerifyAndClearExpectations(mockFuncParser))
  }

  void testParseCompositeFunction(void)
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>CompositeImplicitFunction</Type><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function></Function>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    CompositeImplicitFunctionParser functionParser;
    ImplicitFunctionParser* planeParser = new PlaneImplicitFunctionParser;
    planeParser->setParameterParser(constructRootParameterParser());
    functionParser.setSuccessorParser(planeParser);
    ImplicitFunctionBuilder* implicitFunctionBuilder = functionParser.createFunctionBuilder(pRootElem);
    boost::scoped_ptr<Mantid::API::ImplicitFunction> impFunction(implicitFunctionBuilder->create());

    CompositeImplicitFunction* compositeFunction = dynamic_cast<CompositeImplicitFunction*>(impFunction.get());

    TSM_ASSERT("A composite implicit function should have been created from the xml.", compositeFunction != NULL);
    TSM_ASSERT_EQUALS("The composite does not contain the expected number of next-level nested functions.", 2, compositeFunction->getNFunctions())
  }


};

#endif