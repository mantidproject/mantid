#include "MantidVisitPresenters/FieldDataToMetadata.h"
#include <boost/algorithm/string.hpp>
#include "vtkFieldData.h"
#include "vtkCharArray.h"

namespace Mantid
{
namespace VATES
{

std::string FieldDataToMetadata::operator()(vtkFieldData* fieldData, std::string id) const
{
  return execute(fieldData, id);
}

std::string FieldDataToMetadata::execute(vtkFieldData* fieldData, const std::string& id) const
{
  std::string sXml;
  vtkDataArray* arry =  fieldData->GetArray(id.c_str());
  if(arry == NULL)
  {
    throw std::runtime_error("The specified vtk array does not exist");
  }
  if (vtkCharArray* carry = dynamic_cast<vtkCharArray*> (arry))
  {
    carry->Squeeze();
    for (int i = 0; i < carry->GetSize(); i++)
    {
      char c = carry->GetValue(i);
      if (int(c) > 1)
      {
        sXml.push_back(c);
      }
    }
    boost::trim(sXml);
  }
  return sXml;
}

}
}
