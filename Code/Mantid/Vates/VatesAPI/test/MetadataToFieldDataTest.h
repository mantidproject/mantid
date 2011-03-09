#ifndef METADATATOFIELDDATATEST_H_
#define METADATATOFIELDDATATEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "boost/algorithm/string.hpp"

using Mantid::VATES::MetadataToFieldData;

class MetadataToFieldDataTest: public CxxTest::TestSuite
{
private:

  //helper method
  static std::string convertCharArrayToString(vtkCharArray* carry)
  {
    std::string sResult;
    for (int i = 0; i < carry->GetSize(); i++)
    {
      char c = carry->GetValue(i);
      if (int(c) > 1)
      {
        sResult.push_back(c);
      }
    }
    boost::trim(sResult);
    return sResult;
  }

public:

  void testMetaDataToFieldData()
  {
    std::string testData = "<test data/>%s";
    const std::string id = "1";

    vtkFieldData* fieldData = vtkFieldData::New();
    vtkCharArray* charArray = vtkCharArray::New();
    charArray->SetName(id.c_str());
    fieldData->AddArray(charArray);

    MetadataToFieldData function;
    function(fieldData, testData, id);

    //convert vtkchararray back into a string.
    vtkCharArray* carry = dynamic_cast<vtkCharArray*> (fieldData->GetArray(id.c_str()));

    TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
    charArray->Delete();
    fieldData->Delete();
  }

  void testMetaDataToFieldDataWithEmptyFieldData()
  {
    std::string testData = "<test data/>%s";
    const std::string id = "1";

    vtkFieldData* emptyFieldData = vtkFieldData::New();
    MetadataToFieldData function;
    function(emptyFieldData, testData.c_str(), id.c_str());

    //convert vtkchararray back into a string.
    vtkCharArray* carry = dynamic_cast<vtkCharArray*> (emptyFieldData->GetArray(id.c_str()));

    TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
    emptyFieldData->Delete();
  }

};

#endif
