#include "vtkScaleWorkspace.h"

#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkUnstructuredGrid.h>

vtkStandardNewMacro(vtkScaleWorkspace);

using namespace Mantid::VATES;

vtkScaleWorkspace::vtkScaleWorkspace() :
  m_xScaling(1),
  m_yScaling(1),
  m_zScaling(1)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkScaleWorkspace::~vtkScaleWorkspace()
{
}


int vtkScaleWorkspace::RequestData(vtkInformation*, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *outputDataSet = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSetToScaledDataSet scaler(inputDataSet, outputDataSet);
  scaler.initialize(m_xScaling, m_yScaling, m_zScaling);
  scaler.execute();
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