// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FunctionParserTest.h"
#include <boost/scoped_ptr.hpp>
#include <memory>
#include <vector>

#include "MantidMDAlgorithms/CompositeImplicitFunctionParser.h"
#include "MantidMDAlgorithms/InvalidParameterParser.h"
// #include "MantidMDAlgorithms/PlaneImplicitFunctionParser.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

class CompositeImplicitFunctionParserTest : public CxxTest::TestSuite, FunctionParserTest {

public:
  void disabled_testBadXMLSchemaThrows(void) {
    using namespace Mantid::MDAlgorithms;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><X><Type>"
                             "CompositeImplicitFunction</Type><ParameterList></"
                             "ParameterList></X>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CompositeImplicitFunctionParser functionParser;
    TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as "
                      "Function element was expected, but not found.",
                      functionParser.createFunctionBuilder(pRootElem), const std::invalid_argument &);
  }

  void disabled_testNoSuccessorFunctionParserThrows(void) {
    using namespace Mantid::MDAlgorithms;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><Function><Type>"
                             "CompositeImplicitFunction</Type><ParameterList></"
                             "ParameterList></Function>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CompositeImplicitFunctionParser functionParser;
    TSM_ASSERT_THROWS("There is no successor parser setup for the PlaneFunctionParser",
                      functionParser.createFunctionBuilder(pRootElem), const std::runtime_error &);
  }

  void disabled_testCallsFunctionParserChain() {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><Function><Type>"
                             "OtherFunctionType</Type><ParameterList></"
                             "ParameterList></Function>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    MockFunctionParser *mockFuncParser = new MockFunctionParser(constructRootParameterParser());
    EXPECT_CALL(*mockFuncParser, createFunctionBuilder(testing::_)).Times(1);

    CompositeImplicitFunctionParser functionParser;
    functionParser.setSuccessorParser(mockFuncParser);
    ImplicitFunctionBuilder *builder = functionParser.createFunctionBuilder(pRootElem);
    delete builder;

    TSM_ASSERT("Incorrect calling of nested successor function parsers",
               testing::Mock::VerifyAndClearExpectations(mockFuncParser))
  }

  void disabled_testParseCompositeFunction(void) {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") + "<Function>" +
                             "<Type>CompositeImplicitFunction</Type>" + "<Function>" +
                             "<Type>PlaneImplicitFunction</Type>" + "<ParameterList>" +
                             "<Parameter><Type>NormalParameter</Type><Value>-1, -2, "
                             "-3</Value></Parameter>" +
                             "<Parameter><Type>OriginParameter</Type><Value>1, 2, "
                             "3</Value></Parameter>" +
                             "<Parameter><Type>WidthParameter</Type><Value>7</Value></Parameter>" + "</ParameterList>" +
                             "</Function>" + "<Function>" + "<Type>PlaneImplicitFunction</Type>" + "<ParameterList>" +
                             "<Parameter><Type>NormalParameter</Type><Value>-1, -2, "
                             "-3</Value></Parameter>" +
                             "<Parameter><Type>OriginParameter</Type><Value>1, 2, "
                             "3</Value></Parameter>" +
                             "<Parameter><Type>WidthParameter</Type><Value>7</Value></Parameter>" + "</ParameterList>" +
                             "</Function>" + "</Function>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CompositeImplicitFunctionParser functionParser;
    ImplicitFunctionParser *planeParser = new PlaneImplicitFunctionParser;
    planeParser->setParameterParser(constructRootParameterParser());
    functionParser.setSuccessorParser(planeParser);
    ImplicitFunctionBuilder *implicitFunctionBuilder = functionParser.createFunctionBuilder(pRootElem);
    Mantid::Geometry::MDImplicitFunction_sptr impFunction(implicitFunctionBuilder->create());

    CompositeImplicitFunction *compositeFunction = dynamic_cast<CompositeImplicitFunction *>(impFunction.get());

    TSM_ASSERT("A composite implicit function should have been created from the xml.", compositeFunction != NULL);
    TSM_ASSERT_EQUALS("The composite does not contain the expected number of "
                      "next-level nested functions.",
                      2, compositeFunction->getNFunctions())
  }
};
