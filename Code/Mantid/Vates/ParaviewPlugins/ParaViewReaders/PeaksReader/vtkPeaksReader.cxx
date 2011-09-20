#include "vtkPeaksReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <vtkCubeSource.h>

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidCrystal/LoadIsawPeaks.h"

#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"

#include <vtkPVGlyphFilter.h>

vtkCxxRevisionMacro(vtkPeaksReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkPeaksReader);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

vtkPeaksReader::vtkPeaksReader() :
  m_isSetup(false)
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkPeaksReader::~vtkPeaksReader()
{
  this->SetFileName(0);
}


void vtkPeaksReader::SetWidth(double width)
{
  m_width = width;
  this->Modified();
}


int vtkPeaksReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{

  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  Mantid::API::FrameworkManager::Instance();

   //Ensure that the Peaks Workspace is only generated once
  if(!m_isSetup) 
  {
    // This actually loads the peaks file
    Mantid::API::IAlgorithm_sptr alg(new Mantid::Crystal::LoadIsawPeaks());
    alg->initialize();
    alg->setPropertyValue("Filename", this->FileName);
    alg->setPropertyValue("OutputWorkspace", "LoadedPeaksWS");

    FilterUpdateProgressAction<vtkPeaksReader> updateHandler(this);
    Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(updateHandler, &ProgressAction::handler);

    alg->addObserver(observer);
    alg->execute();
    alg->removeObserver(observer);

    Workspace_sptr result=AnalysisDataService::Instance().retrieve("LoadedPeaksWS");
    Mantid::API::IPeaksWorkspace_sptr peakWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(result);
    m_isSetup = true;
  }

  // Instantiate the factory that makes the peak markers
  vtkPeakMarkerFactory * p_peakFactory = new vtkPeakMarkerFactory("peaks");

  Workspace_sptr result=AnalysisDataService::Instance().retrieve("LoadedPeaksWS");
  Mantid::API::IPeaksWorkspace_sptr peakWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(result);

  p_peakFactory->initialize(peakWS);
  vtkDataSet * structuredMesh = p_peakFactory->create();

  vtkCubeSource* cube = vtkCubeSource::New();
  cube->SetXLength(m_width);
  cube->SetYLength(m_width);
  cube->SetZLength(m_width);

  vtkPVGlyphFilter* glyphFilter = vtkPVGlyphFilter::New();
  glyphFilter->SetInput(structuredMesh);
  glyphFilter->SetSource(cube->GetOutput());
  glyphFilter->Update();
  vtkPolyData* glyphed = glyphFilter->GetOutput();

  output->ShallowCopy(glyphed);


  glyphFilter->Delete();

  return 1;
}

int vtkPeaksReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *)
{
  return 1;
}

void vtkPeaksReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkPeaksReader::CanReadFile(const char* vtkNotUsed(fname))
{
  return 1; //TODO: Apply checks here.
}

unsigned long vtkPeaksReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  return mTime;
}

/**
  Update/Set the progress.
  @parameter progress : progress increment.
*/
void vtkPeaksReader::updateAlgorithmProgress(double progress)
{
  this->SetProgressText("Executing Peaks Reader...");
  this->UpdateProgress(progress);
}
