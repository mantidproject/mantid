#ifndef TEST_UP_PARAMETER_PARSER_H
#define TEST_UP_PARAMETER_PARSER_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/VectorParameterParser.h"
#include "MantidMDAlgorithms/UpParameter.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

class UpParameterParserTest : public CxxTest::TestSuite
{
private:

    //Testable sub-class
    class ExposedUpParameterParser : public Mantid::MDAlgorithms::UpParameterParser
    {
    public: //Make protected method on base public.
        Mantid::MDAlgorithms::UpParameter* exposedParseUpParameterValue(std::string value)
        {
            return this->parseVectorParameter(value);
        }
    };

  //Mock class
    class SuccessorParameterParser : public Mantid::API::ImplicitFunctionParameterParser
    {
    public:
        MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter*(Poco::XML::Element* parameterElement));
        MOCK_METHOD1(setSuccessorParser, void(Mantid::API::ImplicitFunctionParameterParser* parameterParser));
    };

public:

    void testParseUpParameterValue()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedUpParameterParser parser;
        UpParameter* UpParameter = parser.exposedParseUpParameterValue("1, 2, 3");
        TSM_ASSERT_EQUALS("The UpParameter x value has not been parsed correctly.", 1, UpParameter->getX());
        TSM_ASSERT_EQUALS("The UpParameter y value has not been parsed correctly.", 2, UpParameter->getY());
        TSM_ASSERT_EQUALS("The UpParameter z value has not been parsed correctly.", 3, UpParameter->getZ());
        delete UpParameter;
    }

    void testParseUpParameterValueIncompleteThrows()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedUpParameterParser parser;
        TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as only two of the three components are provided.", parser.exposedParseUpParameterValue("1, 2"), std::invalid_argument );
    }

    void testParseUpParameterFragment()
    {
        using namespace Mantid::MDAlgorithms;
        using namespace Poco::XML;

        DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>UpParameter</Type><Value>1, 2, 3</Value></Parameter>";
        Document* pDoc = pParser.parseString(xmlToParse);
        Element* pRootElem = pDoc->documentElement();

        UpParameterParser parser;
        Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
        UpParameter* pNormalParam = dynamic_cast<UpParameter*>(iparam);
        boost::scoped_ptr<UpParameter> nparam(pNormalParam);
        TSM_ASSERT("The paramter generated should be an PerpendicularParamter", NULL != pNormalParam);
    }
    void testChainOfResponsibility()
    {
        using namespace Mantid::MDAlgorithms;
        using namespace Poco::XML;

        DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>Unknown</Type><Value>1, 2, 3</Value></Parameter>";
        Document* pDoc = pParser.parseString(xmlToParse);
        Element* pRootElem = pDoc->documentElement();

        SuccessorParameterParser* successor = new SuccessorParameterParser;
        EXPECT_CALL(*successor, createParameter(testing::_)).Times(1);

        UpParameterParser parser;

        parser.setSuccessorParser(successor);
        Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
        delete iparam;

        TSM_ASSERT("Chain of responsibility did not execute as expected for UpParameter type.", testing::Mock::VerifyAndClearExpectations(successor))
    }

  void testCanParseXMLOutput()
  {
    //Circular check that xml given by an normal parameter can be used to create a new one using the parser.
    using namespace Mantid::MDAlgorithms;
    UpParameter originalUp(1, 2, 3);

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(originalUp.toXMLString());

    UpParameterParser perpendicularParser;
    UpParameter* synthUp = dynamic_cast<UpParameter*>(perpendicularParser.createParameter(pDoc->documentElement()));

    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. x-values do not match", originalUp.getX() ,  synthUp->getX());
    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. y-values do not match", originalUp.getY() ,  synthUp->getY());
    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. z-values do not match", originalUp.getZ() ,  synthUp->getZ());

    delete synthUp;
  }
};

#endif
