#include "MantidVatesAPI/Common.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

#include <vtkNew.h>
#include <vtkFieldData.h>
#include <vtkStringArray.h>

//using namespace Mantid::Geometry;
namespace Mantid
{
namespace VATES
{
std::string makeAxisTitle(Dimension_const_sptr dim)
{
  std::string title = dim->getName();
  title += " (";
  title += dim->getUnits();
  title += ")";
  return title;
}

void setAxisLabel(std::string metadataLabel,
                  std::string labelString,
                  vtkFieldData *fieldData)
{
  vtkNew<vtkStringArray> axisTitle;
  axisTitle->SetName(metadataLabel.c_str());
  axisTitle->SetNumberOfComponents(1);
  axisTitle->SetNumberOfTuples(1);
  axisTitle->SetValue(0, labelString.c_str());
  fieldData->AddArray(axisTitle.GetPointer());
}


} // VATES
} // Mantid
