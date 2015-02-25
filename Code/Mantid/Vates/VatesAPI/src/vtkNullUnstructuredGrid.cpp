#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"

#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
namespace Mantid {
namespace VATES {

/// Constructor
vtkNullUnstructuredGrid::vtkNullUnstructuredGrid() {}

/// Destructor
vtkNullUnstructuredGrid::~vtkNullUnstructuredGrid() {}

/**
 * Creates a default vtkDataSet.
 *@returns A pointer to the default vtkDataSet
 */
vtkUnstructuredGrid *vtkNullUnstructuredGrid::createNullData() {

  vtkUnstructuredGrid *dataSet = vtkUnstructuredGrid::New();
  dataSet->Allocate(1);

  vtkPoints *points = vtkPoints::New();
  points->Allocate(1);
  points->SetNumberOfPoints(1);
  points->SetPoint(0, 0, 0, 0);

  vtkIdList *pointList = vtkIdList::New();
  pointList->SetNumberOfIds(1);

  dataSet->SetPoints(points);

  return dataSet;
}
}
}