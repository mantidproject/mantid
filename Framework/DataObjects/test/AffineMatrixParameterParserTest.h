// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

#include "MantidDataObjects/AffineMatrixParameterParser.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;

class AffineMatrixParameterParserTest : public CxxTest::TestSuite {
public:
  void testParse2by2() {
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><Parameter><Type>"
                             "AffineMatrixParameter</Type><Value>1,2;3,4;5,6</"
                             "Value></Parameter>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    AffineMatrixParameterParser parser;
    AffineMatrixParameter *parameter = parser.createParameter(pRootElem);

    AffineMatrixType product = parameter->getAffineMatrix();

    // Check that matrix is recovered.
    TS_ASSERT_EQUALS(1, product[0][0]);
    TS_ASSERT_EQUALS(2, product[0][1]);
    TS_ASSERT_EQUALS(3, product[1][0]);
    TS_ASSERT_EQUALS(4, product[1][1]);
    TS_ASSERT_EQUALS(5, product[2][0]);
    TS_ASSERT_EQUALS(6, product[2][1]);

    delete parameter;
  }

  void testParse3by3() {
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><Parameter><Type>AffineMatrixParameter</"
                             "Type><Value>1,2,3;4,5,6;7,8,9</Value></Parameter>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    AffineMatrixParameterParser parser;
    AffineMatrixParameter *parameter = parser.createParameter(pRootElem);

    AffineMatrixType product = parameter->getAffineMatrix();

    // Check that matrix is recovered.
    TS_ASSERT_EQUALS(1, product[0][0]);
    TS_ASSERT_EQUALS(2, product[0][1]);
    TS_ASSERT_EQUALS(3, product[0][2]);
    TS_ASSERT_EQUALS(4, product[1][0]);
    TS_ASSERT_EQUALS(5, product[1][1]);
    TS_ASSERT_EQUALS(6, product[1][2]);
    TS_ASSERT_EQUALS(7, product[2][0]);
    TS_ASSERT_EQUALS(8, product[2][1]);
    TS_ASSERT_EQUALS(9, product[2][2]);

    delete parameter;
  }

  void testParse4by4() {
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><Parameter><Type>AffineMatrixParameter</"
                             "Type><Value>1,2,3,4;5,6,7,8;9,10,11,12</Value></Parameter>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    AffineMatrixParameterParser parser;
    AffineMatrixParameter *parameter = parser.createParameter(pRootElem);

    AffineMatrixType product = parameter->getAffineMatrix();

    // Check that matrix is recovered.
    TS_ASSERT_EQUALS(1, product[0][0]);
    TS_ASSERT_EQUALS(2, product[0][1]);
    TS_ASSERT_EQUALS(3, product[0][2]);
    TS_ASSERT_EQUALS(4, product[0][3]);
    TS_ASSERT_EQUALS(5, product[1][0]);
    TS_ASSERT_EQUALS(6, product[1][1]);
    TS_ASSERT_EQUALS(7, product[1][2]);
    TS_ASSERT_EQUALS(8, product[1][3]);
    TS_ASSERT_EQUALS(9, product[2][0]);
    TS_ASSERT_EQUALS(10, product[2][1]);
    TS_ASSERT_EQUALS(11, product[2][2]);
    TS_ASSERT_EQUALS(12, product[2][3]);

    delete parameter;
  }

  void testThrowsOnCallSetSuccessor() {
    AffineMatrixParameterParser parser;
    AffineMatrixParameterParser otherParser;
    TS_ASSERT_THROWS(parser.setSuccessorParser(&otherParser), const std::runtime_error &);
  }

  void testThrowsIfWrongXML() {
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = "<?xml version=\"1.0\" "
                             "encoding=\"utf-8\"?><Parameter><Type>SOME_OTHER_"
                             "PARAMETER_TYPE</Type><Value></Value></Parameter>";
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    AffineMatrixParameterParser parser;

    TS_ASSERT_THROWS(parser.createParameter(pRootElem), const std::runtime_error &);
  }
};
