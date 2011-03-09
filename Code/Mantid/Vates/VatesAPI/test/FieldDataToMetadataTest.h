#ifndef FIELDDATATOMETADATATEST_H_
#define FIELDDATATOMETADATATEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include "MantidVatesAPI/FieldDataToMetadata.h"

using Mantid::VATES::FieldDataToMetadata;

class FieldDataToMetadataTest : public CxxTest::TestSuite
{

private:

  //Helper method
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

  void testExecute()
  {
    const std::string id = "1";
    const std::string testData = "abc";
    vtkFieldData* fieldData = createFieldDataWithCharArray(testData, id);

    FieldDataToMetadata function;
    std::string metadata = function.execute(fieldData, id);

    TSM_ASSERT_EQUALS("The Function failed to properly convert field data to metadata", testData, metadata);
    fieldData->Delete();
  }

  void testOperatorOverload()
  {
    const std::string id = "1";
    const std::string testData = "abc";
    vtkFieldData* fieldData = createFieldDataWithCharArray(testData, id);

    typedef std::binary_function<vtkFieldData*, std::string, std::string> BaseType;
    FieldDataToMetadata function;
    TSM_ASSERT_EQUALS("Results from two equivalent methods differ.", function(fieldData, id), function.execute(fieldData, id));
    fieldData->Delete();
  }

  void testThrowsIfNotFound()
  {
    const std::string id = "1";
    const std::string testData = "abc";
    vtkFieldData* fieldData = createFieldDataWithCharArray(testData, id);

    FieldDataToMetadata function;
    TSM_ASSERT_THROWS("Unknown id requested. Should have thrown.", function.execute(fieldData, "x"), std::runtime_error );
    fieldData->Delete();
  }


};


#endif
