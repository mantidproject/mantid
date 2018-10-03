// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MD_GEOMETRYXMLBUILDER_TEST_H_
#define MD_GEOMETRYXMLBUILDER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

#include <string>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace testing;

class MDGeometryXMLBuilderTest : public CxxTest::TestSuite {
private:
  /// Mock IMDDimension allows tests to specify exact expected behavior of
  /// dependency.
  class MockIMDDimension : public Mantid::Geometry::IMDDimension {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(getUnits, const Mantid::Kernel::UnitLabel());
    MOCK_CONST_METHOD0(getDimensionId, const std::string &());
    MOCK_CONST_METHOD0(getMaximum, coord_t());
    MOCK_CONST_METHOD0(getMinimum, coord_t());
    MOCK_CONST_METHOD0(getNBins, size_t());
    MOCK_CONST_METHOD0(getNBoundaries, size_t());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    MOCK_CONST_METHOD0(getIsIntegrated, bool());
    MOCK_CONST_METHOD1(getX, coord_t(size_t ind));
    MOCK_METHOD3(setRange, void(size_t nBins, coord_t min, coord_t max));
    MOCK_CONST_METHOD0(getMDUnits, const Kernel::MDUnit &());
    MOCK_CONST_METHOD0(getMDFrame, const Geometry::MDFrame &());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };

