#ifndef DIMENSIONFACTORYTEST_H_
#define DIMENSIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/DimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>

using namespace Mantid::MDAlgorithms;

class DimensionFactoryTest: public CxxTest::TestSuite
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

  void testCreationOfReciprocalMDDimension()
  {
    using namespace Mantid::Geometry;
    DimensionFactory factory(constructReciprocalDimensionXML());
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
    DimensionFactory factory(constructNonReciprocalDimensionXML());
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

    DimensionFactory factoryA = DimensionFactory::createDimensionFactory(xmlToParse);
    DimensionFactory factoryB(constructNonReciprocalDimensionXML());

    //Constructed either way, the products should be equivalent
    TSM_ASSERT_EQUALS("Created through either route, the products should be equal", factoryA.create()->getDimensionId(), factoryB.create()->getDimensionId());

  }
};
#endif
