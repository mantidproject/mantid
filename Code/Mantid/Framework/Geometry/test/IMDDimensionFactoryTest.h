#ifndef IMD_DIMENSIONFACTORYTEST_H_
#define IMD_DIMENSIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>

using namespace Mantid::Geometry;

class IMDDimensionFactoryTest: public CxxTest::TestSuite
{
private:

  static Poco::XML::Element* constructDimensionWithUnits()
  {
    std::string xmlToParse = std::string("<Dimension ID=\"qz\">") + "<Name>Qz</Name>"
        + "<Units>Cubits</Units>"
        + "<UpperBounds>3</UpperBounds>" + "<LowerBounds>-3</LowerBounds>"
        + "<NumberOfBins>8</NumberOfBins>"
        + "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" + "</Dimension>";

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    return pDoc->documentElement();
  }

  static Poco::XML::Element* constructDimensionWithoutUnits()
  {
    std::string xmlToParse = std::string("<Dimension ID=\"qz\">") + "<Name>Qz</Name>"
        + "<UpperBounds>3</UpperBounds>" + "<LowerBounds>-3</LowerBounds>"
        + "<NumberOfBins>8</NumberOfBins>"
        + "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" + "</Dimension>";

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    return pDoc->documentElement();
  }

  static Poco::XML::Element* constructReciprocalDimensionXML()
  {
    std::string xmlToParse = std::string("<Dimension ID=\"qz\">") + "<Name>Qz</Name>"
        + "<UpperBounds>6.6</UpperBounds>" + "<LowerBounds>-6.6</LowerBounds>"
        + "<NumberOfBins>6</NumberOfBins>"
        + "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" + "</Dimension>";

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    return pDoc->documentElement();
  }

  static Poco::XML::Element* constructUnknownReciprocalDimensionXML()
  {
    std::string xmlToParse = std::string("<Dimension ID=\"unknown\">") + "<Name>Qz</Name>"
      + "<UpperBounds>6.6</UpperBounds>" + "<LowerBounds>-6.6</LowerBounds>"
      + "<NumberOfBins>6</NumberOfBins>"
      + "<ReciprocalDimensionMapping>unknown</ReciprocalDimensionMapping>" + "</Dimension>";

    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    return pDoc->documentElement();
  }

  static std::string constructNonReciprocalDimensionXMLString()
  {
    return std::string("<Dimension ID=\"en\">") + "<Name>Energy</Name>"
        + "<UpperBounds>150</UpperBounds>" + "<LowerBounds>0</LowerBounds>"
        + "<NumberOfBins>4</NumberOfBins>" + "</Dimension>";
  }

  static Poco::XML::Element* constructNonReciprocalDimensionXML()
  {
    std::string xmlToParse = constructNonReciprocalDimensionXMLString();
    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    return pDoc->documentElement();
  }

public:

  void testCorrectGeneration()
  {
    using namespace Mantid::Geometry;
    IMDDimensionFactory factory(constructDimensionWithUnits());
    IMDDimension* dimension = factory.create();
    TS_ASSERT_EQUALS("Cubits", dimension->getUnits());
    TS_ASSERT_EQUALS("Qz", dimension->getName());
    TS_ASSERT_EQUALS("qz", dimension->getDimensionId());
    TS_ASSERT_EQUALS(-3, dimension->getMinimum());
    TS_ASSERT_EQUALS(3, dimension->getMaximum());
    TS_ASSERT_EQUALS(8, dimension->getNBins());
    delete dimension;
  }

  void testCorrectGenerationWithoutUnits()
  {
    using namespace Mantid::Geometry;
    IMDDimensionFactory factory(constructDimensionWithoutUnits());
    IMDDimension* dimension = factory.create();
    TS_ASSERT_EQUALS("None", dimension->getUnits());
    TS_ASSERT_EQUALS("Qz", dimension->getName());
    TS_ASSERT_EQUALS("qz", dimension->getDimensionId());
    TS_ASSERT_EQUALS(-3, dimension->getMinimum());
    TS_ASSERT_EQUALS(3, dimension->getMaximum());
    TS_ASSERT_EQUALS(8, dimension->getNBins());
    delete dimension;
  }

  void testStaticCreation()
  {
    std::string xmlToParse = constructNonReciprocalDimensionXMLString();

    IMDDimensionFactory factoryA = IMDDimensionFactory::createDimensionFactory(xmlToParse);
    IMDDimensionFactory factoryB(constructNonReciprocalDimensionXML());

    //Constructed either way, the products should be equivalent
    TSM_ASSERT_EQUALS("Created through either route, the products should be equal", factoryA.create()->getDimensionId(), factoryB.create()->getDimensionId());

  }
};
#endif
