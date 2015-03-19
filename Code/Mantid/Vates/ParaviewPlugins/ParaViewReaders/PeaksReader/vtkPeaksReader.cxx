#include "vtkPeaksReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkAxes.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <vtkPVGlyphFilter.h>

#include <boost/algorithm/string.hpp>    

vtkStandardNewMacro(vtkPeaksReader)

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

vtkPeaksReader::vtkPeaksReader() :
  m_isSetup(false), m_wsTypeName(""),
  m_uintPeakMarkerSize(0.3), m_dimensions(1)
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkPeaksReader::~vtkPeaksReader()
{
  this->SetFileName(0);
}


void vtkPeaksReader::SetDimensions(int dimensions)
{
  m_dimensions = dimensions;
  this->Modified();
}

/**
 * Setter for the unintegrated peak marker size
 * @param mSize : size for the marker
 */
void vtkPeaksReader::SetUnintPeakMarkerSize(double mSize)
{
  m_uintPeakMarkerSize = mSize;
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

int vtkPeaksReader::CanReadFile(const char* fname)
{
  const std::string fileString(fname);
  const size_t startExtension = fileString.find_last_of('.');
  const size_t endExtension = fileString.length();
  if(startExtension >= endExtension)
  {
    throw std::runtime_error("File has no extension.");
  }
  std::string extension = fileString.substr(startExtension, endExtension - startExtension);
  boost::algorithm::to_lower(extension);
  boost::algorithm::trim(extension);
  if(extension == ".peaks")
  {
    return 1;
  }
  else
  {
    return 0; 
  }
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
