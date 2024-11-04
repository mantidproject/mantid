// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/CoordTransformDistanceParser.h"

#include <cxxtest/TestSuite.h>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using namespace Mantid::DataObjects;

class CoordTransformDistanceParserTest : public CxxTest::TestSuite {
private:
  // Helper type.
  class MockCoordTransformAffineParser : public CoordTransformAffineParser {
    Mantid::API::CoordTransform *createTransform(Poco::XML::Element *) const override {
      return new CoordTransformAffine(1, 1);
    }
  };

public:
  void testSuccessfulParse() {
    std::string xmlToParse = std::string("<CoordTransform>") + "<Type>CoordTransformDistance</Type>" +
                             "<ParameterList>" + "<Parameter><Type>InDimParameter</Type><Value>4</Value></Parameter>" +
                             "<Parameter><Type>OutDimParameter</Type><Value>1</Value></Parameter>" +
                             "<Parameter><Type>CoordCenterVectorParam</"
                             "Type><Value>1.0000,2.0000,2.0000,1.0000</Value></Parameter>" +
                             "<Parameter><Type>DimensionsUsedVectorParam</Type><Value>1,0,0,1</"
                             "Value></Parameter>"
                             "</ParameterList>"
                             "</CoordTransform>";

    Poco::XML::DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CoordTransformDistanceParser parser;
    Mantid::API::CoordTransform *transform = nullptr;
    TS_ASSERT_THROWS_NOTHING(transform = parser.createTransform(pRootElem));

    // Circular check. Acutally hard to debug, but gives certainty that
    // serialization and deserialization cause no side effects.
    TSM_ASSERT_EQUALS("Parsing has not occured correctly if the output is not "
                      "equal to the intput",
                      transform->toXMLString(), xmlToParse);

    delete transform;
  }

  void testNotACoordTransformThrows() {
    std::string xmlToParse = std::string("<OTHER></OTHER>");

    Poco::XML::DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CoordTransformDistanceParser parser;
    TSM_ASSERT_THROWS("XML root node must be a coordinate transform", parser.createTransform(pRootElem),
                      const std::invalid_argument &);
  }

  void testNoSuccessorThrows() {
    std::string xmlToParse = "<CoordTransform><Type>OTHER</Type></CoordTransform>"; // type is not a
                                                                                    // coordinate
                                                                                    // transform, so
                                                                                    // should try to
                                                                                    // use it's
                                                                                    // successor

    Poco::XML::DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CoordTransformDistanceParser parser;
    TSM_ASSERT_THROWS("Should throw since no successor parser has been set", parser.createTransform(pRootElem),
                      const std::runtime_error &);
  }

  void testDelegateToSuccessor() {
    std::string xmlToParse = "<CoordTransform><Type>OTHER</Type></CoordTransform>"; // type is not a
                                                                                    // coordinate
                                                                                    // transform, so
                                                                                    // should try to
                                                                                    // use it's
                                                                                    // successor

    Poco::XML::DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    CoordTransformDistanceParser parser;
    parser.setSuccessor(new MockCoordTransformAffineParser);
    Mantid::API::CoordTransform *product = nullptr;
    TS_ASSERT_THROWS_NOTHING(product = parser.createTransform(pRootElem));
    delete product;
  }
};
