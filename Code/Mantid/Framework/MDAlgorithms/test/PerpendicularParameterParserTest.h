#ifndef TEST_PERPENDICULAR_PARAMETER_PARSER_H
#define TEST_PERPENDICULAR_PARAMETER_PARSER_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/VectorParameterParser.h"
#include "MantidMDAlgorithms/PerpendicularParameter.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

class PerpendicularParameterParserTest : public CxxTest::TestSuite
{
private:

    //Testable sub-class
    class ExposedPerpendicularParameterParser : public Mantid::MDAlgorithms::PerpendicularParameterParser
    {
    public: //Make protected method on base public.
        Mantid::MDAlgorithms::PerpendicularParameter* exposedParsePerpendicularParameterValue(std::string value)
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

    void testParsePerpendicularParameterValue()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedPerpendicularParameterParser parser;
        PerpendicularParameter* perpendicularParameter = parser.exposedParsePerpendicularParameterValue("1, 2, 3");
        TSM_ASSERT_EQUALS("The PerpendicularParameter x value has not been parsed correctly.", 1, perpendicularParameter->getX());
        TSM_ASSERT_EQUALS("The PerpendicularParameter y value has not been parsed correctly.", 2, perpendicularParameter->getY());
        TSM_ASSERT_EQUALS("The PerpendicularParameter z value has not been parsed correctly.", 3, perpendicularParameter->getZ());
        delete perpendicularParameter;
    }

    void testParsePerpendicularParameterValueIncompleteThrows()
    {
        using namespace Mantid::MDAlgorithms;
        ExposedPerpendicularParameterParser parser;
        TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as only two of the three components are provided.", parser.exposedParsePerpendicularParameterValue("1, 2"), std::invalid_argument );
    }

    void testParsePerpendicularParameterFragment()
    {
        using namespace Mantid::MDAlgorithms;
        using namespace Poco::XML;

        DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>PerpendicularParameter</Type><Value>1, 2, 3</Value></Parameter>";
        Document* pDoc = pParser.parseString(xmlToParse);
        Element* pRootElem = pDoc->documentElement();

        PerpendicularParameterParser parser;
        Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
        PerpendicularParameter* pNormalParam = dynamic_cast<PerpendicularParameter*>(iparam);
        boost::scoped_ptr<PerpendicularParameter> nparam(pNormalParam);
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

        PerpendicularParameterParser parser;

        parser.setSuccessorParser(successor);
        Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);

        TSM_ASSERT("Chain of responsiblity did not execute as expected for PerpendicularParameter type.", testing::Mock::VerifyAndClearExpectations(successor))
    }

  void testCanParseXMLOutput()
  {
    //Circular check that xml given by an normal parameter can be used to create a new one using the parser.
    using namespace Mantid::MDAlgorithms;
    PerpendicularParameter originalPerpendicular(1, 2, 3);

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(originalPerpendicular.toXMLString());

    PerpendicularParameterParser perpendicularParser;
    PerpendicularParameter* synthPerpendicular = dynamic_cast<PerpendicularParameter*>(perpendicularParser.createParameter(pDoc->documentElement()));

    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. x-values do not match", originalPerpendicular.getX() ,  synthPerpendicular->getX());
    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. y-values do not match", originalPerpendicular.getY() ,  synthPerpendicular->getY());
    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. z-values do not match", originalPerpendicular.getZ() ,  synthPerpendicular->getZ());

    delete synthPerpendicular;
  }
};

#endif
