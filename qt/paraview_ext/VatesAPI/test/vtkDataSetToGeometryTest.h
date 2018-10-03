// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef VTKDATASET_TO_GEOMETRY_TEST_H_
#define VTKDATASET_TO_GEOMETRY_TEST_H_

#include "MantidVatesAPI/vtkDataSetToGeometry.h"

#include <cxxtest/TestSuite.h>

#include "MantidVatesAPI/vtkRectilinearGrid_Silent.h"
#include "vtkCharArray.h"
#include "vtkFieldData.h"
#include "vtkNew.h"

#include "MantidVatesAPI/VatesXMLDefinitions.h"

class vtkDataSetToGeometryTest : public CxxTest::TestSuite {

private:
  // Helper method. Creates xml required as input for geometry. Allows mappings
  // to be specified via function parameters.
  static std::string constructXML(const std::string &xDimensionIdMapping,
                                  const std::string &yDimensionIdMapping,
                                  const std::string &zDimensionIdMapping,
                                  const std::string &tDimensionIdMapping) {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
           "<MDInstruction>" + "<DimensionSet>" + "<Dimension ID=\"en\">" +
           "<Name>Energy</Name>" + "<UpperBounds>150</UpperBounds>" +
           "<LowerBounds>0</LowerBounds>" + "<NumberOfBins>5</NumberOfBins>" +
           "</Dimension>" + "<Dimension ID=\"qx\">" + "<Name>Qx</Name>" +
           "<UpperBounds>5</UpperBounds>" + "<LowerBounds>-1.5</LowerBounds>" +
           "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" +
           "<Dimension ID=\"qy\">" + "<Name>Qy</Name>" +
           "<UpperBounds>6.6</UpperBounds>" +
           "<LowerBounds>-6.6</LowerBounds>" +
           "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" +
           "<Dimension ID=\"qz\">" + "<Name>Qz</Name>" +
           "<UpperBounds>6.6</UpperBounds>" +
           "<LowerBounds>-6.6</LowerBounds>" +
           "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" +
           "<Dimension ID=\"other\">" + "<Name>Other</Name>" +
           "<UpperBounds>6.6</UpperBounds>" +
           "<LowerBounds>-6.6</LowerBounds>" +
           "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" + "<XDimension>" +
           "<RefDimensionId>" + xDimensionIdMapping + "</RefDimensionId>" +
           "</XDimension>" + "<YDimension>" + "<RefDimensionId>" +
           yDimensionIdMapping + "</RefDimensionId>" + "</YDimension>" +
           "<ZDimension>" + "<RefDimensionId>" + zDimensionIdMapping +
           "</RefDimensionId>" + "</ZDimension>" + "<TDimension>" +
           "<RefDimensionId>" + tDimensionIdMapping + "</RefDimensionId>" +
           "</TDimension>" + "</DimensionSet>" + "</MDInstruction>";
  }

