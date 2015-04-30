#include "vtkNexusPeaksReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkAxes.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
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
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
#include <boost/algorithm/string.hpp>    

vtkStandardNewMacro(vtkNexusPeaksReader)

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

vtkNexusPeaksReader::vtkNexusPeaksReader() :
  m_isSetup(false), m_wsTypeName(""),
  m_uintPeakMarkerSize(0.3), m_dimensions(1)
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkNexusPeaksReader::~vtkNexusPeaksReader()
{
  this->SetFileName(0);
}


void vtkNexusPeaksReader::SetDimensions(int dimensions)
{
  m_dimensions = dimensions;
  this->Modified();
}

/**
 * Setter for the unintegrated peak marker size
 * @param mSize : size for the marker
 */
void vtkNexusPeaksReader::SetUnintPeakMarkerSize(double mSize)
{
  m_uintPeakMarkerSize = mSize;
  this->Modified();
}

int vtkNexusPeaksReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
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

  FilterUpdateProgressAction<vtkNexusPeaksReader> drawingProgressUpdate(this, "Drawing...");
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

int vtkNexusPeaksReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *)
{
  Mantid::API::FrameworkManager::Instance();
  //This is just a peaks workspace so should load really quickly.
  if(!m_isSetup) 
  {
    // This actually loads the peaks file
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadNexusProcessed");
    alg->initialize();
    alg->setPropertyValue("Filename", this->FileName);
    alg->setPropertyValue("OutputWorkspace", "LoadedPeaksWS");

    FilterUpdateProgressAction<vtkNexusPeaksReader> updateHandler(this, "Loading...");
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

void vtkNexusPeaksReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkNexusPeaksReader::CanReadFile(const char* fname)
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
  if(extension != ".nxs")
  {
    return 0; 
  }

  ::NeXus::File * file = NULL;
  file = new ::NeXus::File(fileString);
  try
  {
    try
    {
      std::map<std::string, std::string> entries = file->getEntries();
      auto topEntryName = entries.begin()->first;
      file->openGroup(topEntryName, "NXentry");
      entries = file->getEntries();
      for (auto it = entries.begin(); it != entries.end(); ++it)
      {
        if ((it->first == "peaks_workspace") && (it->second == "NXentry"))
        {
          file->close();
          return 1;
        }
      }
    }
    catch(::NeXus::Exception &)
    {
      file->close();
      return 0;
    }
  }
  catch(std::exception& ex)
  {
    std::cerr << "Could not open " << fileString << " as an PeaksWorkspace nexus file because of exception: " << ex.what() << std::endl;
    // Clean up, if possible
    if (file)
      file->close();
  }
  file->close();
  return 0;
}

unsigned long vtkNexusPeaksReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  return mTime;
}

/**
  Update/Set the progress.
  @param progress : progress increment.
  @param message : progress message
*/
void vtkNexusPeaksReader::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/*
Getter for the workspace type name.
*/
char* vtkNexusPeaksReader::GetWorkspaceTypeName()
{
  //Preload the Workspace and then cache it to avoid reloading later.
  return const_cast<char*>(m_wsTypeName.c_str());
}
