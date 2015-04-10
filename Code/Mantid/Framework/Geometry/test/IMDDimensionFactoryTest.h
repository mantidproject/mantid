#ifndef IMD_DIMENSIONFACTORYTEST_H_
#define IMD_DIMENSIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidKernel/UnitLabel.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/XML/XMLException.h>

using namespace Mantid::Geometry;

class IMDDimensionFactoryTest: public CxxTest::TestSuite
{
private:

  std::string constructDimensionWithUnitsXMLString()
  {
    std::string xmlToParse = std::string("<Dimension ID=\"qz\">") + "<Name>Qz</Name>"
        + "<Units>Cubits</Units>"
        + "<UpperBounds>3</UpperBounds>" + "<LowerBounds>-3</LowerBounds>"
        + "<NumberOfBins>8</NumberOfBins>"
        + "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" + "</Dimension>";

    return xmlToParse;
  }

  std::string constructDimensionWithoutUnitsXMLString()
  {
    std::string xmlToParse = std::string("<Dimension ID=\"qz\">") + "<Name>Qz</Name>"
        + "<UpperBounds>3</UpperBounds>" + "<LowerBounds>-3</LowerBounds>"
        + "<NumberOfBins>8</NumberOfBins>"
        + "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" + "</Dimension>";

    return xmlToParse;
  }

  std::string constructNonReciprocalDimensionXMLString()
  {
    return std::string("<Dimension ID=\"en\">") + "<Name>Energy</Name>"
        + "<UpperBounds>150</UpperBounds>" + "<LowerBounds>0</LowerBounds>"
        + "<NumberOfBins>4</NumberOfBins>" + "</Dimension>";
  }

  Poco::AutoPtr<Poco::XML::Document> constructNonReciprocalDimensionXML()
  {
    std::string xmlToParse = constructNonReciprocalDimensionXMLString();
    Poco::XML::DOMParser pParser;
    return pParser.parseString(xmlToParse);
  }

public:

  void testCorrectGeneration()
  {
    IMDDimension_const_sptr dimension = createDimension(constructDimensionWithUnitsXMLString());
    TS_ASSERT_EQUALS("Cubits", dimension->getUnits().ascii());
    TS_ASSERT_EQUALS("Qz", dimension->getName());
    TS_ASSERT_EQUALS("qz", dimension->getDimensionId());
    TS_ASSERT_EQUALS(-3, dimension->getMinimum());
    TS_ASSERT_EQUALS(3, dimension->getMaximum());
    TS_ASSERT_EQUALS(8, dimension->getNBins());
  }

  void testCorrectGenerationWithoutUnits()
  {
    IMDDimension_const_sptr dimension = createDimension(constructDimensionWithoutUnitsXMLString());
    TS_ASSERT_EQUALS("None", dimension->getUnits().ascii());
    TS_ASSERT_EQUALS("Qz", dimension->getName());
    TS_ASSERT_EQUALS("qz", dimension->getDimensionId());
    TS_ASSERT_EQUALS(-3, dimension->getMinimum());
    TS_ASSERT_EQUALS(3, dimension->getMaximum());
    TS_ASSERT_EQUALS(8, dimension->getNBins());
  }

  void testCreationViaStringVsElement()
  {
    std::string xmlToParse = constructNonReciprocalDimensionXMLString();
    IMDDimension_const_sptr viaString = createDimension(xmlToParse);
    auto document = constructNonReciprocalDimensionXML();
    IMDDimension_const_sptr viaXML = createDimension(*document->documentElement());

    //Constructed either way, the products should be equivalent
    TSM_ASSERT_EQUALS("Created through either route, the products should be equal", viaString->getDimensionId(), viaXML->getDimensionId());
  }

  void testOverrideMethod()
  {
    IMDDimension_const_sptr dimension = createDimension(constructDimensionWithUnitsXMLString(),10,-9.0,8.5);
    TS_ASSERT_EQUALS("Cubits", dimension->getUnits().ascii());
    TS_ASSERT_EQUALS("Qz", dimension->getName());
    TS_ASSERT_EQUALS("qz", dimension->getDimensionId());
    TS_ASSERT_EQUALS(-9.0, dimension->getMinimum());
    TS_ASSERT_EQUALS(8.5, dimension->getMaximum());
    TS_ASSERT_EQUALS(10, dimension->getNBins());
  }

  void testPassInvalidString()
  {
    TS_ASSERT_THROWS( createDimension(""), std::invalid_argument );
    TS_ASSERT_THROWS( createDimension("garbage"), std::invalid_argument );

    std::string missingID = constructNonReciprocalDimensionXMLString().erase(10,8);
    TS_ASSERT_THROWS( createDimension(missingID), std::invalid_argument );
    std::string missingName = constructNonReciprocalDimensionXMLString().erase(19,19);
    TS_ASSERT_THROWS( createDimension(missingName), std::invalid_argument );
    std::string missingUpperBounds = constructNonReciprocalDimensionXMLString().erase(38,30);
    TS_ASSERT_THROWS( createDimension(missingUpperBounds), std::invalid_argument );
    std::string missingUpperBoundsValue = constructNonReciprocalDimensionXMLString().erase(51,3);
    TS_ASSERT_THROWS( createDimension(missingUpperBoundsValue), std::invalid_argument );
    std::string missingLowerBounds = constructNonReciprocalDimensionXMLString().erase(68,28);
    TS_ASSERT_THROWS( createDimension(missingLowerBounds), std::invalid_argument );
    std::string missingLowerBoundsValue = constructNonReciprocalDimensionXMLString().erase(81,1);
    TS_ASSERT_THROWS( createDimension(missingLowerBoundsValue), std::invalid_argument );
    std::string missingNumberOfBins = constructNonReciprocalDimensionXMLString().erase(96,30);
    TS_ASSERT_THROWS( createDimension(missingNumberOfBins), std::invalid_argument );
    std::string missingNumberOfBinsValue = constructNonReciprocalDimensionXMLString().erase(110,1);
    TS_ASSERT_THROWS( createDimension(missingNumberOfBins), std::invalid_argument );
  }
};
#endif
