#ifndef TEST_BOX_FUNCTION_PARSER_H_
#define TEST_BOX_FUNCTION_PARSER_H_

#include "FunctionParserTest.h"
#include <vector>
#include <memory>
#include <boost/scoped_ptr.hpp>

#include "MantidMDAlgorithms/BoxImplicitFunctionParser.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::MDAlgorithms;

class  BoxImplicitFunctionParserTest : public CxxTest::TestSuite, FunctionParserTest 
{

public:

  void testBadXMLSchemaThrows(void)
  {
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><X><Type>BoxImplicitFunction</Type><ParameterList></ParameterList></X>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    BoxImplicitFunctionParser functionParser;
    TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as Function element was expected, but not found.", functionParser.createFunctionBuilder(pRootElem), std::invalid_argument );
  }

  void testNoSuccessorFunctionParserThrows(void)
  {
    using namespace Mantid::MDAlgorithms;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>UnknownFunction</Type><ParameterList></ParameterList></Function>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    BoxImplicitFunctionParser functionParser;
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

    BoxImplicitFunctionParser functionParser;
    functionParser.setSuccessorParser(mockFuncParser);
    ImplicitFunctionBuilder* builder = functionParser.createFunctionBuilder(pRootElem);
    delete builder;

    TSM_ASSERT("Incorrect calling of nested successor function parsers", testing::Mock::VerifyAndClearExpectations(mockFuncParser))
  }

  void testParseBoxFunction(void)
  {
    using namespace Mantid::API;
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>BoxImplicitFunction</Type><ParameterList><Parameter><Type>WidthParameter</Type><Value>1</Value></Parameter><Parameter><Type>HeightParameter</Type><Value>2</Value></Parameter><Parameter><Type>DepthParameter</Type><Value>3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>4, 5, 6</Value></Parameter></ParameterList></Function>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    BoxImplicitFunctionParser functionParser;


    MockParameterParser* paramParser = new MockParameterParser;
        EXPECT_CALL(*paramParser, createParameter(testing::_))
            .WillOnce(testing::Return(new WidthParameter(1)))
            .WillOnce(testing::Return(new HeightParameter(2)))
            .WillOnce(testing::Return(new DepthParameter(3)))
            .WillOnce(testing::Return(new OriginParameter(4, 5, 6)))
            ;

		functionParser.setParameterParser(paramParser);


    ImplicitFunctionBuilder* implicitFunctionBuilder = functionParser.createFunctionBuilder(pRootElem);
    boost::scoped_ptr<Mantid::API::ImplicitFunction> impFunction(implicitFunctionBuilder->create());

    BoxImplicitFunction* boxFunction = dynamic_cast<BoxImplicitFunction*>(impFunction.get());
    TSM_ASSERT("A box implicit function should have been created from the xml.", boxFunction != NULL);
  }
   

};

#endif
