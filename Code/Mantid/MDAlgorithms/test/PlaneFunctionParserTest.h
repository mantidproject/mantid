#ifndef TEST_PLANE_FUNCTION_PARSERS_H_
#define TEST_PLANE_FUNCTION_PARSERS_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "PlaneFunctionParser.h"
#include "NormalParameterParser.h"
#include "OriginParameterParser.h"
#include "InvalidParameterParser.h"
#include "PlaneFunctionBuilder.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

class  PlaneFunctionParserTest : public CxxTest::TestSuite
{
private:

    class ExposedPlaneFunctionParser : public Mantid::MDAlgorithms::PlaneFunctionParser
    {
    public:
        bool isSuccess;
        ExposedPlaneFunctionParser(std::auto_ptr<Mantid::MDAlgorithms::ParameterParser> paramParser) :  PlaneFunctionParser(paramParser)
        {}
        Mantid::MDAlgorithms::PlaneFunctionBuilder* exposedParsePlaneFunction(Poco::XML::Element* functionElement)
        {
            return this->parsePlaneFunction(functionElement);
        }
        std::auto_ptr<Mantid::MDAlgorithms::IFunctionBuilder> createFunctionBuilder(Poco::XML::Element* functionElement)
        { 
            isSuccess = true;
            return Mantid::MDAlgorithms::PlaneFunctionParser::createFunctionBuilder(functionElement);
        }
    };

    std::auto_ptr<Mantid::MDAlgorithms::ParameterParser> rootParameterParser;

public:

    virtual void setUp()
    {
        using namespace Mantid::MDAlgorithms;
        std::auto_ptr<ParameterParser> originParser = std::auto_ptr<ParameterParser>(new OriginParameterParser);
        std::auto_ptr<ParameterParser> normalParser = std::auto_ptr<ParameterParser>(new NormalParameterParser);
        std::auto_ptr<ParameterParser> invalidParser = std::auto_ptr<ParameterParser>(new InvalidParameterParser);

        originParser->setSuccessorParser(invalidParser.release());
        normalParser->setSuccessorParser(originParser.release());

        //Apply the chain of responsibility for the parameter parsers.
        this->rootParameterParser = normalParser;
    }


    void testParsePlaneFunction(void)
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        ExposedPlaneFunctionParser functionParser(this->rootParameterParser);
        PlaneFunctionBuilder* planeBuilder = functionParser.exposedParsePlaneFunction(pRootElem);
        std::auto_ptr<Mantid::API::IImplicitFunction> impFunction = planeBuilder->create();

        PlaneImplicitFunction* planeFunction = dynamic_cast<PlaneImplicitFunction*>(impFunction.get());

        TSM_ASSERT("A plane implicit function should have been created from the xml.", planeFunction != NULL);

        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of origin parameters to give the correct ox value.", 1, planeFunction->getOriginX());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of origin parameters to give the correct oy value.", 2, planeFunction->getOriginY());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of origin parameters to give the correct oz value.", 3, planeFunction->getOriginZ());

        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of normal parameters to give the correct nx value.", -1, planeFunction->getNormalX());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of normal parameters to give the correct ny value.", -2, planeFunction->getNormalY());
        TSM_ASSERT_EQUALS("The plane parser did not direct the parsing of normal parameters to give the correct nz value.", -3, planeFunction->getNormalZ());
    }

    void testBadXMLThrows(void)
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><X><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></X>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        PlaneFunctionParser functionParser(this->rootParameterParser);
        TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as Function element was expected, but not found.", functionParser.createFunctionBuilder(pRootElem), std::invalid_argument );
    }

    void testNoSuccessorFunctionParserThrows(void)
    {
        using namespace Mantid::MDAlgorithms;

        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Function><Type>UnknownImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>-1, -2, -3</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter></ParameterList></Function>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        PlaneFunctionParser functionParser(this->rootParameterParser);
        TSM_ASSERT_THROWS("There is no successor parser setup for the PlaneFunctionParser", functionParser.createFunctionBuilder(pRootElem), std::runtime_error );
    }

};

#endif