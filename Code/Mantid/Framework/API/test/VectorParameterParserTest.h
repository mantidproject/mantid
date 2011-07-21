#ifndef VECTOR_IMPLICIT_FUNCTION_PARAMETER_PARSER_TEST_H_
#define VECTOR_IMPLICIT_FUNCTION_PARAMETER_PARSER_TEST_H_

#include "MantidAPI/VectorParameterParser.h"
#include "MantidAPI/VectorParameter.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class VectorParameterParserTest : public CxxTest::TestSuite
{

private:

  //Declare a concrete vector parameter for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorParameter, double)

  //Declare a concrete vector parameter parser for testing.
  typedef VectorParameterParser<ConcreteVectorParameter> ConcreteVectorParameterParser;

public:

  void testParsesParmeterValue1D()
  {
    ConcreteVectorParameterParser parser;
    ConcreteVectorParameter* product = parser.parseVectorParameter("1");
    double v1 = (*product)[0];
    TS_ASSERT_EQUALS(1, v1 );
    delete product;
  }

  void testParsesParmeterValue2D()
  {
    ConcreteVectorParameterParser parser;
    ConcreteVectorParameter* product = parser.parseVectorParameter("1,2");
    double v1 = (*product)[0];
    double v2 = (*product)[1];
    TS_ASSERT_EQUALS(1, v1 );
    TS_ASSERT_EQUALS(2, v2 );
    delete product;
  }

  void testParsesParmeterValue3D()
  {
    ConcreteVectorParameterParser parser;
    ConcreteVectorParameter* product = parser.parseVectorParameter("1,2,3");
    double v1 = (*product)[0];
    double v2 = (*product)[1];
    double v3 = (*product)[2];
    TS_ASSERT_EQUALS(1, v1 );
    TS_ASSERT_EQUALS(2, v2 );
    TS_ASSERT_EQUALS(3, v3 );
    delete product;
  }

  void testSuccessfulParse()
  {
    using namespace Poco::XML;

    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>ConcreteVectorParameter</Type><Value>1, 2, 3</Value></Parameter>";
    Document* pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorParameterParser parser;
    ImplicitFunctionParameter * product = parser.createParameter(pRootElem);

    ConcreteVectorParameter* actualProduct = dynamic_cast<ConcreteVectorParameter*>(product);

    TSM_ASSERT("The wrong product parameter has been produced", (actualProduct != NULL));
    double v1 = (*actualProduct)[0];
    double v2 = (*actualProduct)[1];
    double v3 = (*actualProduct)[2];

    TS_ASSERT_EQUALS(1, v1);
    TS_ASSERT_EQUALS(2, v2);
    TS_ASSERT_EQUALS(3, v3);
    delete actualProduct;
  }

  void testThrowsIfNoSuccessor()
  {
    using namespace Poco::XML;

    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>SucessorVectorParameter</Type><Value>1, 2, 3</Value></Parameter>";
    Document* pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorParameterParser parser;
    TSM_ASSERT_THROWS("No successor, so should throw!", parser.createParameter(pRootElem), std::runtime_error);
  }

  DECLARE_VECTOR_PARAMETER(SucessorVectorParameter, double);

  void testChainOfResponsibility()
  {
    using namespace Poco::XML;

    //Local declare of a successor parser with a successor parameter.
    typedef VectorParameterParser<SucessorVectorParameter> ConcreteSuccessorVectorParameterParser;

    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>SucessorVectorParameter</Type><Value>1, 2, 3</Value></Parameter>";
    Document* pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    SucessorVectorParameter* successor = new SucessorVectorParameter;

    ConcreteVectorParameterParser parser;

    parser.setSuccessorParser(new ConcreteSuccessorVectorParameterParser);
    Mantid::API::ImplicitFunctionParameter* product = parser.createParameter(pRootElem);

    TSM_ASSERT("Product should be a SucessorVectorParameter", dynamic_cast<SucessorVectorParameter*>(product));
    delete product;
  }

};

#endif
