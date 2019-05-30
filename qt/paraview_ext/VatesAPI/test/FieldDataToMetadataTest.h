// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FIELDDATATOMETADATATEST_H_
#define FIELDDATATOMETADATATEST_H_

#include "MantidVatesAPI/FieldDataToMetadata.h"
#include <cxxtest/TestSuite.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkSmartPointer.h>

using Mantid::VATES::FieldDataToMetadata;

class FieldDataToMetadataTest : public CxxTest::TestSuite {

private:
  // Helper method
  static vtkFieldData *createFieldDataWithCharArray(std::string testData,
                                                    std::string id) {
    vtkFieldData *fieldData = vtkFieldData::New();
    auto charArray = vtkSmartPointer<vtkCharArray>::New();
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
  void testExecute() {
    const std::string id = "1";
    const std::string testData = "abc";
    vtkSmartPointer<vtkFieldData> fieldData =
        createFieldDataWithCharArray(testData, id);

    FieldDataToMetadata function;
    std::string metadata = function.execute(fieldData, id);

    TSM_ASSERT_EQUALS(
        "The Function failed to properly convert field data to metadata",
        testData, metadata);
  }

  void testOperatorOverload() {
    const std::string id = "1";
    const std::string testData = "abc";
    vtkSmartPointer<vtkFieldData> fieldData =
        createFieldDataWithCharArray(testData, id);

    FieldDataToMetadata function;
    TSM_ASSERT_EQUALS("Results from two equivalent methods differ.",
                      function(fieldData, id), function.execute(fieldData, id));
  }

  void testThrowsIfNotFound() {
    const std::string id = "1";
    const std::string testData = "abc";
    vtkSmartPointer<vtkFieldData> fieldData =
        createFieldDataWithCharArray(testData, id);

    FieldDataToMetadata function;
    TSM_ASSERT_THROWS("Unknown id requested. Should have thrown.",
                      function.execute(fieldData, "x"),
                      const std::runtime_error &);
  }

  void testThrowsIfNullFieldData() {
    vtkFieldData *nullFieldData = nullptr;
    FieldDataToMetadata function;
    TSM_ASSERT_THROWS("Should not be able to execute with null field data.",
                      function.execute(nullFieldData, "x"),
                      const std::runtime_error &);
  }
};

#endif
