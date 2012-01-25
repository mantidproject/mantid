#include "vtkPeaksSource.h"

#include "vtkCubeSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPVGlyphFilter.h"

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"

vtkCxxRevisionMacro(vtkPeaksSource, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkPeaksSource);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

/// Constructor
vtkPeaksSource::vtkPeaksSource() :  m_wsName(""), m_wsTypeName("")
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

void vtkPeaksSource::SetWidth(double width)
{
  m_width = width;
  this->Modified();
}

int vtkPeaksSource::RequestData(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *outputVector)
{
  //get the info objects
  if (!m_wsName.empty())
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    FilterUpdateProgressAction<vtkPeaksSource> updateHandler(this);

    vtkPolyData *output = vtkPolyData::SafeDownCast(
                            outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Instantiate the factory that makes the peak markers
    vtkPeakMarkerFactory *p_peakFactory = new vtkPeakMarkerFactory("peaks");

    p_peakFactory->initialize(m_PeakWS);
    vtkDataSet *structuredMesh = p_peakFactory->create();

    vtkCubeSource *cube = vtkCubeSource::New();
    cube->SetXLength(m_width);
    cube->SetYLength(m_width);
    cube->SetZLength(m_width);

    vtkPVGlyphFilter *glyphFilter = vtkPVGlyphFilter::New();
    glyphFilter->SetInput(structuredMesh);
    glyphFilter->SetSource(cube->GetOutput());
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
  }
  return 1;
}

void vtkPeaksSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkPeaksSource::updateAlgorithmProgress(double progress)
{
  this->SetProgressText("Executing Peaks Source...");
  this->UpdateProgress(progress);
}


/*
Getter for the workspace type name.
*/
char* vtkPeaksSource::GetWorkspaceTypeName()
{
  return const_cast<char*>(m_wsTypeName.c_str());
}