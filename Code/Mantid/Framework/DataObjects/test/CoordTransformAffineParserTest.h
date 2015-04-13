#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMPARSERTEST_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMPARSERTEST_H_

#include "MantidAPI/CoordTransform.h"
#include "MantidDataObjects/CoordTransformAffineParser.h"
#include "MantidDataObjects/CoordTransformAffine.h"

#include <cxxtest/TestSuite.h>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using namespace Mantid::DataObjects;

class CoordTransformAffineParserTest : public CxxTest::TestSuite
{
private:

  class MockCoordTransformAffineParser : public CoordTransformAffineParser
  {
    virtual Mantid::API::CoordTransform* createTransform(Poco::XML::Element*) const
    {
      return new CoordTransformAffine(1, 1);
    }
  };

public:

  void testSuccessfulParse()
  {
    std::string xmlToParse = std::string("<CoordTransform>") +
    "<Type>CoordTransformAffine</Type>" +
    "<ParameterList>" +
    "<Parameter><Type>InDimParameter</Type><Value>2</Value></Parameter>" + 
    "<Parameter><Type>OutDimParameter</Type><Value>2</Value></Parameter>" + 
    "<Parameter><Type>AffineMatrixParameter</Type><Value>0,1,2;3,4,5;6,7,8</Value></Parameter>" + 
    "</ParameterList></CoordTransform>";

   Poco::XML::DOMParser pParser;
   Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   CoordTransformAffineParser parser;
   CoordTransformAffine* transform = dynamic_cast<CoordTransformAffine*>(parser.createTransform(pRootElem));

   AffineMatrixType product = transform->getMatrix();

   //Check that matrix is recovered.
   TS_ASSERT_EQUALS(0, product[0][0]);
   TS_ASSERT_EQUALS(1, product[0][1]);
   TS_ASSERT_EQUALS(2, product[0][2]);
   TS_ASSERT_EQUALS(3, product[1][0]);
   TS_ASSERT_EQUALS(4, product[1][1]);
   TS_ASSERT_EQUALS(5, product[1][2]);
   TS_ASSERT_EQUALS(6, product[2][0]);
   TS_ASSERT_EQUALS(7, product[2][1]);
   TS_ASSERT_EQUALS(8, product[2][2]);

   //Circular check. Acutally hard to debug, but gives certainty that serialization and deserialization cause no side effects.
   TSM_ASSERT_EQUALS("Parsing has not occured correctly if the output is not equal to the intput", transform->toXMLString(), xmlToParse);

   delete transform;

  }

  void testNotACoordTransformThrows()
  {
   std::string xmlToParse = std::string("<OTHER></OTHER>");

   Poco::XML::DOMParser pParser;
   Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   CoordTransformAffineParser parser;
   TSM_ASSERT_THROWS("XML root node must be a coordinate transform", parser.createTransform(pRootElem), std::invalid_argument);
  }

  void testNoSuccessorThrows()
  {
   std::string xmlToParse = "<CoordTransform><Type>OTHER</Type></CoordTransform>"; //type is not a coordinate transform, so should try to use it's successor

   Poco::XML::DOMParser pParser;
   Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   CoordTransformAffineParser parser;
   TSM_ASSERT_THROWS("Should throw since no successor parser has been set", parser.createTransform(pRootElem), std::runtime_error);
  }

  void testDelegateToSuccessor()
  {
   std::string xmlToParse = "<CoordTransform><Type>OTHER</Type></CoordTransform>"; //type is not a coordinate transform, so should try to use it's successor

   Poco::XML::DOMParser pParser;
   Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   CoordTransformAffineParser parser;
   parser.setSuccessor(new MockCoordTransformAffineParser);
   Mantid::API::CoordTransform* product = parser.createTransform(pRootElem);
   delete product;
  }

};

#endif
