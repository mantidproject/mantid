#include "vtkScaleWorkspace.h"
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include <vtkNew.h>
#include "vtkObjectFactory.h"
#include <vtkPointSet.h>
#include <vtkUnsignedCharArray.h>
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkScaleWorkspace);

vtkScaleWorkspace::vtkScaleWorkspace()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkScaleWorkspace::~vtkScaleWorkspace()
{
}


int vtkScaleWorkspace::RequestData(vtkInformation*, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation * inputInf = inputVector[0]->GetInformationObject(0);
  vtkPointSet * inputDataSet = vtkPointSet::SafeDownCast(inputInf->Get(vtkDataObject::DATA_OBJECT()));
  // Grab the original bounding box so we can recall original extents
  inputDataSet->GetBounds(m_bb);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *dataset = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* points = inputDataSet->GetPoints();
  double* point;
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->Allocate(points->GetNumberOfPoints());
  for(int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    point = points->GetPoint(i);
    point[0] *= m_xScaling;
    point[1] *= m_yScaling;
    point[2] *= m_zScaling;
    newPoints->InsertNextPoint(point);
  }
  //Shallow copy the input.
  dataset->ShallowCopy(inputDataSet);
  //Give the output dataset the scaled set of points.
  dataset->SetPoints(newPoints);
  this->UpdateMetaData(dataset);
  return 1;
}

int vtkScaleWorkspace::RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

void vtkScaleWorkspace::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
Setter for the X Scaling factor
@param xScaling : scaling factor in x
*/
void vtkScaleWorkspace::SetXScaling(double xScaling)
{
  if(xScaling != m_xScaling && xScaling > 0)
  {
    this->Modified();
    m_xScaling = xScaling;
  }
}

/**
Setter for the Y Scaling factor
@param yScaling : scaling factor in y
*/
void vtkScaleWorkspace::SetYScaling(double yScaling)
{
  if(yScaling != m_yScaling && yScaling > 0)
  {
    this->Modified();
    m_yScaling = yScaling;
  }
}

/**
Setter for the Z Scaling factor
@param zScaling : scaling factor in z
*/
void vtkScaleWorkspace::SetZScaling(double zScaling)
{
  if(zScaling != m_zScaling && zScaling > 0)
  {
    this->Modified();
    m_zScaling = zScaling;
  }
}

/**
 * Correct the axis extents so that the data ranges show the original
 * extents and not the scaled ones.
 * @param ds
 */
void vtkScaleWorkspace::UpdateMetaData(vtkDataSet *ds)
{
  vtkFieldData *fieldData = ds->GetFieldData();

  // Set that the label range is actively being managed
  vtkNew<vtkUnsignedCharArray> activeLabelRange;
  activeLabelRange->SetNumberOfComponents(1);
  activeLabelRange->SetNumberOfTuples(3);
  activeLabelRange->SetName("LabelRangeActiveFlag");
  fieldData->AddArray(activeLabelRange.GetPointer());
  activeLabelRange->SetValue(0, 1);
  activeLabelRange->SetValue(1, 1);
  activeLabelRange->SetValue(2, 1);

  // Set the original axis extents from the bounding box
  vtkNew<vtkFloatArray> uLabelRange;
  uLabelRange->SetNumberOfComponents(2);
  uLabelRange->SetNumberOfTuples(1);
  uLabelRange->SetName("LabelRangeForX");
  double labelRangeX[2] = {m_bb[0], m_bb[1]};
  uLabelRange->SetTuple(0, labelRangeX);
  fieldData->AddArray(uLabelRange.GetPointer());

  vtkNew<vtkFloatArray> vLabelRange;
  vLabelRange->SetNumberOfComponents(2);
  vLabelRange->SetNumberOfTuples(1);
  vLabelRange->SetName("LabelRangeForY");
  double labelRangeY[2] = {m_bb[2], m_bb[3]};
  vLabelRange->SetTuple(0, labelRangeY);
  fieldData->AddArray(vLabelRange.GetPointer());

  vtkNew<vtkFloatArray> wLabelRange;
  wLabelRange->SetNumberOfComponents(2);
  wLabelRange->SetNumberOfTuples(1);
  wLabelRange->SetName("LabelRangeForZ");
  double labelRangeZ[2] = {m_bb[4], m_bb[5]};
  wLabelRange->SetTuple(0, labelRangeZ);
  fieldData->AddArray(wLabelRange.GetPointer());

  // Set the linear transform for each axis based on scaling factor
  vtkNew<vtkFloatArray> uLinearTransform;
  uLinearTransform->SetNumberOfComponents(2);
  uLinearTransform->SetNumberOfTuples(1);
  uLinearTransform->SetName("LinearTransformForX");
  double transformX[2] = {1.0 / m_xScaling, 0.0};
  uLinearTransform->SetTuple(0, transformX);
  fieldData->AddArray(uLinearTransform.GetPointer());

  vtkNew<vtkFloatArray> vLinearTransform;
  vLinearTransform->SetNumberOfComponents(2);
  vLinearTransform->SetNumberOfTuples(1);
  vLinearTransform->SetName("LinearTransformForY");
  double transformY[2] = {1.0 / m_yScaling, 0.0};
  vLinearTransform->SetTuple(0, transformY);
  fieldData->AddArray(vLinearTransform.GetPointer());

  vtkNew<vtkFloatArray> wLinearTransform;
  wLinearTransform->SetNumberOfComponents(2);
  wLinearTransform->SetNumberOfTuples(1);
  wLinearTransform->SetName("LinearTransformForZ");
  double transformZ[2] = {1.0 / m_zScaling, 0.0};
  wLinearTransform->SetTuple(0, transformZ);
  fieldData->AddArray(wLinearTransform.GetPointer());
}
