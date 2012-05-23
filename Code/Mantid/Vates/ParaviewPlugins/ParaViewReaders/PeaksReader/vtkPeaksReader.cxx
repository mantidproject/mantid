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
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <vtkPVGlyphFilter.h>

vtkCxxRevisionMacro(vtkPeaksReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkPeaksReader);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

vtkPeaksReader::vtkPeaksReader() :
  m_isSetup(false), m_wsTypeName("")
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

void vtkPeaksReader::SetDimensions(int dimensions)
{
  m_dimensions = dimensions;
  this->Modified();
}


int vtkPeaksReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Instantiate the factory that makes the peak markers
  vtkPeakMarkerFactory::ePeakDimensions dim;
  switch (m_dimensions)
  {
  case 1: dim = vtkPeakMarkerFactory::Peak_in_Q_lab; break;
  case 2: dim = vtkPeakMarkerFactory::Peak_in_Q_sample; break;
  case 3: dim = vtkPeakMarkerFactory::Peak_in_HKL; break;
  default: dim = vtkPeakMarkerFactory::Peak_in_Q_lab; break;
  }
  vtkPeakMarkerFactory * p_peakFactory = new vtkPeakMarkerFactory("peaks", dim);

  p_peakFactory->initialize(m_PeakWS);

  FilterUpdateProgressAction<vtkPeaksReader> drawingProgressUpdate(this, "Drawing...");
  vtkDataSet * structuredMesh = p_peakFactory->create(drawingProgressUpdate);

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
  Mantid::API::FrameworkManager::Instance();
  //This is just a peaks workspace so should load really quickly.
  if(!m_isSetup) 
  {
    // This actually loads the peaks file
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadIsawPeaks");
    alg->initialize();
    alg->setPropertyValue("Filename", this->FileName);
    alg->setPropertyValue("OutputWorkspace", "LoadedPeaksWS");

    FilterUpdateProgressAction<vtkPeaksReader> updateHandler(this, "Loading...");
    Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(updateHandler, &ProgressAction::handler);

    alg->addObserver(observer);
    alg->execute();
    alg->removeObserver(observer);

    Workspace_sptr result=AnalysisDataService::Instance().retrieve("LoadedPeaksWS");
    m_PeakWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(result);
    m_wsTypeName = m_PeakWS->id();
    m_isSetup = true;
  }

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
  @param progress : progress increment.
  @param message : progress message
*/
void vtkPeaksReader::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/*
Getter for the workspace type name.
*/
char* vtkPeaksReader::GetWorkspaceTypeName()
{
  //Preload the Workspace and then cache it to avoid reloading later.
  return const_cast<char*>(m_wsTypeName.c_str());
}
