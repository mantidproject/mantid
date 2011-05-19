#ifndef MD_GEOMETRYXMLBUILDER_TEST_H_
#define MD_GEOMETRYXMLBUILDER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <string>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::Geometry;
using namespace testing;

class MDGeometryXMLBuilderTest : public CxxTest::TestSuite
{
private:

  /// Mock IMDDimension allows tests to specify exact expected behavior of dependency.
  class MockIMDDimension : public Mantid::Geometry::IMDDimension {
  public:
    MOCK_CONST_METHOD0(getName,
      std::string());
    MOCK_CONST_METHOD0(getUnits,
      std::string());
    MOCK_CONST_METHOD0(getDimensionId,
      std::string());
    MOCK_CONST_METHOD0(getMaximum,
      double());
    MOCK_CONST_METHOD0(getMinimum,
      double());
    MOCK_CONST_METHOD0(getNBins,
      size_t());
    MOCK_CONST_METHOD0(toXMLString,
      std::string());
    MOCK_CONST_METHOD0(getIsIntegrated,
      bool());
    MOCK_CONST_METHOD1(getX,
      double(size_t ind));
  };

static std::string createDimensionXMLString(unsigned int nbins, int min, int max, std::string name, std::string id)
{
  std::string xmlstream = std::string("<Dimension ID=\"%s\">") +
  "<Name>%s</Name>" +
  "<UpperBounds>%i</UpperBounds>" +
  "<LowerBounds>%i</LowerBounds>" +
  "<NumberOfBins>%i</NumberOfBins>" +
  "</Dimension>";

  std::string formattedXMLString = boost::str(boost::format(xmlstream.c_str()) % id % name % max % min % nbins);
  return formattedXMLString;
}

public:

void testCannotAddDimensionTwice()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillRepeatedly(Return("a"));
  IMDDimension_const_sptr dimension(pDimensionX);
  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  TSM_ASSERT("Addition of dimension to empty set should have succeeded.", builder.addOrdinaryDimension(dimension));
  TSM_ASSERT("Addition of same dimension to set should have failed.", !builder.addOrdinaryDimension(dimension));
}

void testAddingNullDimensionReturnsFalse()
{
  Mantid::Geometry::IMDDimension* pDim = NULL;
  IMDDimension_const_sptr nullDimension(pDim);
  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  TSM_ASSERT("Adding null dimension should return false.", !builder.addOrdinaryDimension(nullDimension));
  TSM_ASSERT("Adding null x dimension should return false.", !builder.addXDimension(nullDimension));
  TSM_ASSERT("Adding null y dimension should return false.", !builder.addYDimension(nullDimension));
  TSM_ASSERT("Adding null z dimension should return false.", !builder.addZDimension(nullDimension));
  TSM_ASSERT("Adding null t dimension should return false.", !builder.addTDimension(nullDimension));
}

void testStrictPolicy()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillRepeatedly(Return("a"));
  EXPECT_CALL(*pDimensionX, getIsIntegrated()).WillRepeatedly(Return(true));
  IMDDimension_const_sptr dimension(pDimensionX);

  MDGeometryBuilderXML<StrictDimensionPolicy> builder;
  TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the x mapping, which is integrated.", builder.addXDimension(dimension), std::invalid_argument);
  TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the y mapping, which is integrated.", builder.addYDimension(dimension), std::invalid_argument);
  TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the z mapping, which is integrated.", builder.addZDimension(dimension), std::invalid_argument);
  TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the t mapping, which is integrated.", builder.addTDimension(dimension), std::invalid_argument);
}

//Same as test above, but shouldn't throw.
void testNoPolicy()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillRepeatedly(Return("a"));
  EXPECT_CALL(*pDimensionX, getIsIntegrated()).WillRepeatedly(Return(true));
  IMDDimension_const_sptr dimension(pDimensionX);

  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension to the x mapping, which is integrated.", builder.addXDimension(dimension));
  TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension to the y mapping, which is integrated.", builder.addYDimension(dimension));
  TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension to the z mapping, which is integrated.", builder.addZDimension(dimension));
  TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension to the t mapping, which is integrated.", builder.addTDimension(dimension));
}


