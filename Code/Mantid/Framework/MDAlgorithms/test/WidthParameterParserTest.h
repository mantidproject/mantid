#ifndef TEST_WIDTH_PARAMETER_PARSER_H_
#define TEST_WIDTH_PARAMETER_PARSER_H_

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
class  WidthParameterParserTest : public CxxTest::TestSuite
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


  void testParseWidthParameterFragment()
  {
    using namespace Mantid::MDAlgorithms;
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>WidthParameter</Type><Value>3</Value></Parameter>";
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    WidthParameterParser parser;
    Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
    WidthParameter* pWidthParam = dynamic_cast<WidthParameter*>(iparam);
    TSM_ASSERT("The paramter generated should be an WidthParamter", NULL != pWidthParam);
    TSM_ASSERT_EQUALS("Numeric value has not been parsed correctly", 3, pWidthParam->getValue() );
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

    WidthParameterParser parser;

    parser.setSuccessorParser(successor);
    Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
    delete iparam;
    TSM_ASSERT("Chain of responsiblity did not execute as expected for OriginParameter type.", testing::Mock::VerifyAndClearExpectations(successor))
  }

  void testCanParseXMLOutput()
  { 
    //Circular check that xml given by an origin parameter can be used to create a new one using the parser.
    WidthParameter originalWidth(2);

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(originalWidth.toXMLString());

    WidthParameterParser widthParser;
    WidthParameter* synthWidth = dynamic_cast<WidthParameter*>(widthParser.createParameter(pDoc->documentElement()));

    TSM_ASSERT_EQUALS("Formats used for xml parsing and xml output are not synchronised. values do not match", originalWidth.getValue() ,  synthWidth->getValue());

    delete synthWidth;
  }

};

#endif
