#ifndef IMD_DIMENSIONFACTORYTEST_H_
#define IMD_DIMENSIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>

using namespace Mantid::Geometry;

class IMDDimensionFactoryTest: public CxxTest::TestSuite
{
private:

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

  void testCreationOfReciprocalMDDimensionThrows()
  {
    using namespace Mantid::Geometry;
    IMDDimensionFactory factory(constructUnknownReciprocalDimensionXML());
    TSM_ASSERT_THROWS("Uses tag/id 'unknown' which should not be possible to match to q1,q2,q3.", factory.create(),  std::runtime_error);
  }

  void testCreationOfReciprocalMDDimension()
  {
    using namespace Mantid::Geometry;
    IMDDimensionFactory factory(constructReciprocalDimensionXML());
    IMDDimension* dimension = factory.create();

    MDDimensionRes* resDimension = dynamic_cast<MDDimensionRes*> (dimension);
    TSM_ASSERT("This should have been of type MDReciprocal dimension", NULL != resDimension);
    TS_ASSERT_EQUALS(6.6, resDimension->getMaximum());
    TS_ASSERT_EQUALS(-6.6, resDimension->getMinimum());
    TS_ASSERT_EQUALS(6, resDimension->getNBins());
  }

  void testCreationOfMDDimension()
  {
    using namespace Mantid::Geometry;
    IMDDimensionFactory factory(constructNonReciprocalDimensionXML());
    IMDDimension* dimension = factory.create();

    MDDimension* resDimension = dynamic_cast<MDDimension*> (dimension);
    TSM_ASSERT("This should have been of type MD dimension", NULL != resDimension);

    TS_ASSERT_EQUALS(150, resDimension->getMaximum());
    TS_ASSERT_EQUALS(0, resDimension->getMinimum());
    TS_ASSERT_EQUALS(4, resDimension->getNBins());
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
