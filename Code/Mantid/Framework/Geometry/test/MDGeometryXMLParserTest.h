
#ifndef MDGEOMETRY_XML_PARSER_TEST_H_
#define MDGEOMETRY_XML_PARSER_TEST_H_

#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include <cxxtest/TestSuite.h>


class MDGeometryXMLParserTest : public CxxTest::TestSuite
{

public:

   static std::string constructXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping)
{
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
  "<DimensionSet>" +
    "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<UpperBounds>5</UpperBounds>" +
      "<LowerBounds>-1.5</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"other\">" +
      "<Name>Other</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
    "</Dimension>" +
    "<XDimension>" +
      "<RefDimensionId>" +
      xDimensionIdMapping +
      "</RefDimensionId>" +
    "</XDimension>" +
    "<YDimension>" +
      "<RefDimensionId>" +
      yDimensionIdMapping +
      "</RefDimensionId>" +
    "</YDimension>" +
    "<ZDimension>" +
      "<RefDimensionId>" + 
      zDimensionIdMapping +
      "</RefDimensionId>" +
    "</ZDimension>" +
    "<TDimension>" +
      "<RefDimensionId>" +
      tDimensionIdMapping +
      "</RefDimensionId>" +
    "</TDimension>" +
  "</DimensionSet>";
  }


