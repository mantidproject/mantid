#include "vtkPeaksFilter.h"
#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkUnstructuredGrid.h>

vtkStandardNewMacro(vtkPeaksFilter);

using namespace Mantid::VATES;

vtkPeaksFilter::vtkPeaksFilter() : m_peaksWorkspaceName(""),
                                   m_radiusNoShape(0.5),
                                   m_radiusType(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkPeaksFilter::~vtkPeaksFilter()
{
}


int vtkPeaksFilter::RequestData(vtkInformation*, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *outputDataSet = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSetToPeaksFilteredDataSet peaksFilter(inputDataSet, outputDataSet);
  peaksFilter.initialize(m_peaksWorkspaceName, m_radiusNoShape, m_radiusType);
  peaksFilter.execute();

  return 1;
}

int vtkPeaksFilter::RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

void vtkPeaksFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
 * Set the peaks workspace name
 * @param peaksWorkspaceName The peaks workspace name.
*/
void vtkPeaksFilter::SetPeaksWorkspace(std::string peaksWorkspaceName)
{
  m_peaksWorkspaceName = peaksWorkspaceName;
}

/**
 * Set the radius for PeakShape == NoShape.
 * @param radius The radius
 */
void vtkPeaksFilter::SetRadiusNoShape(double radius)
{
  m_radiusNoShape = radius;
  this->Modified();
}

/**
 * Set the radius type.
 * @param type The type of the radius
 */
void vtkPeaksFilter::SetRadiusType(int type)
{
  m_radiusType = type;
  this->Modified();
}
