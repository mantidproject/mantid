#ifndef TEST_ORIGIN_PARAMETER_PARSER_H_
#define TEST_ORIGIN_PARAMETER_PARSER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "OriginParameterParser.h"
#include "OriginParameter.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

class  OriginParameterParserTest : public CxxTest::TestSuite
{
private:


    class ExposedOriginParameterParser : public Mantid::MDAlgorithms::OriginParameterParser
    {
    public:
        Mantid::MDAlgorithms::OriginParameter* exposedParseOriginParameterValue(std::string value)
        {
            return this->parseOriginParameter(value);
        }
    };

    class SuccessorParameterParser : public Mantid::MDAlgorithms::InvalidParameterParser
    {
    public:
        bool isCalled;
        SuccessorParameterParser() : isCalled(false) {}
        std::auto_ptr<Mantid::MDAlgorithms::IParameter> createParameter(Poco::XML::Element* parameterElement)
        {
            using namespace Mantid::MDAlgorithms;
            isCalled = true;
            return std::auto_ptr<IParameter>(new OriginParameter(0,0,0));
        }
    };

public:

    void testParseOriginParameterValue()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedOriginParameterParser parser;
        OriginParameter* originParameter = parser.exposedParseOriginParameterValue("1, 2, 3");
        TSM_ASSERT_EQUALS("The OriginParameter x value has not been parsed correctly.", 1, originParameter->getX());
        TSM_ASSERT_EQUALS("The OriginParameter y value has not been parsed correctly.", 2, originParameter->getY());
        TSM_ASSERT_EQUALS("The OriginParameter z value has not been parsed correctly.", 3, originParameter->getZ());
        delete originParameter;
    }

    void testParseOriginParameterValueIncompleteThrows()
    {
        ExposedOriginParameterParser parser;
        TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as only two of three origin components are provided.", parser.exposedParseOriginParameterValue("1, 2"), std::invalid_argument );
    }

    void testParseOriginParameterFragment()
    {
        using namespace Mantid::MDAlgorithms;
        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>OriginParameter</Type><Value>1, 2, 3</Value></Parameter>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        OriginParameterParser parser;
        std::auto_ptr<IParameter> iparam = parser.createParameter(pRootElem);
        OriginParameter* pOriginParam = dynamic_cast<OriginParameter*>(iparam.release());
        std::auto_ptr<OriginParameter> oparam = std::auto_ptr<OriginParameter>(pOriginParam);
        TSM_ASSERT("The paramter generated should be an OriginParamter", NULL != pOriginParam);
    }

    void testChainOfResponsibility()
    {
        using namespace Mantid::MDAlgorithms;
        Poco::XML::DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>UnknownParameter</Type><Value>1, 2, 3</Value></Parameter>";
        Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
        Poco::XML::Element* pRootElem = pDoc->documentElement();

        OriginParameterParser parser;
        SuccessorParameterParser* successor = new SuccessorParameterParser;
        parser.setSuccessorParser(std::auto_ptr<ParameterParser>(successor));
        std::auto_ptr<IParameter> iparam = parser.createParameter(pRootElem);
        TSM_ASSERT("The successor parser has not been called.", successor->isCalled)
    }

};

#endif