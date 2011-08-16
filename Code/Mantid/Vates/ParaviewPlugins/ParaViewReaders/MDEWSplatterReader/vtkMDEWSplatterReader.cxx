#include "vtkMDEWSplatterReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "MantidVatesAPI/vtkMDEWHexahedronFactory.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/GaussianThresholdRange.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidVatesAPI/MedianAndBelowThresholdRange.h"

#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/LoadMDEW.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"

vtkCxxRevisionMacro(vtkMDEWSplatterReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkMDEWSplatterReader);

using namespace Mantid::VATES;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

vtkMDEWSplatterReader::vtkMDEWSplatterReader() :
    m_needsLoading(true),
    m_ThresholdRange(new IgnoreZerosThresholdRange())
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  //On first-pass rebinning is necessary.
}

vtkMDEWSplatterReader::~vtkMDEWSplatterReader()
{
  this->SetFileName(0);
}


/**
Sets maximum recursion depth.
@param depth : recursion maximum depth.
*/
void vtkMDEWSplatterReader::SetNumberOfPoints(int depth)
{
  if(depth >= 0)
  {
    size_t temp = static_cast<size_t>(depth);
    if(m_numberPoints != temp)
    {
      m_numberPoints = temp;
      this->Modified();
    }
  }
}


/**
Sets algorithm in-memory property. If this is changed, the file is reloaded.
@param inMemory : true if the entire file should be loaded into memory.
*/
void vtkMDEWSplatterReader::SetInMemory(bool inMemory)
{
  if(m_loadInMemory != inMemory)
  {
    m_loadInMemory = inMemory;
    m_needsLoading = true; // Need to re-load
    this->Modified();   
  }
}


int vtkMDEWSplatterReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  FilterUpdateProgressAction<vtkMDEWSplatterReader> updatehandler(this);
  Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(updatehandler, &ProgressAction::handler);

  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (m_needsLoading)
  {
    AnalysisDataService::Instance().remove("Ws_id");

    Mantid::MDEvents::LoadMDEW alg;

    alg.initialize();
    alg.setPropertyValue("Filename", this->FileName);
    alg.setPropertyValue("OutputWorkspace", "Ws_id");
    alg.setProperty("FileBackEnd", !m_loadInMemory); //Load from file by default.
    alg.setProperty("Memory", 200); //Set a small cache
    alg.addObserver(observer);
    alg.execute();
    alg.removeObserver(observer);

    m_needsLoading = false; //Don't need to reload later
  }
  Workspace_sptr result=AnalysisDataService::Instance().retrieve("Ws_id");

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  std::string scalarName = "signal";
  vtkSplatterPlotFactory vtkGridFactory(m_ThresholdRange, scalarName, m_numberPoints);
  vtkGridFactory.initialize(result);

  output->ShallowCopy(vtkGridFactory.create());

  return 1;
}

int vtkMDEWSplatterReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *)
{
  return 1; 
}

void vtkMDEWSplatterReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkMDEWSplatterReader::CanReadFile(const char* fname)
{
  ::NeXus::File * file = NULL;

  file = new ::NeXus::File(fname);
  // MDEventWorkspace file has a different name for the entry
  try
  {
    file->openGroup("MDEventWorkspace", "NXentry");
    file->close();
    return 1;
  }
  catch(::NeXus::Exception &)
  {
    // If the entry name does not match, then it can't read the file.
    file->close();
    return 0;
  }
  return 0;
}

unsigned long vtkMDEWSplatterReader::GetMTime()
{
  return Superclass::GetMTime();
}

/**
Update/Set the progress.
@parameter progress : progress increment.
*/
void vtkMDEWSplatterReader::updateAlgorithmProgress(double progress)
{
  progressMutex.lock();
  this->SetProgressText("Executing Mantid MDEventWorkspace Loading Algorithm...");
  this->UpdateProgress(progress);
  progressMutex.unlock();
}

