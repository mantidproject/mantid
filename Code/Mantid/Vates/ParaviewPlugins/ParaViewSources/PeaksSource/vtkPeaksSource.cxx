#include "vtkPeaksSource.h"

#include "vtkSphereSource.h"
#include "vtkAxes.h"
#include "vtkTransform.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPVGlyphFilter.h"
#include "vtkTransformPolyDataFilter.h"

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"


vtkStandardNewMacro(vtkPeaksSource);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

/// Constructor
vtkPeaksSource::vtkPeaksSource() :  m_wsName(""), 
  m_wsTypeName(""), m_uintPeakMarkerSize(0.3),
  m_dimToShow(vtkPeakMarkerFactory::Peak_in_Q_lab)
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
  if (!name.empty())
  {
    m_wsName = name;
    this->Modified();
  }
}
void vtkPeaksSource::SetPeakDimension(int dim)
{
  m_dimToShow = static_cast<vtkPeakMarkerFactory::ePeakDimensions>(dim);
  this->Modified();
}

/**
 * Setter for the unintegrated peak marker size
 * @param mSize : size for the marker
 */
void vtkPeaksSource::SetUnintPeakMarkerSize(double mSize)
{
  m_uintPeakMarkerSize = mSize;
  this->Modified();
}

int vtkPeaksSource::RequestData(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *outputVector)
{
  //get the info objects
  if (!m_wsName.empty())
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    FilterUpdateProgressAction<vtkPeaksSource> drawingProgressUpdate(this, "Drawing...");

    vtkPolyData *output = vtkPolyData::SafeDownCast(
                            outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Instantiate the factory that makes the peak markers
    vtkPeakMarkerFactory *p_peakFactory = new vtkPeakMarkerFactory("peaks",
                                                                   m_dimToShow);

    p_peakFactory->initialize(m_PeakWS);
    vtkDataSet *structuredMesh = p_peakFactory->create(drawingProgressUpdate);

    // Pick the radius up from the factory if possible, otherwise use the user-provided value.
    vtkPolyDataAlgorithm* shapeMarker = NULL;
    if(p_peakFactory->isPeaksWorkspaceIntegrated())
    {
      double peakRadius = p_peakFactory->getIntegrationRadius(); 
      const int resolution = 6;
      vtkSphereSource *sphere = vtkSphereSource::New();
      sphere->SetRadius(peakRadius);
      sphere->SetPhiResolution(resolution);
      sphere->SetThetaResolution(resolution);
      shapeMarker = sphere;
    }
    else
    {
      vtkAxes* axis = vtkAxes::New();
      axis->SymmetricOn();
      axis->SetScaleFactor(m_uintPeakMarkerSize);

      vtkTransform* transform = vtkTransform::New();
      const double rotationDegrees = 45;
      transform->RotateX(rotationDegrees);
      transform->RotateY(rotationDegrees);
      transform->RotateZ(rotationDegrees);

      vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();
      transformFilter->SetTransform(transform);
      transformFilter->SetInputConnection(axis->GetOutputPort());
      transformFilter->Update();
      shapeMarker = transformFilter;
    }

    vtkPVGlyphFilter *glyphFilter = vtkPVGlyphFilter::New();
    glyphFilter->SetInputData(structuredMesh);
    glyphFilter->SetSourceConnection(shapeMarker->GetOutputPort());
    glyphFilter->Update();
    vtkPolyData *glyphed = glyphFilter->GetOutput();

    output->ShallowCopy(glyphed);

    glyphFilter->Delete();
  }
  return 1;
}

int vtkPeaksSource::RequestInformation(vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **vtkNotUsed(inputVector),
                                       vtkInformationVector *vtkNotUsed(outputVector))
{
  if (!m_wsName.empty())
  {
    //Preload the Workspace and then cache it to avoid reloading later.
    Workspace_sptr result = AnalysisDataService::Instance().retrieve(m_wsName);
    m_PeakWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(result);
    m_wsTypeName = m_PeakWS->id();
    m_instrument = m_PeakWS->getInstrument()->getName();
  }
  return 1;
}

void vtkPeaksSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkPeaksSource::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}


/*
Getter for the workspace type name.
*/
char* vtkPeaksSource::GetWorkspaceTypeName()
{
  return const_cast<char*>(m_wsTypeName.c_str());
}

const char* vtkPeaksSource::GetWorkspaceName()
{
  return m_wsName.c_str();
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
const char* vtkPeaksSource::GetInstrument()
{
  return m_instrument.c_str();
}