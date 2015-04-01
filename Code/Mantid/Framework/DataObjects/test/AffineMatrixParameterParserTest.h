#ifndef AFFINE_MATRIX_PARAMETER_PARSER_TEST_H
#define AFFINE_MATRIX_PARAMETER_PARSER_TEST_H

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/AffineMatrixParameterParser.h"

using namespace Mantid::DataObjects;

class AffineMatrixParameterParserTest :    public CxxTest::TestSuite
{
public:

 void testParse2by2()
 {
   Poco::XML::DOMParser pParser;
   std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>AffineMatrixParameter</Type><Value>1,2;3,4;5,6</Value></Parameter>";
   Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   AffineMatrixParameterParser parser;
   AffineMatrixParameter* parameter = parser.createParameter(pRootElem);

   AffineMatrixType product = parameter->getAffineMatrix();

   //Check that matrix is recovered.
   TSM_ASSERT(1, product[0][0]);
   TSM_ASSERT(2, product[0][1]);
   TSM_ASSERT(3, product[1][0]);
   TSM_ASSERT(4, product[1][1]);
   TSM_ASSERT(5, product[2][0]);
   TSM_ASSERT(6, product[2][1]);

   delete parameter;
 }

 void testParse3by3()
 {
   Poco::XML::DOMParser pParser;
   std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>AffineMatrixParameter</Type><Value>1,2,3;4,5,6;7,8,9</Value></Parameter>";
   Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   AffineMatrixParameterParser parser;
   AffineMatrixParameter* parameter = parser.createParameter(pRootElem);

   AffineMatrixType product = parameter->getAffineMatrix();

   //Check that matrix is recovered.
   TSM_ASSERT(1, product[0][0]);
   TSM_ASSERT(2, product[0][1]);
   TSM_ASSERT(3, product[0][2]);
   TSM_ASSERT(4, product[1][0]);
   TSM_ASSERT(5, product[1][1]);
   TSM_ASSERT(6, product[1][2]);
   TSM_ASSERT(7, product[2][0]);
   TSM_ASSERT(8, product[2][1]);
   TSM_ASSERT(9, product[2][2]);

   delete parameter;
 }

 void testParse4by4()
 {
   Poco::XML::DOMParser pParser;
   std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>AffineMatrixParameter</Type><Value>1,2,3,4;5,6,7,8;9,10,11,12</Value></Parameter>";
   Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   AffineMatrixParameterParser parser;
   AffineMatrixParameter* parameter = parser.createParameter(pRootElem);

   AffineMatrixType product = parameter->getAffineMatrix();

   //Check that matrix is recovered.
   TSM_ASSERT(1, product[0][0]);
   TSM_ASSERT(2, product[0][1]);
   TSM_ASSERT(3, product[0][2]);
   TSM_ASSERT(4, product[0][3]);
   TSM_ASSERT(5, product[1][0]);
   TSM_ASSERT(6, product[1][1]);
   TSM_ASSERT(7, product[1][2]);
   TSM_ASSERT(8, product[1][3]);
   TSM_ASSERT(9, product[2][0]);
   TSM_ASSERT(10, product[2][1]);
   TSM_ASSERT(11, product[2][2]);
   TSM_ASSERT(12, product[2][3]);

   delete parameter;
 }

 void testThrowsOnCallSetSuccessor()
 {
   AffineMatrixParameterParser parser;
   AffineMatrixParameterParser otherParser;
   TS_ASSERT_THROWS(parser.setSuccessorParser(&otherParser), std::runtime_error);
 }

 void testThrowsIfWrongXML()
 {
   Poco::XML::DOMParser pParser;
   std::string xmlToParse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Parameter><Type>SOME_OTHER_PARAMETER_TYPE</Type><Value></Value></Parameter>";
   Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
   Poco::XML::Element* pRootElem = pDoc->documentElement();

   AffineMatrixParameterParser parser;

   TS_ASSERT_THROWS(parser.createParameter(pRootElem), std::runtime_error);
 }
 
};
#endif