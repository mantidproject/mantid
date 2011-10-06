#include "vtkBox.h"
#include "vtkPeaksSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkPeaksSource, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkPeaksSource);

/// Constructor
vtkPeaksSource::vtkPeaksSource() :  m_wsName("")
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkPeaksSource::~vtkPeaksSource()
{
}

/**
  Setter for the workspace name.
  @param name : workspace name to extract from ADS.
*/
void vtkPeaksSource::SetWsName(std::string name)
{
  if(m_wsName != name)
  {
    m_wsName = name;
    this->Modified();
  }
}

int vtkPeaksSource::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

int vtkPeaksSource::RequestInformation(vtkInformation *vtkNotUsed(request), vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

void vtkPeaksSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkPeaksSource::updateAlgorithmProgress(double)
{
}
