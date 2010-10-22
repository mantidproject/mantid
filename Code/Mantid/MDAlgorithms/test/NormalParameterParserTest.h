#ifndef TEST_PLANE_FUNCTION_PARSER_H_
#define TEST_PLANE_FUNCTION_PARSER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "NormalParameterParser.h"
#include "NormalParameter.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

class NormalParameterParserTest : public CxxTest::TestSuite
{
private:

    class ExposedNormalParameterParser : public Mantid::MDAlgorithms::NormalParameterParser
    {
    public:
        Mantid::MDAlgorithms::NormalParameter* exposedParseNormalParameterValue(std::string value)
        {
            return this->parseNormalParameter(value);
        }
    };

    class SuccessorParameterParser : public Mantid::MDAlgorithms::InvalidParameterParser
    {
    public:
        bool isCalled;

        std::auto_ptr<Mantid::MDAlgorithms::IParameter> createParameter(Poco::XML::Element* parameterElement)
        {
            isCalled = true;
            return std::auto_ptr<Mantid::MDAlgorithms::IParameter>(new Mantid::MDAlgorithms::NormalParameter(0,0,0));
        }
    };

public:

    void testParseNormalParameterValue()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedNormalParameterParser parser;
        NormalParameter* normalParameter = parser.exposedParseNormalParameterValue("1, 2, 3");
        TSM_ASSERT_EQUALS("The NormalParameter x value has not been parsed correctly.", 1, normalParameter->getX());
        TSM_ASSERT_EQUALS("The NormalParameter y value has not been parsed correctly.", 2, normalParameter->getY());
        TSM_ASSERT_EQUALS("The NormalParameter z value has not been parsed correctly.", 3, normalParameter->getZ());
        delete normalParameter;
    }

    void testParseNormalParameterValueIncompleteThrows()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedNormalParameterParser parser;
        TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as only two of three normal components are provided.", parser.exposedParseNormalParameterValue("1, 2"), std::invalid_argument );
    }

    void testParseNormalParameterFragment()
    {
        using namespace Mantid::MDAlgorithms;
        using namespace Poco::XML;

        DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>NormalParameter</Type><Value>1, 2, 3</Value></Parameter>";
        Document* pDoc = pParser.parseString(xmlToParse);
        Element* pRootElem = pDoc->documentElement();

        NormalParameterParser parser;
        std::auto_ptr<IParameter> iparam = parser.createParameter(pRootElem);
        NormalParameter* pNormalParam = dynamic_cast<NormalParameter*>(iparam.release());
        std::auto_ptr<NormalParameter> nparam = std::auto_ptr<NormalParameter>(pNormalParam);
        TSM_ASSERT("The paramter generated should be an NormalParamter", NULL != pNormalParam);
    }

    void testChainOfResponsibility()
    {
        using namespace Mantid::MDAlgorithms;
        using namespace Poco::XML;

        DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>Unknown</Type><Value>1, 2, 3</Value></Parameter>";
        Document* pDoc = pParser.parseString(xmlToParse);
        Element* pRootElem = pDoc->documentElement();

        NormalParameterParser parser;
        SuccessorParameterParser* successor = new SuccessorParameterParser;
        parser.setSuccessorParser(std::auto_ptr<ParameterParser>(successor));
        std::auto_ptr<IParameter> iparam = parser.createParameter(pRootElem);
        TSM_ASSERT("The successor parser has not been called.", successor->isCalled)
    }

};

#endif