  static std::string createDimensionXMLString(unsigned int nbins, int min,
                                              int max, std::string name,
                                              std::string id) {
    std::string xmlstream =
        std::string("<Dimension ID=\"%s\">") + "<Name>%s</Name>" +
        "<UpperBounds>%i</UpperBounds>" + "<LowerBounds>%i</LowerBounds>" +
        "<NumberOfBins>%i</NumberOfBins>" + "</Dimension>";

    std::string formattedXMLString = boost::str(
        boost::format(xmlstream.c_str()) % id % name % max % min % nbins);
    return formattedXMLString;
  }

public:
  void testCopyConstruction() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    std::string a{"_a"}, b{"_b"}, c{"_c"}, d{"_d"};
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));
    EXPECT_CALL(*pDimensionX, toXMLString()).WillRepeatedly(Return("_a_xml"));

    auto pDimensionY = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionY), getDimensionId())
        .WillRepeatedly(ReturnRef(b));
    EXPECT_CALL(*pDimensionY, toXMLString()).WillRepeatedly(Return("_b_xml"));

    auto pDimensionZ = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionZ), getDimensionId())
        .WillRepeatedly(ReturnRef(c));
    EXPECT_CALL(*pDimensionZ, toXMLString()).WillRepeatedly(Return("_c_xml"));

    auto pDimensionT = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionT), getDimensionId())
        .WillRepeatedly(ReturnRef(d));
    EXPECT_CALL(*pDimensionT, toXMLString()).WillRepeatedly(Return("_d_xml"));

    MDGeometryBuilderXML<NoDimensionPolicy> original;
    original.addXDimension(IMDDimension_const_sptr(pDimensionX));
    original.addYDimension(IMDDimension_const_sptr(pDimensionY));
    original.addZDimension(IMDDimension_const_sptr(pDimensionZ));
    original.addTDimension(IMDDimension_const_sptr(pDimensionT));

    // Copy the original object.
    MDGeometryBuilderXML<NoDimensionPolicy> copy(original);

    // Test that the outputs of the original and copy are the same.
    TSM_ASSERT_EQUALS(
        "Copy construction has failed to generate a genuine copy.",
        original.create(), copy.create());
  }

  void testAssignment() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    std::string a{"_a"}, b{"_b"}, c{"_c"}, d{"_d"};
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));
    EXPECT_CALL(*pDimensionX, toXMLString()).WillRepeatedly(Return("_a_xml"));

    auto pDimensionY = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionY), getDimensionId())
        .WillRepeatedly(ReturnRef(b));
    EXPECT_CALL(*pDimensionY, toXMLString()).WillRepeatedly(Return("_b_xml"));

    auto pDimensionZ = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionZ), getDimensionId())
        .WillRepeatedly(ReturnRef(c));
    EXPECT_CALL(*pDimensionZ, toXMLString()).WillRepeatedly(Return("_c_xml"));

    auto pDimensionT = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionT), getDimensionId())
        .WillRepeatedly(ReturnRef(d));
    EXPECT_CALL(*pDimensionT, toXMLString()).WillRepeatedly(Return("_d_xml"));

    MDGeometryBuilderXML<NoDimensionPolicy> A;
    A.addXDimension(IMDDimension_const_sptr(pDimensionX));
    A.addYDimension(IMDDimension_const_sptr(pDimensionY));
    A.addZDimension(IMDDimension_const_sptr(pDimensionZ));
    A.addTDimension(IMDDimension_const_sptr(pDimensionT));

    MDGeometryBuilderXML<NoDimensionPolicy> B;
    B = A;

    // Test that the outputs of the original and the one ovewritten are the
    // same.
    TSM_ASSERT_EQUALS("Assignment has failed to clone the original.",
                      A.create(), B.create());
  }

  void testCannotAddSameDimensionMultipleTimes() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    std::string a{"a"};
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));
    IMDDimension_const_sptr dimension(pDimensionX);

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    TSM_ASSERT("Addition of dimension to empty set should have succeeded.",
               builder.addOrdinaryDimension(dimension));
    TSM_ASSERT(
        "Addition of same dimension to set should have failed.",
        !builder.addOrdinaryDimension(dimension)); // Test can re-Add/overwrite.
  }

  void testAddingNullDimensionReturnsFalse() {
    Mantid::Geometry::IMDDimension *pDim = nullptr;
    IMDDimension_const_sptr nullDimension(pDim);
    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    TSM_ASSERT("Adding null dimension should return false.",
               !builder.addOrdinaryDimension(nullDimension));
    TSM_ASSERT("Adding null x dimension should return false.",
               !builder.addXDimension(nullDimension));
    TSM_ASSERT("Adding null y dimension should return false.",
               !builder.addYDimension(nullDimension));
    TSM_ASSERT("Adding null z dimension should return false.",
               !builder.addZDimension(nullDimension));
    TSM_ASSERT("Adding null t dimension should return false.",
               !builder.addTDimension(nullDimension));
  }

  void testStrictPolicy() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    std::string a{"a"};
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));
    EXPECT_CALL(*pDimensionX, getIsIntegrated()).WillRepeatedly(Return(true));
    IMDDimension_const_sptr dimension(pDimensionX);

    MDGeometryBuilderXML<StrictDimensionPolicy> builder;
    TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the "
                      "x mapping, which is integrated.",
                      builder.addXDimension(dimension), std::invalid_argument);
    TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the "
                      "y mapping, which is integrated.",
                      builder.addYDimension(dimension), std::invalid_argument);
    TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the "
                      "z mapping, which is integrated.",
                      builder.addZDimension(dimension), std::invalid_argument);
    TSM_ASSERT_THROWS("Strict policy should prevent add of a dimension to the "
                      "t mapping, which is integrated.",
                      builder.addTDimension(dimension), std::invalid_argument);
  }

  // Same as test above, but shouldn't throw.
  void testNoPolicy() {
    std::string a{"a"};
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));
    EXPECT_CALL(*pDimensionX, getIsIntegrated()).WillRepeatedly(Return(true));
    IMDDimension_const_sptr dimension(pDimensionX);

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension "
                              "to the x mapping, which is integrated.",
                              builder.addXDimension(dimension));
    TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension "
                              "to the y mapping, which is integrated.",
                              builder.addYDimension(dimension));
    TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension "
                              "to the z mapping, which is integrated.",
                              builder.addZDimension(dimension));
    TSM_ASSERT_THROWS_NOTHING("Strict policy should prevent add of a dimension "
                              "to the t mapping, which is integrated.",
                              builder.addTDimension(dimension));
  }

  void testWithOrinaryDimensionOnly() {
    auto pDimensionOrdinary = boost::make_shared<MockIMDDimension>();

    EXPECT_CALL(*pDimensionOrdinary, toXMLString())
        .Times(1)
        .WillOnce(Return(createDimensionXMLString(1, -1, 1, "O", "o")));

    std::string o{"o"};
    EXPECT_CALL(Const(*pDimensionOrdinary), getDimensionId())
        .WillRepeatedly(ReturnRef(o));

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    builder.addOrdinaryDimension(IMDDimension_const_sptr(pDimensionOrdinary));
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = builder.create(); // Serialize the geometry.

    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    // Check that the number of dimensions provided is correct.
    Poco::AutoPtr<Poco::XML::NodeList> dimension =
        pRootElem->getElementsByTagName("Dimension");
    TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 1,
                      dimension->length());

    // Check that mapping nodes give correct mappings.
    TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "",
                      pRootElem->getChildElement("XDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "",
                      pRootElem->getChildElement("YDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "",
                      pRootElem->getChildElement("ZDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "",
                      pRootElem->getChildElement("TDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
  }

  void testManyOrinaryDimensions() {
    auto pDimA = boost::make_shared<MockIMDDimension>();
    auto pDimB = boost::make_shared<MockIMDDimension>();
    auto pDimC = boost::make_shared<MockIMDDimension>();
    std::string a{"a"}, b{"b"}, c{"c"};
    EXPECT_CALL(Const(*pDimA), getDimensionId()).WillRepeatedly(ReturnRef(a));
    EXPECT_CALL(Const(*pDimB), getDimensionId()).WillRepeatedly(ReturnRef(b));
    EXPECT_CALL(Const(*pDimC), getDimensionId()).WillRepeatedly(ReturnRef(c));

    EXPECT_CALL(*pDimA, toXMLString())
        .Times(1)
        .WillOnce(Return(createDimensionXMLString(1, -1, 1, "A", "a")));
    EXPECT_CALL(*pDimB, toXMLString())
        .Times(1)
        .WillOnce(Return(createDimensionXMLString(1, -1, 1, "B", "b")));
    EXPECT_CALL(*pDimC, toXMLString())
        .Times(1)
        .WillOnce(Return(createDimensionXMLString(1, -1, 1, "C", "c")));

    VecIMDDimension_sptr vecDims;
    vecDims.push_back(IMDDimension_sptr(pDimA));
    vecDims.push_back(IMDDimension_sptr(pDimB));
    vecDims.push_back(IMDDimension_sptr(pDimC));

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    builder.addManyOrdinaryDimensions(vecDims);

    TS_ASSERT_THROWS_NOTHING(builder.create()); // Serialize the geometry.
    TS_ASSERT(testing::Mock::VerifyAndClear(pDimA.get()));
    TS_ASSERT(testing::Mock::VerifyAndClear(pDimB.get()));
    TS_ASSERT(testing::Mock::VerifyAndClear(pDimC.get()));
  }

  void testWithXDimensionOnly() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    std::string a{"a"};
    EXPECT_CALL(*pDimensionX, toXMLString())
        .WillOnce(Return(createDimensionXMLString(1, -1, 1, "A", a)));
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = builder.create(); // Serialize the geometry.

    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    // Check that the number of dimensions provided is correct.
    Poco::AutoPtr<Poco::XML::NodeList> dimension =
        pRootElem->getElementsByTagName("Dimension");
    TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 1,
                      dimension->length());

    // Check that mapping nodes give correct mappings.
    TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a",
                      pRootElem->getChildElement("XDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "",
                      pRootElem->getChildElement("YDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "",
                      pRootElem->getChildElement("ZDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "",
                      pRootElem->getChildElement("TDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
  }

  void testWithXYDimensionOnly() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    auto pDimensionY = boost::make_shared<MockIMDDimension>();
    std::string a{"a"}, b{"b"};

    EXPECT_CALL(*pDimensionX, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "A", a)));
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));

    EXPECT_CALL(*pDimensionY, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "B", b)));
    EXPECT_CALL(Const(*pDimensionY), getDimensionId())
        .WillRepeatedly(ReturnRef(b));

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
    builder.addYDimension(IMDDimension_const_sptr(pDimensionY));

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = builder.create(); // Serialize the geometry.

    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    // Check that the number of dimensions provided is correct.
    Poco::AutoPtr<Poco::XML::NodeList> dimension =
        pRootElem->getElementsByTagName("Dimension");
    TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 2,
                      dimension->length());

    // Check that mapping nodes give correct mappings.
    TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a",
                      pRootElem->getChildElement("XDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "b",
                      pRootElem->getChildElement("YDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "",
                      pRootElem->getChildElement("ZDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "",
                      pRootElem->getChildElement("TDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
  }

  void testWithXYZDimensionOnly() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    auto pDimensionY = boost::make_shared<MockIMDDimension>();
    auto pDimensionZ = boost::make_shared<MockIMDDimension>();
    std::string a{"a"}, b{"b"}, c{"c"};

    EXPECT_CALL(*pDimensionX, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "A", a)));
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));

    EXPECT_CALL(*pDimensionY, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "B", b)));
    EXPECT_CALL(Const(*pDimensionY), getDimensionId())
        .WillRepeatedly(ReturnRef(b));

    EXPECT_CALL(*pDimensionZ, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "C", c)));
    EXPECT_CALL(Const(*pDimensionZ), getDimensionId())
        .WillRepeatedly(ReturnRef(c));

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    builder.addXDimension(IMDDimension_const_sptr(pDimensionX));
    builder.addYDimension(IMDDimension_const_sptr(pDimensionY));
    builder.addZDimension(IMDDimension_const_sptr(pDimensionZ));

    Poco::XML::DOMParser pParser;
    std::string xmlToParse = builder.create(); // Serialize the geometry.

    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    // Check that the number of dimensions provided is correct.
    Poco::AutoPtr<Poco::XML::NodeList> dimension =
        pRootElem->getElementsByTagName("Dimension");
    TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 3,
                      dimension->length());

    // Check that mapping nodes give correct mappings.
    TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a",
                      pRootElem->getChildElement("XDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionY mapping", "b",
                      pRootElem->getChildElement("YDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionZ mapping", "c",
                      pRootElem->getChildElement("ZDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("Should have no DimensionT mapping", "",
                      pRootElem->getChildElement("TDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
  }

  void testFullCreate() {
    auto pDimensionX = boost::make_shared<MockIMDDimension>();
    auto pDimensionY = boost::make_shared<MockIMDDimension>();
    auto pDimensionZ = boost::make_shared<MockIMDDimension>();
    auto pDimensionT = boost::make_shared<MockIMDDimension>();
    std::string a{"a"}, b{"b"}, c{"c"}, d{"d"};

    EXPECT_CALL(*pDimensionX, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "A", a)));
    EXPECT_CALL(Const(*pDimensionX), getDimensionId())
        .WillRepeatedly(ReturnRef(a));

    EXPECT_CALL(*pDimensionY, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "B", b)));
    EXPECT_CALL(Const(*pDimensionY), getDimensionId())
        .WillRepeatedly(ReturnRef(b));

    EXPECT_CALL(*pDimensionZ, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "C", c)));
    EXPECT_CALL(Const(*pDimensionZ), getDimensionId())
        .WillRepeatedly(ReturnRef(c));

    EXPECT_CALL(*pDimensionT, toXMLString())
        .WillRepeatedly(Return(createDimensionXMLString(1, -1, 1, "D", d)));
    EXPECT_CALL(Const(*pDimensionT), getDimensionId())
        .WillRepeatedly(ReturnRef(d));

    MDGeometryBuilderXML<NoDimensionPolicy> builder;
    builder.addXDimension(pDimensionX);
    builder.addYDimension(pDimensionY);
    builder.addZDimension(pDimensionZ);
    builder.addTDimension(pDimensionT);

    // Only practicle way to check the xml output in the absense of xsd is as
    // part of a dom tree.
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = builder.create(); // Serialize the geometry.

    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element *pRootElem = pDoc->documentElement();

    // Check that the number of dimensions provided is correct.
    Poco::AutoPtr<Poco::XML::NodeList> dimension =
        pRootElem->getElementsByTagName("Dimension");
    TSM_ASSERT_EQUALS("Wrong number of dimensions in geometry xml", 4,
                      dimension->length());

    // Check that mapping nodes have been provided.
    Poco::AutoPtr<Poco::XML::NodeList> xdimension =
        pRootElem->getElementsByTagName("XDimension");
    TSM_ASSERT_EQUALS("No DimensionX in geometry xml", 1, xdimension->length());
    Poco::AutoPtr<Poco::XML::NodeList> ydimension =
        pRootElem->getElementsByTagName("YDimension");
    TSM_ASSERT_EQUALS("No DimensionY in geometry xml", 1, ydimension->length());
    Poco::AutoPtr<Poco::XML::NodeList> zdimension =
        pRootElem->getElementsByTagName("ZDimension");
    TSM_ASSERT_EQUALS("No DimensionZ in geometry xml", 1, zdimension->length());
    Poco::AutoPtr<Poco::XML::NodeList> tdimension =
        pRootElem->getElementsByTagName("TDimension");
    TSM_ASSERT_EQUALS("No DimensionT in geometry xml", 1, tdimension->length());

    // Check that mapping nodes give correct mappings.
    TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "a",
                      pRootElem->getChildElement("XDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("No DimensionY mapping is incorrect", "b",
                      pRootElem->getChildElement("YDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("No DimensionZ mapping is incorrect", "c",
                      pRootElem->getChildElement("ZDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
    TSM_ASSERT_EQUALS("No DimensionT mapping is incorrect", "d",
                      pRootElem->getChildElement("TDimension")
                          ->getChildElement("RefDimensionId")
                          ->innerText());
  }
};

#endif /* MD_GEOMETRYXMLBUILDER_TEST_H_ */
