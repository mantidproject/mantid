#include "MantidVatesAPI/MetadataToFieldData.h"
#include "vtkCharArray.h"
#include "vtkFieldData.h"
#include "vtkNew.h"

namespace Mantid {
namespace VATES {

void MetadataToFieldData::operator()(vtkFieldData *fieldData,
                                     const std::string &metaData,
                                     const std::string &id) const {
  execute(fieldData, metaData, id);
}

void MetadataToFieldData::execute(vtkFieldData *fieldData,
                                  const std::string &metaData,
                                  const std::string &id) const {
  // clean out existing.
  vtkDataArray *array = fieldData->GetArray(id.c_str());
  if (array) {
    fieldData->RemoveArray(id.c_str());
  }
  // create new.
  vtkNew<vtkCharArray> newArray;
  newArray->SetNumberOfTuples(metaData.size());
  newArray->SetName(id.c_str());
  fieldData->AddArray(newArray.GetPointer());
  for (size_t i = 0; i < metaData.size(); i++) {
    newArray->SetValue(i, metaData[i]);
  }
}
}
}
