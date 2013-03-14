#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"

#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
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
  double *inPoint;
  double outPoint[3];
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(points->GetNumberOfPoints());

  /*
  // Gd2, HKLE
  double convert[9] = {0.475446282, 0.158482094, 0.0,
                       0.274499039, -0.274499039, 0.0,
                       0.0, 0.0, 0.172980453};
  */
  /*
  // Gd, HKLE
  double convert[9] = {0.316964188, 0.158482094, 0.0,
                       0.0, 0.274499039, 0.0,
                       0.0, 0.0, 0.172980453};
  */

  // Gd, HEKL
  double convert[9] = {1.0, 0.0, 0.5,
                       0.0, 1.0, 0.0,
                       0.0, 0.0, 0.8660254};

  /*
  // Gd2, HEKL
  double convert[9] = {0.475446282, 0.0, 0.158482094,
                       0.0, 1.0, 0.0,
                       0.274499039, 0.0, -0.274499039};
  */

  for(int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    inPoint = points->GetPoint(i);
    vtkMatrix3x3::MultiplyPoint(convert, inPoint, outPoint);
    newPoints->InsertNextPoint(outPoint);
  }
  data->SetPoints(newPoints);
  this->updateMetaData(data);
}

void vtkDataSetToNonOrthogonalDataSet::updateMetaData(vtkUnstructuredGrid *ugrid)
{
  /*
  // Gd2, HKLE
  double baseX[3] = {0.8660254, 0.5, 0.0};
  double baseY[3] = {0.5, -0.8660254, 0.0};
  double baseZ[3] = {0.0, 0.0, 1.0};
  */
  /*
  // Gd, HKLE
  double baseX[3] = {1.0, 0.0, 0.0};
  double baseY[3] = {0.5, 0.8660254, 0.0};
  double baseZ[3] = {0.0, 0.0, 1.0};
  */

  // Gd, HEKL
  double baseX[3] = {1.0, 0.0, 0.0};
  double baseY[3] = {0.0, 1.0, 0.0};
  double baseZ[3] = {0.5, 0.0, 0.8660254};

  /*
  // Gd2, HEKL
  double baseX[3] = {0.8660254, 0.0, 0.5};
  double baseY[3] = {0.0, 1.0, 0.0};
  double baseZ[3] = {0.5, 0.0, -0.8660254};
  */

  vtkFieldData *fieldData = ugrid->GetFieldData();

  vtkNew<vtkFloatArray> uBase;
  uBase->SetNumberOfComponents(3);
  uBase->SetNumberOfTuples(1);
  uBase->SetName("AxisBaseForX");
  uBase->SetTuple(0, baseX);
  fieldData->AddArray(uBase.GetPointer());

  vtkNew<vtkFloatArray> vBase;
  vBase->SetNumberOfComponents(3);
  vBase->SetNumberOfTuples(1);
  vBase->SetName("AxisBaseForY");
  vBase->SetTuple(0, baseY);
  fieldData->AddArray(vBase.GetPointer());

  vtkNew<vtkFloatArray> wBase;
  wBase->SetNumberOfComponents(3);
  wBase->SetNumberOfTuples(1);
  wBase->SetName("AxisBaseForZ");
  wBase->SetTuple(0, baseZ);
  fieldData->AddArray(wBase.GetPointer());
}

} // namespace VATES
} // namespace Mantid
