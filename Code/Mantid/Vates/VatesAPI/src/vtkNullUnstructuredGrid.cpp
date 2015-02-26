#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"

#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkVertex.h>
#include <vtkSmartPointer.h>
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

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkVertex> vertex = vtkSmartPointer<vtkVertex>::New();

  Mantid::coord_t p[3] = {0.0, 0.0, 0.0};
  points->InsertPoint(0, p);
  vertex->GetPointIds()->SetId(0, 0);

  dataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());
  dataSet->SetPoints(points);
  dataSet->Squeeze();

  return dataSet;
}
}
}