void testWithOrinaryDimensionOnly()
{
  MockIMDDimension* pDimensionOrdinary = new MockIMDDimension;

  EXPECT_CALL(*pDimensionOrdinary, toXMLString()).Times(1).WillOnce(Return(createDimensionXMLString(1, -1, 1, "O", "o")));

  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  builder.addOrdinaryDimension(IMDDimension_const_sptr(pDimensionOrdinary));
  Poco::XML::DOMParser pParser;
  std::string xmlToParse = builder.create(); //Serialize the geometry.

  Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
  Poco::XML::Element* pRootElem = pDoc->documentElement();

  //Check that the number of dimensions provided is correct.
  TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 1, pRootElem->getElementsByTagName("Dimension")->length());

  //Check that mapping nodes give correct mappings.
  Poco::XML::Element* dimensionSetElement = pRootElem;
  TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "", dimensionSetElement->getChildElement("XDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "", dimensionSetElement->getChildElement("YDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "", dimensionSetElement->getChildElement("ZDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "", dimensionSetElement->getChildElement("TDimension")->getChildElement("RefDimensionId")->innerText());
}


void testWithXDimensionOnly()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;

  EXPECT_CALL(*pDimensionX, toXMLString()).WillOnce(Return(createDimensionXMLString(1, -1, 1, "A", "a")));
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillOnce(Return("a"));

  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
  Poco::XML::DOMParser pParser;
  std::string xmlToParse = builder.create(); //Serialize the geometry.

  Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
  Poco::XML::Element* pRootElem = pDoc->documentElement();

  //Check that the number of dimensions provided is correct.
  TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 1, pRootElem->getElementsByTagName("Dimension")->length());

  //Check that mapping nodes give correct mappings.
  Poco::XML::Element* dimensionSetElement = pRootElem;
  TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a", dimensionSetElement->getChildElement("XDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "", dimensionSetElement->getChildElement("YDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "", dimensionSetElement->getChildElement("ZDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "", dimensionSetElement->getChildElement("TDimension")->getChildElement("RefDimensionId")->innerText());
}

void testWithXYDimensionOnly()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;
  MockIMDDimension* pDimensionY = new MockIMDDimension;

  EXPECT_CALL(*pDimensionX, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "A", "a")));
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillRepeatedly(Return("a"));

  EXPECT_CALL(*pDimensionY, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "B", "b")));
  EXPECT_CALL(*pDimensionY, getDimensionId()).WillRepeatedly(Return("b"));

  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
  builder.addYDimension(IMDDimension_const_sptr(pDimensionY));

  Poco::XML::DOMParser pParser;
  std::string xmlToParse = builder.create(); //Serialize the geometry.

  Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
  Poco::XML::Element* pRootElem = pDoc->documentElement();

  //Check that the number of dimensions provided is correct.
  TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 2, pRootElem->getElementsByTagName("Dimension")->length());

  //Check that mapping nodes give correct mappings.
  Poco::XML::Element* dimensionSetElement = pRootElem;
  TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a", dimensionSetElement->getChildElement("XDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "b", dimensionSetElement->getChildElement("YDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "", dimensionSetElement->getChildElement("ZDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "", dimensionSetElement->getChildElement("TDimension")->getChildElement("RefDimensionId")->innerText());
}

