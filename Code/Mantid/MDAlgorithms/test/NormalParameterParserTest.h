#ifndef TEST_PLANE_FUNCTION_PARSER_H_
#define TEST_PLANE_FUNCTION_PARSER_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    //Testable sub-class
    class ExposedNormalParameterParser : public Mantid::MDAlgorithms::NormalParameterParser
    {
    public: //Make protected method on base public.
        Mantid::MDAlgorithms::NormalParameter* exposedParseNormalParameterValue(std::string value)
        {
            return this->parseNormalParameter(value);
        }
    };

	//Mock class
    class SuccessorParameterParser : public Mantid::MDAlgorithms::ParameterParser 
    {
    public:
        MOCK_METHOD1(createParameter, Mantid::MDAlgorithms::IParameter*(Poco::XML::Element* parameterElement));
        MOCK_METHOD1(setSuccessorParser, void(Mantid::MDAlgorithms::ParameterParser* parameterParser));
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
        IParameter* iparam = parser.createParameter(pRootElem);
        NormalParameter* pNormalParam = dynamic_cast<NormalParameter*>(iparam);
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
		
		SuccessorParameterParser* successor = new SuccessorParameterParser;
        EXPECT_CALL(*successor, createParameter(testing::_)).Times(1);
        
		NormalParameterParser parser;
		
        parser.setSuccessorParser(successor);
        IParameter* iparam = parser.createParameter(pRootElem);

        TSM_ASSERT("Chain of responsiblity did not execute as expected for NormalParameter type.", testing::Mock::VerifyAndClearExpectations(successor))
    }
	
	void testCanParseXMLOutput()
	{ 
	  //Circular check that xml given by an normal parameter can be used to create a new one using the parser.
	  using namespace Mantid::MDAlgorithms;
	  NormalParameter originalNormal(1, 2, 3);
	  
	  Poco::XML::DOMParser pParser;
      Poco::XML::Document* pDoc = pParser.parseString(originalNormal.toXMLString());
	
	  NormalParameterParser normalParser;
	  NormalParameter* synthNormal = dynamic_cast<NormalParameter*>(normalParser.createParameter(pDoc->documentElement()));
	  
	  TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. x-values do not match", originalNormal.getX() ,  synthNormal->getX());
	  TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. y-values do not match", originalNormal.getY() ,  synthNormal->getY());
	  TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. z-values do not match", originalNormal.getZ() ,  synthNormal->getZ());
	  
	  delete synthNormal;
	}
};

#endif