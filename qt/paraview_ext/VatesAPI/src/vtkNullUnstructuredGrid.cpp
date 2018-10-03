// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"

#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertex.h>
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

  double p[3];
  p[0] = 0.0;
  p[1] = 0.0;
  p[2] = 0.0;

  points->InsertPoint(0, p);

  vertex->GetPointIds()->SetId(0, 0);

  dataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());
  dataSet->SetPoints(points);
  dataSet->Squeeze();

  return dataSet;
}
} // namespace VATES
} // namespace Mantid