#ifndef TEST_HEIGHT_PARAMETER_PARSER_H_
#define TEST_HEIGHT_PARAMETER_PARSER_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidMDAlgorithms/SingleValueParameterParser.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::MDAlgorithms;

class  HeightParameterParserTest : public CxxTest::TestSuite
{
private:

  //Mock class
  class SuccessorParameterParser : public Mantid::API::ImplicitFunctionParameterParser 
  {
  public:
    MOCK_METHOD1(createParameter, Mantid::API::ImplicitFunctionParameter*(Poco::XML::Element* parameterElement));
    MOCK_METHOD1(setSuccessorParser, void(Mantid::API::ImplicitFunctionParameterParser* parameterParser));
  };

public:


  void testParseHeightParameterFragment()
  {
    using namespace Mantid::MDAlgorithms;
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>HeightParameter</Type><Value>3</Value></Parameter>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    HeightParameterParser parser;
    Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
    HeightParameter* pHeightParam = dynamic_cast<HeightParameter*>(iparam);
    TSM_ASSERT("The paramter generated should be an HeightParamter", NULL != pHeightParam);
    TSM_ASSERT_EQUALS("Numeric value has not been parsed correctly", 3, pHeightParam->getValue() );
  }

  void testChainOfResponsibility()
  {
    using namespace Mantid::MDAlgorithms;
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>UnknownParameter</Type><Value>1, 2, 3</Value></Parameter>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    SuccessorParameterParser* successor = new SuccessorParameterParser;
    EXPECT_CALL(*successor, createParameter(testing::_)).Times(1);

    HeightParameterParser parser;

    parser.setSuccessorParser(successor);
    Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);

    TSM_ASSERT("Chain of responsiblity did not execute as expected for OriginParameter type.", testing::Mock::VerifyAndClearExpectations(successor))
  }

  void testCanParseXMLOutput()
  { 
    //Circular check that xml given by an origin parameter can be used to create a new one using the parser.
    HeightParameter originalHeight(2);

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(originalHeight.toXMLString());

    HeightParameterParser heightParser;
    HeightParameter* synthHeight = dynamic_cast<HeightParameter*>(heightParser.createParameter(pDoc->documentElement()));

    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. values do not match", originalHeight.getValue() ,  synthHeight->getValue());

    delete synthHeight;
  }

};

#endif
