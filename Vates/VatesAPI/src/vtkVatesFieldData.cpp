#include "MantidVatesAPI/vtkVatesFieldData.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkVATESFieldData);

void vtkVATESFieldData::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Arrays: " << this->GetNumberOfArrays() << "\n";
  for (int i=0; i<this->GetNumberOfArrays(); i++)
    {
    if (this->GetArrayName(i))
      {
      auto name = this->GetArrayName(i);
      Mantid::VATES::FieldDataToMetadata convert;
      auto value = convert(this, name);
      os << indent << "Array " << i << " name = "
         << name << " value = " << value << "\n";
      }
    else
      {
      os << indent << "Array " << i << " name = NULL value = NULL\n";
      }
    }
  os << indent << "Number Of Components: " << this->GetNumberOfComponents()
     << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
}