void testWithXYZDimensionOnly()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;
  MockIMDDimension* pDimensionY = new MockIMDDimension;
  MockIMDDimension* pDimensionZ = new MockIMDDimension;

  EXPECT_CALL(*pDimensionX, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "A", "a")));
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillRepeatedly(Return("a"));

  EXPECT_CALL(*pDimensionY, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "B", "b")));
  EXPECT_CALL(*pDimensionY, getDimensionId()).WillRepeatedly(Return("b"));

  EXPECT_CALL(*pDimensionZ, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "C", "c")));
  EXPECT_CALL(*pDimensionZ, getDimensionId()).WillRepeatedly(Return("c"));

  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
  builder.addYDimension(IMDDimension_const_sptr(pDimensionY));
  builder.addZDimension(IMDDimension_const_sptr(pDimensionZ));

  Poco::XML::DOMParser pParser;
  std::string xmlToParse = builder.create(); //Serialize the geometry.

  Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
  Poco::XML::Element* pRootElem = pDoc->documentElement();

  //Check that the number of dimensions provided is correct.
  TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 3, pRootElem->getElementsByTagName("Dimension")->length());

  //Check that mapping nodes give correct mappings.
  Poco::XML::Element* dimensionSetElement = pRootElem;
  TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a", dimensionSetElement->getChildElement("XDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "b", dimensionSetElement->getChildElement("YDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "c", dimensionSetElement->getChildElement("ZDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "", dimensionSetElement->getChildElement("TDimension")->getChildElement("RefDimensionId")->innerText());
}

void testFullCreate()
{
  MockIMDDimension* pDimensionX = new MockIMDDimension;
  MockIMDDimension* pDimensionY = new MockIMDDimension;
  MockIMDDimension* pDimensionZ = new MockIMDDimension;
  MockIMDDimension* pDimensionT = new MockIMDDimension;

  EXPECT_CALL(*pDimensionX, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "A", "a")));
  EXPECT_CALL(*pDimensionX, getDimensionId()).WillRepeatedly(Return("a"));

  EXPECT_CALL(*pDimensionY, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "B", "b")));
  EXPECT_CALL(*pDimensionY, getDimensionId()).WillRepeatedly(Return("b"));

  EXPECT_CALL(*pDimensionZ, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "C", "c")));
  EXPECT_CALL(*pDimensionZ, getDimensionId()).WillRepeatedly(Return("c"));

  EXPECT_CALL(*pDimensionT, toXMLString()).WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "D", "d")));
  EXPECT_CALL(*pDimensionT, getDimensionId()).WillRepeatedly(Return("d"));

  MDGeometryBuilderXML<NoDimensionPolicy> builder;
  builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
  builder.addYDimension(IMDDimension_const_sptr(pDimensionY));
  builder.addZDimension(IMDDimension_const_sptr(pDimensionZ));
  builder.addTDimension(IMDDimension_const_sptr(pDimensionT));

  //Only practicle way to check the xml output in the absense of xsd is as part of a dom tree.
  Poco::XML::DOMParser pParser;
  std::string xmlToParse = builder.create(); //Serialize the geometry.

  Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
  Poco::XML::Element* pRootElem = pDoc->documentElement();

  //Check that the number of dimensions provided is correct.
  TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 4, pRootElem->getElementsByTagName("Dimension")->length());

  //Check that mapping nodes have been provided.
  TSM_ASSERT_EQUALS("No DimensionX in geometry xml", 1, pRootElem->getElementsByTagName("XDimension")->length());
  TSM_ASSERT_EQUALS("No DimensionY in geometry xml", 1, pRootElem->getElementsByTagName("YDimension")->length());
  TSM_ASSERT_EQUALS("No DimensionZ in geometry xml", 1, pRootElem->getElementsByTagName("ZDimension")->length());
  TSM_ASSERT_EQUALS("No DimensionT in geometry xml", 1, pRootElem->getElementsByTagName("TDimension")->length());

  //Check that mapping nodes give correct mappings.
  Poco::XML::Element* dimensionSetElement = pRootElem;
  TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a", dimensionSetElement->getChildElement("XDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("No DimensionY mapping is incorrect", "b", dimensionSetElement->getChildElement("YDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("No DimensionZ mapping is incorrect", "c", dimensionSetElement->getChildElement("ZDimension")->getChildElement("RefDimensionId")->innerText());
  TSM_ASSERT_EQUALS("No DimensionT mapping is incorrect", "d", dimensionSetElement->getChildElement("TDimension")->getChildElement("RefDimensionId")->innerText());
}

};

#endif /* MD_GEOMETRYXMLBUILDER_TEST_H_ */
