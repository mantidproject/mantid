#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "vtkCharArray.h"
#include "vtkFieldData.h"
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace VATES {

std::string FieldDataToMetadata::operator()(vtkFieldData *fieldData,
                                            const std::string &id) const {
  return execute(fieldData, id);
}

std::string FieldDataToMetadata::execute(vtkFieldData *fieldData,
                                         const std::string &id) const {
  std::string sXml;
  if (!fieldData) {
    throw std::runtime_error("vtkFieldData argument is null");
  }
  vtkDataArray *array = fieldData->GetArray(id.c_str());
  if (!array) {
    throw std::runtime_error("The specified vtk array does not exist");
  }
  if (vtkCharArray *carray = vtkCharArray::FastDownCast(array)) {
    carray->Squeeze();
    for (int i = 0; i < carray->GetSize(); i++) {
      char c = carray->GetValue(i);
      if (int(c) > 1) {
        sXml.push_back(c);
      }
    }
    boost::trim(sXml);
  }
  return sXml;
}
} // namespace VATES
} // namespace Mantid
