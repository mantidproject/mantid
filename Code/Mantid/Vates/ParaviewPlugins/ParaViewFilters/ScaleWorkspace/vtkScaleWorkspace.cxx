#include "vtkScaleWorkspace.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkScaleWorkspace, "$Revision: 1.0 $");
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
