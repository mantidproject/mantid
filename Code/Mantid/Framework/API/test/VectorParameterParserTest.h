#ifndef VECTOR_IMPLICIT_FUNCTION_PARAMETER_PARSER_TEST_H_
#define VECTOR_IMPLICIT_FUNCTION_PARAMETER_PARSER_TEST_H_

#include "MantidAPI/VectorParameterParser.h"
#include "MantidAPI/VectorParameter.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Poco::XML;

class VectorParameterParserTest : public CxxTest::TestSuite
{

private:

  //Declare a concrete vector parameter for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorDblParam, double)

  //Declare a concrete vector parameter parser for testing.
  typedef VectorParameterParser<ConcreteVectorDblParam> ConcreteVectorDblParamParser;

  //Declare a concrete type with elements of type bool for testing.
  DECLARE_VECTOR_PARAMETER(ConcreteVectorBoolParam, bool)

  //Declare a concrete vector parameter parser for testing.
  typedef VectorParameterParser<ConcreteVectorBoolParam> ConcreteVectorBoolParamParser;

public:

  void testParsesParmeterValue1D()
  {
    ConcreteVectorDblParamParser parser;
    ConcreteVectorDblParam* product = parser.parseVectorParameter("1");
    double v1 = (*product)[0];
    TS_ASSERT_EQUALS(1, v1 );
    delete product;
  }

  void testParsesParmeterValue2D()
  {
    ConcreteVectorDblParamParser parser;
    ConcreteVectorDblParam* product = parser.parseVectorParameter("1,2");
    double v1 = (*product)[0];
    double v2 = (*product)[1];
    TS_ASSERT_EQUALS(1, v1 );
    TS_ASSERT_EQUALS(2, v2 );
    delete product;
  }

  void testParsesParmeterValue3D()
  {
    ConcreteVectorDblParamParser parser;
    ConcreteVectorDblParam* product = parser.parseVectorParameter("1,2,3");
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
    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>ConcreteVectorDblParam</Type><Value>1, 2, 3</Value></Parameter>";
    Poco::AutoPtr<Document> pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorDblParamParser parser;
    ImplicitFunctionParameter * product = parser.createParameter(pRootElem);

    ConcreteVectorDblParam* actualProduct = dynamic_cast<ConcreteVectorDblParam*>(product);

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
    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>SucessorVectorParameter</Type><Value>1, 2, 3</Value></Parameter>";
    Poco::AutoPtr<Document> pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorDblParamParser parser;
    TSM_ASSERT_THROWS("No successor, so should throw!", parser.createParameter(pRootElem), std::runtime_error);
  }

  DECLARE_VECTOR_PARAMETER(SucessorVectorParameter, double)

  void testChainOfResponsibility()
  {
    //Local declare of a successor parser with a successor parameter.
    typedef VectorParameterParser<SucessorVectorParameter> ConcreteSuccessorVectorParameterParser;

    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>SucessorVectorParameter</Type><Value>1, 2, 3</Value></Parameter>";
    Poco::AutoPtr<Document> pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorDblParamParser parser;

    parser.setSuccessorParser(new ConcreteSuccessorVectorParameterParser);
    Mantid::API::ImplicitFunctionParameter* product = parser.createParameter(pRootElem);

    TSM_ASSERT("Product should be a SucessorVectorParameter", dynamic_cast<SucessorVectorParameter*>(product));
    delete product;
  }

  void testSuccessfulParseBools()
  {
    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>ConcreteVectorBoolParam</Type><Value>1, 0, 1, 0</Value></Parameter>";
    Poco::AutoPtr<Document> pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorBoolParamParser parser;
    ImplicitFunctionParameter * product = parser.createParameter(pRootElem);

    ConcreteVectorBoolParam* actualProduct = dynamic_cast<ConcreteVectorBoolParam*>(product);

    TSM_ASSERT("The wrong product parameter has been produced", (actualProduct != NULL));

    bool v1 = (*actualProduct)[0];
    bool v2 = (*actualProduct)[1];
    bool v3 = (*actualProduct)[2];
    bool v4 = (*actualProduct)[3];

    TS_ASSERT_EQUALS(true, v1);
    TS_ASSERT_EQUALS(false, v2);
    TS_ASSERT_EQUALS(true, v3);
    TS_ASSERT_EQUALS(false, v4);
    delete actualProduct;
  }

  void testCreateWithoutDelegationThrows()
  {
    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>OTHER</Type><Value>1, 0, 1, 0</Value></Parameter>";
    Poco::AutoPtr<Document> pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorBoolParamParser parser;
    TSM_ASSERT_THROWS("Should throw since delegation is not possible.", parser.createWithoutDelegation(pRootElem), std::runtime_error);
  }

  void testCreateWithoutDelegation()
  {
    DOMParser pParser;
    std::string xmlToParse = "<Parameter><Type>ConcreteVectorDblParam</Type><Value>1, 0, 1, 0</Value></Parameter>";
    Poco::AutoPtr<Document> pDoc = pParser.parseString(xmlToParse);
    Element* pRootElem = pDoc->documentElement();

    ConcreteVectorDblParamParser parser;
    ConcreteVectorDblParam * product = parser.createWithoutDelegation(pRootElem);
    TS_ASSERT(product != NULL);
    delete product;
  }

};

#endif
