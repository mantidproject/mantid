
#ifndef VTKDATASET_TO_GEOMETRY_TEST_H_
#define VTKDATASET_TO_GEOMETRY_TEST_H_

#include "MantidVatesAPI/vtkDataSetToGeometry.h"

#include <cxxtest/TestSuite.h>

#include "vtkFieldData.h"
#include "vtkCharArray.h"
#include "vtkRectilinearGrid.h"

#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"

class vtkDataSetToGeometryTest : public CxxTest::TestSuite
{

private:

// Helper method. Creates xml required as input for geometry. Allows mappings to be specified via function parameters.
 static std::string constructXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping)
{
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
"<MDInstruction>" +
  "<DimensionSet>" +
    "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
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
  "</DimensionSet>" +
"</MDInstruction>";
  }


static vtkFieldData* createFieldDataWithCharArray(std::string testData, std::string id)
{
  vtkFieldData* fieldData = vtkFieldData::New();
  vtkCharArray* charArray = vtkCharArray::New();
  charArray->SetName(id.c_str());
  charArray->Allocate(100);
  for(unsigned int i = 0; i < testData.size(); i++)
  {
    char cNextVal = testData.at(i);
    if(int(cNextVal) > 1)
    {
      charArray->InsertNextValue(cNextVal);

    }
  }
  fieldData->AddArray(charArray);
  charArray->Delete();
  return fieldData;
}

public:

  void testNoDimensionMappings()
  {
    using namespace Mantid::VATES;
    vtkRectilinearGrid* data = vtkRectilinearGrid::New();
    data->SetFieldData(createFieldDataWithCharArray(constructXML("", "", "", ""), Mantid::VATES::XMLDefinitions::metaDataId())); // No mappings
   
    vtkDataSetToGeometry xmlParser(data);
    xmlParser.execute();
    
    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", xmlParser.getYDimension().get() == NULL);
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", xmlParser.getYDimension().get() == NULL);
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", xmlParser.getZDimension().get() == NULL);
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", xmlParser.getTDimension().get() == NULL);
    data->Delete();
  }

  void testGetXDimension()
  {
    using namespace Mantid::VATES;
    vtkRectilinearGrid* data = vtkRectilinearGrid::New();
    data->SetFieldData(createFieldDataWithCharArray(constructXML("en", "", "", ""), Mantid::VATES::XMLDefinitions::metaDataId())); // Only x
   
    vtkDataSetToGeometry xmlParser(data);
    xmlParser.execute();
    
    TSM_ASSERT("X dimension should have been extracted via its mappings", xmlParser.getXDimension().get() != NULL);
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", xmlParser.getYDimension().get() == NULL);
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", xmlParser.getZDimension().get() == NULL);
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", xmlParser.getTDimension().get() == NULL);
    data->Delete();
  }

  void testGetYDimension()
  {
    using namespace Mantid::VATES;
    vtkRectilinearGrid* data = vtkRectilinearGrid::New();
    data->SetFieldData(createFieldDataWithCharArray(constructXML("", "en", "", ""), Mantid::VATES::XMLDefinitions::metaDataId())); // Only y
   
    vtkDataSetToGeometry xmlParser(data);
    xmlParser.execute();
   
    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", xmlParser.getXDimension().get() == NULL);
    TSM_ASSERT("Y dimension should have been extracted via its mappings", xmlParser.getYDimension().get() != NULL);
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", xmlParser.getZDimension().get() == NULL);
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", xmlParser.getTDimension().get() == NULL);
    data->Delete();
  }

  void testGetZDimension()
  {
    using namespace Mantid::VATES;
    vtkRectilinearGrid* data = vtkRectilinearGrid::New();
    data->SetFieldData(createFieldDataWithCharArray(constructXML("", "", "en", ""), Mantid::VATES::XMLDefinitions::metaDataId())); // Only z
   
    vtkDataSetToGeometry xmlParser(data);
    xmlParser.execute();
   
    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", xmlParser.getXDimension().get() == NULL);
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", xmlParser.getYDimension().get() == NULL);
    TSM_ASSERT("Z dimension should have been extracted via its mappings", xmlParser.getZDimension().get() != NULL);
    TSM_ASSERT("T dimension mappings are absent. No dimension should have been set.", xmlParser.getTDimension().get() == NULL);
    data->Delete();
  }

  void testGetTDimension()
  {
    using namespace Mantid::VATES;
    vtkRectilinearGrid* data = vtkRectilinearGrid::New();
    data->SetFieldData(createFieldDataWithCharArray(constructXML("", "", "", "en"), Mantid::VATES::XMLDefinitions::metaDataId())); // Only t
   
    vtkDataSetToGeometry xmlParser(data);
    xmlParser.execute();
   
    TSM_ASSERT("X dimension mappings are absent. No dimension should have been set.", xmlParser.getXDimension().get() == NULL);
    TSM_ASSERT("Y dimension mappings are absent. No dimension should have been set.", xmlParser.getYDimension().get() == NULL);
    TSM_ASSERT("Z dimension mappings are absent. No dimension should have been set.", xmlParser.getZDimension().get() == NULL);
    TSM_ASSERT("T dimension should have been extracted via its mappings", xmlParser.getTDimension().get() != NULL);
    data->Delete();
  }

  void testAllDimensions()
  {
    using namespace Mantid::VATES;
    vtkRectilinearGrid* data = vtkRectilinearGrid::New();
    data->SetFieldData(createFieldDataWithCharArray(constructXML("qy", "qx", "en", "qz"), Mantid::VATES::XMLDefinitions::metaDataId())); // All configured.
   
    vtkDataSetToGeometry xmlParser(data);
    xmlParser.execute();
   
    TSM_ASSERT("X dimension should have been extracted via its mappings", xmlParser.getXDimension().get() != NULL);
    TSM_ASSERT("Y dimension should have been extracted via its mappings", xmlParser.getYDimension().get() != NULL);
    TSM_ASSERT("Z dimension should have been extracted via its mappings", xmlParser.getZDimension().get() != NULL);
    TSM_ASSERT("T dimension should have been extracted via its mappings", xmlParser.getTDimension().get() != NULL);

    TSM_ASSERT_EQUALS("Wrong mapping for XDimension", "qy", xmlParser.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for YDimension", "qx", xmlParser.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for ZDimension", "en", xmlParser.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for TDimension", "qz", xmlParser.getTDimension()->getDimensionId());

    data->Delete();
  }



};

#endif