public:

   void testNoDimensionMappings()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("", "", "", "")); // No mappings
    xmlParser.execute();
    
    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", !xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", !xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", !xmlParser.hasZDimension());
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 5, xmlParser.getNonMappedDimensions().size());
  }

  void testGetXDimension()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("en", "", "", "")); // Only x
    xmlParser.execute();
    
    TSM_ASSERT("X dimension should have been extracted via its mappings", xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", !xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", !xmlParser.hasZDimension());
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4, xmlParser.getNonMappedDimensions().size());
  }

  void testGetYDimension()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("", "en", "", "")); // Only y
    xmlParser.execute();

    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", !xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension should have been extracted via its mappings", xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", !xmlParser.hasZDimension());
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4, xmlParser.getNonMappedDimensions().size());
  }

  void testGetZDimension()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("", "", "en", "")); // Only z
    xmlParser.execute();

    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", !xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", !xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension should have been extracted via its mappings", xmlParser.hasZDimension());
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4, xmlParser.getNonMappedDimensions().size());
  }

  void testGetTDimension()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("", "", "", "en")); // Only t
    xmlParser.execute();
   
    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", !xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", !xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", !xmlParser.hasZDimension());
    TSM_ASSERT("T dimension should have been extracted via its mappings", xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4, xmlParser.getNonMappedDimensions().size());
  }

  void testAllDimensions()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz")); // All mapped
    xmlParser.execute();
   
    TSM_ASSERT("X dimension should have been extracted via its mappings", xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension should have been extracted via its mappings", xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension should have been extracted via its mappings", xmlParser.hasZDimension());
    TSM_ASSERT("T dimension should have been extracted via its mappings", xmlParser.hasTDimension());

    TSM_ASSERT_EQUALS("Wrong mapping for XDimension", "qy", xmlParser.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for YDimension", "qx", xmlParser.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for ZDimension", "en", xmlParser.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for TDimension", "qz", xmlParser.getTDimension()->getDimensionId());

    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 1, xmlParser.getNonMappedDimensions().size());
    TSM_ASSERT_EQUALS("Wrong non-mapped dimension found", "other" ,xmlParser.getNonMappedDimensions()[0]->getDimensionId());
  }
  
  
  void testGetNonMappedDimensionsThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getNonMappedDimensions(), std::runtime_error);
  }

  void testGetXDimensionThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getXDimension(), std::runtime_error);
  }

   void testGetYDimensionThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getYDimension(), std::runtime_error);
  }

  void testGetZDimensionThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getZDimension(), std::runtime_error);
  }

   void testGetTDimensionThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getTDimension(), std::runtime_error);
  }

  void testGetAllDimensionsThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getAllDimensions(), std::runtime_error);
  }

  void testGetNonIntegratedDimensionsThrows()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    TSM_ASSERT_THROWS("execute not called before using getter. Should have thrown", xmlParser.getNonIntegratedDimensions(), std::runtime_error);
  }

  void testGetAllDimensions()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    xmlParser.execute();
    TSM_ASSERT_EQUALS("Returned wrong number of dimensions", 5, xmlParser.getAllDimensions().size());
  }

  void testGetAllNonIntegratedDimensions()
  {
    using namespace Mantid::Geometry;
    //2 of the 5 dimensions have been setup to be integrated => nbins==1.
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    xmlParser.execute();
    TSM_ASSERT_EQUALS("Returned wrong number of non integrated dimensions", 3, xmlParser.getNonIntegratedDimensions().size());
  }

  void testGetAllIntegratedDimensions()
  {
    using namespace Mantid::Geometry;
    //2 of the 5 dimensions have been setup to be integrated => nbins==1.
    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    xmlParser.execute();
    TSM_ASSERT_EQUALS("Returned wrong number of integrated dimensions", 2, xmlParser.getIntegratedDimensions().size());
  }

  void testAllMappedDimensions()
  {
    using namespace Mantid::Geometry;

    MDGeometryXMLParser xmlParser(constructXML("qy", "qx", "en", "qz"));
    xmlParser.execute();
    
    TSM_ASSERT("X dimension should have been extracted via its mappings", xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension should have been extracted via its mappings", xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension should have been extracted via its mappings", xmlParser.hasZDimension());
    TSM_ASSERT("T dimension should have been extracted via its mappings", xmlParser.hasTDimension());

    TSM_ASSERT_EQUALS("Wrong mapping for XDimension", "qy", xmlParser.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for YDimension", "qx", xmlParser.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for ZDimension", "en", xmlParser.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for TDimension", "qz", xmlParser.getTDimension()->getDimensionId());
  }

  void testAssignment()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser A(constructXML("qy", "qx", "en", "qz"));
    MDGeometryXMLParser B(constructXML("", "", "", ""));
    A.execute();

    B = A;
    TSM_ASSERT_EQUALS("X dimension output not the same after assignment", A.hasXDimension(), B.hasXDimension());
    TSM_ASSERT_EQUALS("X dimension output not the same after assignment", A.getXDimension()->getDimensionId(), B.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment", A.hasYDimension(), B.hasYDimension());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment", A.getYDimension()->getDimensionId(), B.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment", A.hasZDimension(), B.hasZDimension());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment", A.getZDimension()->getDimensionId(), B.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment", A.hasTDimension(), B.hasTDimension());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment", A.getTDimension()->getDimensionId(), B.getTDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Non mapped dimension output not the same after assignment", A.getNonMappedDimensions().size(), B.getNonMappedDimensions().size());

  }

  void testCopy()
  {
    using namespace Mantid::Geometry;
    MDGeometryXMLParser A(constructXML("qy", "qx", "en", "qz"));
    A.execute();
    MDGeometryXMLParser B = A;

    TSM_ASSERT_EQUALS("X dimension output not the same after assignment", A.hasXDimension(), B.hasXDimension());
    TSM_ASSERT_EQUALS("X dimension output not the same after assignment", A.getXDimension()->getDimensionId(), B.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment", A.hasYDimension(), B.hasYDimension());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment", A.getYDimension()->getDimensionId(), B.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment", A.hasZDimension(), B.hasZDimension());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment", A.getZDimension()->getDimensionId(), B.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment", A.hasTDimension(), B.hasTDimension());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment", A.getTDimension()->getDimensionId(), B.getTDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Non mapped dimension output not the same after assignment", A.getNonMappedDimensions().size(), B.getNonMappedDimensions().size());

  }

  void testThrowsIfRootInvalid()
  {
    using namespace Mantid::Geometry;

    MDGeometryXMLParser xmlParser("<ElementTypeA><ElementTypeB></ElementTypeB></ElementTypeA>"); //Valid xml, but the wrong schema.
    xmlParser.SetRootNodeCheck("SomeOtherSchemaElement"); //This won't match so should throw!
    TS_ASSERT_THROWS(xmlParser.execute(), std::runtime_error);
  }

};

#endif