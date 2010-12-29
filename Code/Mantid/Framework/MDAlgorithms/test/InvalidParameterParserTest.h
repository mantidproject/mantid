#ifndef TEST_INVALID_PARAMETER_PARSER_H_
#define TEST_INVALID_PARAMETER_PARSER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include "MantidMDAlgorithms/InvalidParameter.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

class  InvalidParameterParserTest : public CxxTest::TestSuite
{
public:

    void testParseInvalidParameterFragment()
    {
        using namespace Poco::XML;
        using namespace Mantid::MDAlgorithms;

        DOMParser pParser;
        std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>SomeUnknownParameter</Type><Value>x</Value></Parameter>";
        Document* pDoc = pParser.parseString(xmlToParse);
        Element* pRootElem = pDoc->documentElement();

        InvalidParameterParser parser;
        Mantid::API::ImplicitFunctionParameter* iparam = parser.createParameter(pRootElem);
        InvalidParameter* pInvalidParam = dynamic_cast<InvalidParameter*>(iparam);
        boost::scoped_ptr<InvalidParameter> invalparam(pInvalidParam);

        TSM_ASSERT("The paramter generated should be an InvalidParamter", NULL != pInvalidParam);
        TSM_ASSERT_EQUALS("The invalid parameter has not been parsed correctly.", "x", invalparam->getValue());
    }

};

#endif
