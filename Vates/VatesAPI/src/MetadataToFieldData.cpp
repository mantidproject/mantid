#include "MantidVatesAPI/MetadataToFieldData.h"
#include "vtkCharArray.h"
#include "vtkFieldData.h"
#include "vtkNew.h"

namespace Mantid
{
namespace VATES
{

void MetadataToFieldData::operator()(vtkFieldData* fieldData, std::string metaData, std::string id) const
{
  execute(fieldData, metaData, id);
}

void MetadataToFieldData::execute(vtkFieldData* fieldData, std::string metaData, std::string id) const
{
  //clean out existing.
  vtkDataArray* arry = fieldData->GetArray(id.c_str());
  if(NULL != arry)
  {
    fieldData->RemoveArray(id.c_str());
  }
  //create new.
  vtkNew<vtkCharArray> newArry;
  newArry->Allocate(metaData.size());
  newArry->SetName(id.c_str());
  fieldData->AddArray(newArry.GetPointer());

  for(unsigned int i = 0 ; i < metaData.size(); i++)
  {
    newArry->InsertNextValue(metaData.at(i));
  }
}

}
}
