#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"

#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include <stdexcept>

namespace Mantid
{
namespace VATES
{

void vtkDataSetToNonOrthogonalDataSet::exec(vtkDataSet *dataset)
{
  vtkDataSetToNonOrthogonalDataSet temp(dataset);
  temp.execute();
}

vtkDataSetToNonOrthogonalDataSet::vtkDataSetToNonOrthogonalDataSet(vtkDataSet *dataset) :
  m_dataSet(dataset)
{
  if (NULL == m_dataSet)
  {
    throw std::runtime_error("Cannot construct vtkDataSetToNonOrthogonalDataSet with null VTK dataset");
  }
}

/**
 * Class destructor
 */
vtkDataSetToNonOrthogonalDataSet::~vtkDataSetToNonOrthogonalDataSet()
{
}

void vtkDataSetToNonOrthogonalDataSet::execute()
{
  // Downcast to a vtkUnstructuredGrid
  vtkUnstructuredGrid *data = vtkUnstructuredGrid::SafeDownCast(m_dataSet);
  if (NULL == data)
  {
    throw std::runtime_error("VTK dataset does not inherit from vtkPointSet");
  }
  // Get the original points
  vtkPoints *points = data->GetPoints();
  double *point;
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(points->GetNumberOfPoints());
  for(int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    point = points->GetPoint(i);

    newPoints->InsertNextPoint(point);
  }
  data->SetPoints(newPoints);
  this->updateMetaData(data);
}

void vtkDataSetToNonOrthogonalDataSet::updateMetaData(vtkUnstructuredGrid *ugrid)
{
  vtkFieldData *fd = ugrid->GetFieldData();
}

} // namespace VATES
} // namespace Mantid