  static vtkFieldData *createFieldDataWithCharArray(std::string testData,
                                                    std::string id) {
    vtkFieldData *fieldData = vtkFieldData::New();
    vtkNew<vtkCharArray> charArray;
    charArray->SetName(id.c_str());
    charArray->Allocate(100);
    for (unsigned int i = 0; i < testData.size(); i++) {
      char cNextVal = testData.at(i);
      if (int(cNextVal) > 1) {
        charArray->InsertNextValue(cNextVal);
      }
    }
    fieldData->AddArray(charArray.GetPointer());
    return fieldData;
  }

public:
  void testNoDimensionMappings() {
    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> data;
    data->SetFieldData(createFieldDataWithCharArray(
        constructXML("", "", "", ""),
        Mantid::VATES::XMLDefinitions::metaDataId())); // No mappings

    vtkDataSetToGeometry xmlParser(data.GetPointer());
    xmlParser.execute();

    TSM_ASSERT(
        "X dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasXDimension());
    TSM_ASSERT(
        "Y dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasYDimension());
    TSM_ASSERT(
        "Z dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasZDimension());
    TSM_ASSERT(
        "T dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 5,
                      xmlParser.getNonMappedDimensions().size());
  }

  void testGetXDimension() {
    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> data;
    data->SetFieldData(createFieldDataWithCharArray(
        constructXML("en", "", "", ""),
        Mantid::VATES::XMLDefinitions::metaDataId())); // Only x

    vtkDataSetToGeometry xmlParser(data.GetPointer());
    xmlParser.execute();

    TSM_ASSERT("X dimension should have been extracted via its mappings",
               xmlParser.hasXDimension());
    TSM_ASSERT(
        "Y dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasYDimension());
    TSM_ASSERT(
        "Z dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasZDimension());
    TSM_ASSERT(
        "T dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4,
                      xmlParser.getNonMappedDimensions().size());
  }

  void testGetYDimension() {
    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> data;
    data->SetFieldData(createFieldDataWithCharArray(
        constructXML("", "en", "", ""),
        Mantid::VATES::XMLDefinitions::metaDataId())); // Only y

    vtkDataSetToGeometry xmlParser(data.GetPointer());
    xmlParser.execute();

    TSM_ASSERT(
        "X dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension should have been extracted via its mappings",
               xmlParser.hasYDimension());
    TSM_ASSERT(
        "Z dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasZDimension());
    TSM_ASSERT(
        "T dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4,
                      xmlParser.getNonMappedDimensions().size());
  }

  void testGetZDimension() {
    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> data;
    data->SetFieldData(createFieldDataWithCharArray(
        constructXML("", "", "en", ""),
        Mantid::VATES::XMLDefinitions::metaDataId())); // Only z

    vtkDataSetToGeometry xmlParser(data.GetPointer());
    xmlParser.execute();

    TSM_ASSERT(
        "X dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasXDimension());
    TSM_ASSERT(
        "Y dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension should have been extracted via its mappings",
               xmlParser.hasZDimension());
    TSM_ASSERT(
        "T dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4,
                      xmlParser.getNonMappedDimensions().size());
  }

  void testGetTDimension() {
    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> data;
    data->SetFieldData(createFieldDataWithCharArray(
        constructXML("", "", "", "en"),
        Mantid::VATES::XMLDefinitions::metaDataId())); // Only t

    vtkDataSetToGeometry xmlParser(data.GetPointer());
    xmlParser.execute();

    TSM_ASSERT(
        "X dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasXDimension());
    TSM_ASSERT(
        "Y dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasYDimension());
    TSM_ASSERT(
        "Z dimension mappings are absent. No dimension should have been set.",
        !xmlParser.hasZDimension());
    TSM_ASSERT("T dimension should have been extracted via its mappings",
               xmlParser.hasTDimension());
    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 4,
                      xmlParser.getNonMappedDimensions().size());
  }

  void testAllDimensions() {
    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> data;
    data->SetFieldData(createFieldDataWithCharArray(
        constructXML("qy", "qx", "en", "qz"),
        Mantid::VATES::XMLDefinitions::metaDataId())); // All configured.

    vtkDataSetToGeometry xmlParser(data.GetPointer());
    xmlParser.execute();

    TSM_ASSERT("X dimension should have been extracted via its mappings",
               xmlParser.hasXDimension());
    TSM_ASSERT("Y dimension should have been extracted via its mappings",
               xmlParser.hasYDimension());
    TSM_ASSERT("Z dimension should have been extracted via its mappings",
               xmlParser.hasZDimension());
    TSM_ASSERT("T dimension should have been extracted via its mappings",
               xmlParser.hasTDimension());

    TSM_ASSERT_EQUALS("Wrong mapping for XDimension", "qy",
                      xmlParser.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for YDimension", "qx",
                      xmlParser.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for ZDimension", "en",
                      xmlParser.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Wrong mapping for TDimension", "qz",
                      xmlParser.getTDimension()->getDimensionId());

    TSM_ASSERT_EQUALS("Wrong number of non-mapped dimensions", 1,
                      xmlParser.getNonMappedDimensions().size());
    TSM_ASSERT_EQUALS("Wrong non-mapped dimension found", "other",
                      xmlParser.getNonMappedDimensions()[0]->getDimensionId());
  }

  void testAssignment() {
    using namespace Mantid::VATES;

    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> dataA;
    dataA->SetFieldData(createFieldDataWithCharArray(
        constructXML("qy", "qx", "en", "qz"),
        Mantid::VATES::XMLDefinitions::metaDataId()));

    vtkNew<vtkRectilinearGrid> dataB;
    dataB->SetFieldData(createFieldDataWithCharArray(
        constructXML("", "", "", ""),
        Mantid::VATES::XMLDefinitions::metaDataId()));

    vtkDataSetToGeometry A(dataA.GetPointer());
    vtkDataSetToGeometry B(dataB.GetPointer());
    B = A;
    A.execute();
    B.execute();

    TSM_ASSERT_EQUALS("X dimension output not the same after assignment",
                      A.hasXDimension(), B.hasXDimension());
    TSM_ASSERT_EQUALS("X dimension output not the same after assignment",
                      A.getXDimension()->getDimensionId(),
                      B.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment",
                      A.hasYDimension(), B.hasYDimension());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment",
                      A.getYDimension()->getDimensionId(),
                      B.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment",
                      A.hasZDimension(), B.hasZDimension());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment",
                      A.getZDimension()->getDimensionId(),
                      B.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment",
                      A.hasTDimension(), B.hasTDimension());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment",
                      A.getTDimension()->getDimensionId(),
                      B.getTDimension()->getDimensionId());
    TSM_ASSERT_EQUALS(
        "Non mapped dimension output not the same after assignment",
        A.getNonMappedDimensions().size(), B.getNonMappedDimensions().size());
  }

  void testCopy() {
    using namespace Mantid::VATES;

    using namespace Mantid::VATES;
    vtkNew<vtkRectilinearGrid> dataA;
    dataA->SetFieldData(createFieldDataWithCharArray(
        constructXML("qy", "qx", "en", "qz"),
        Mantid::VATES::XMLDefinitions::metaDataId()));

    vtkDataSetToGeometry A(dataA.GetPointer());
    vtkDataSetToGeometry B = A;
    A.execute();
    B.execute();

    TSM_ASSERT_EQUALS("X dimension output not the same after assignment",
                      A.hasXDimension(), B.hasXDimension());
    TSM_ASSERT_EQUALS("X dimension output not the same after assignment",
                      A.getXDimension()->getDimensionId(),
                      B.getXDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment",
                      A.hasYDimension(), B.hasYDimension());
    TSM_ASSERT_EQUALS("Y dimension output not the same after assignment",
                      A.getYDimension()->getDimensionId(),
                      B.getYDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment",
                      A.hasZDimension(), B.hasZDimension());
    TSM_ASSERT_EQUALS("Z dimension output not the same after assignment",
                      A.getZDimension()->getDimensionId(),
                      B.getZDimension()->getDimensionId());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment",
                      A.hasTDimension(), B.hasTDimension());
    TSM_ASSERT_EQUALS("T dimension output not the same after assignment",
                      A.getTDimension()->getDimensionId(),
                      B.getTDimension()->getDimensionId());
    TSM_ASSERT_EQUALS(
        "Non mapped dimension output not the same after assignment",
        A.getNonMappedDimensions().size(), B.getNonMappedDimensions().size());
  }
};

#endif
