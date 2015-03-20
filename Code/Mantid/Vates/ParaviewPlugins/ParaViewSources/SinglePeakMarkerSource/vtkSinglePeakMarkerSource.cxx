#include "vtkSinglePeakMarkerSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "MantidVatesAPI/vtkSinglePeakMarker.h"

vtkStandardNewMacro(vtkSinglePeakMarkerSource);

using namespace Mantid::VATES;

/// Constructor
vtkSinglePeakMarkerSource::vtkSinglePeakMarkerSource() : m_position1(0.0), m_position2(0.0), m_position3(0.0), m_radius(0.1)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkSinglePeakMarkerSource::~vtkSinglePeakMarkerSource()
{
}

int vtkSinglePeakMarkerSource::RequestData(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSinglePeakMarker singlePeak;
  output->ShallowCopy(singlePeak.createSinglePeakMarker(m_position1, m_position2, m_position3, m_radius));

  return 1;
}

int vtkSinglePeakMarkerSource::RequestInformation(vtkInformation *vtkNotUsed(request),
                                                  vtkInformationVector **vtkNotUsed(inputVector),
                                                  vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

void vtkSinglePeakMarkerSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkSinglePeakMarkerSource::SetRadiusMarker(double radius)
{
  m_radius = (radius*0.05);
  this->Modified();
}

void vtkSinglePeakMarkerSource::SetPosition1(double position1)
{
  m_position1 = position1;
  this->Modified();
}

void vtkSinglePeakMarkerSource::SetPosition2(double position2)
{
  m_position2 = position2;
  this->Modified();
}

void vtkSinglePeakMarkerSource::SetPosition3(double position3)
{
  m_position3 = position3;
  this->Modified();
}