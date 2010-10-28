#ifndef TEST_PLANE_IMPLICIT_FUNCTION_PARSERS_H_
#define TEST_PLANE_IMPLICIT_FUNCTION_PARSERS_H_


#include <vector>
#include <memory>
#include "FunctionParserTest.h"
#include "PlaneImplicitFunctionParser.h"
#include "NormalParameterParser.h"
#include "OriginParameterParser.h"
#include "InvalidParameterParser.h"
#include "PlaneFunctionBuilder.h"
#include <cxxtest/TestSuite.h>

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

class  PlaneImplicitFunctionParserTest : public CxxTest::TestSuite, FunctionParserTest 
{
private:

    class ExposedPlaneFunctionParser : public Mantid::MDAlgorithms::PlaneImplicitFunctionParser
    {
    public:
        ExposedPlaneFunctionParser(Mantid::API::ImplicitFunctionParameterParser* paramParser) :  PlaneImplicitFunctionParser(paramParser)
        {}
        Mantid::MDAlgorithms::PlaneFunctionBuilder* exposedParsePlaneFunction(Poco::XML::Element* functionElement)
        {
            return this->parsePlaneFunction(functionElement);
        }
        MOCK_METHOD1(createFunctionBuilder, Mantid::API::ImplicitFunctionBuilder*(Poco::XML::Element* functionElement));
    };

public:

    void testCallsParameterParserChain()
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        MockParameterParser* paramParser = new MockParameterParser;
        EXPECT_CALL(*paramParser, createParameter(testing::_))
            .WillOnce(testing::Return(new OriginParameter(0, 0, 0)))
            .WillOnce(testing::Return(new NormalParameter(0, 0, 0)));

        PlaneImplicitFunctionParser functionParser(paramParser);
        Mantid::API::ImplicitFunctionBuilder* builder = functionParser.createFunctionBuilder(pRootElem);
        delete builder;

        TSM_ASSERT("Incorrect calling of nested matched parameter parsers!", testing::Mock::VerifyAndClearExpectations(paramParser))
    }

    void testCallsFunctionParserChain()
    {
        using namespace Mantid::MDAlgorithms;
		using namespace Mantid::API;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>X</Type><ParameterList></ParameterList></Function>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        MockFunctionParser* mockFuncParser = new MockFunctionParser(constructRootParameterParser().release());
        EXPECT_CALL(*mockFuncParser, createFunctionBuilder(testing::_))
            .Times(1);

        PlaneImplicitFunctionParser functionParser(constructRootParameterParser().release());
        functionParser.setSuccessorParser(mockFuncParser);
        ImplicitFunctionBuilder* builder = functionParser.createFunctionBuilder(pRootElem);
        delete builder;

        TSM_ASSERT("Incorrect calling of nested successor function parsers", testing::Mock::VerifyAndClearExpectations(mockFuncParser))
    }

    void testParsePlaneFunction(void)
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        ExposedPlaneFunctionParser functionParser(constructRootParameterParser().release());
        PlaneFunctionBuilder* planeBuilder = functionParser.exposedParsePlaneFunction(pRootElem);
        std::auto_ptr<Mantid::API::ImplicitFunction> impFunction = planeBuilder->create();

        PlaneImplicitFunction* planeFunction = dynamic_cast<PlaneImplicitFunction*>(impFunction.get());

        TSM_ASSERT("A plane implicit function should have been created from the xml.", planeFunction != NULL);

        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of origin parameters to give the correct ox value.", 1, planeFunction->getOriginX());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of origin parameters to give the correct oy value.", 2, planeFunction->getOriginY());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of origin parameters to give the correct oz value.", 3, planeFunction->getOriginZ());

        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of normal parameters to give the correct nx value.", -1, planeFunction->getNormalX());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of normal parameters to give the correct ny value.", -2, planeFunction->getNormalY());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of normal parameters to give the correct nz value.", -3, planeFunction->getNormalZ());
    }

    void testBadXMLSchemaThrows(void)
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><X><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></X>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        PlaneImplicitFunctionParser functionParser(constructRootParameterParser().release());
        TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as Function element was expected, but not found.", functionParser.createFunctionBuilder(pRootElem), std::invalid_argument );
    }

    void testNoSuccessorFunctionParserThrows(void)
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>OtherImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        PlaneImplicitFunctionParser functionParser(constructRootParameterParser().release());
        TSM_ASSERT_THROWS("There is no successor parser setup for the PlaneImplicitFunctionParser", functionParser.createFunctionBuilder(pRootElem), std::runtime_error );
    }
	

};

#endif