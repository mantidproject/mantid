#include "vtkMDEWNexusReaderII.h"
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

vtkCxxRevisionMacro(vtkMDEWNexusReaderII, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkMDEWNexusReaderII);

using namespace Mantid::VATES;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

vtkMDEWNexusReaderII::vtkMDEWNexusReaderII() : m_ThresholdRange(new IgnoreZerosThresholdRange())
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  //On first-pass rebinning is necessary.
}

vtkMDEWNexusReaderII::~vtkMDEWNexusReaderII()
{
  this->SetFileName(0);
}


/**
Sets maximum recursion depth.
@param depth : recursion maximum depth.
*/
void vtkMDEWNexusReaderII::SetRecursionDepth(int depth)
{
  if(depth > 0)
  {
    if(m_recursionDepth != depth)
    {
      m_recursionDepth = static_cast<size_t>(depth);
      this->Modified();
    }
  }
}


/**
Sets algorithm in-memory property. If this is changed, the file is reloaded.
@param inMemory : true if the entire file should be loaded into memory.
*/
void vtkMDEWNexusReaderII::SetInMemory(bool inMemory)
{
  if(m_loadInMemory != inMemory)
  {
    m_loadInMemory = inMemory;
    this->Modified();   
  }
}


int vtkMDEWNexusReaderII::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  FilterUpdateProgressAction<vtkMDEWNexusReaderII> updatehandler(this);
  Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(updatehandler, &ProgressAction::handler);

  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  AnalysisDataService::Instance().remove("Ws_id");

  Mantid::MDEvents::LoadMDEW alg;

  alg.initialize();
  alg.setPropertyValue("Filename", this->FileName);
  alg.setPropertyValue("OutputWorkspace", "Ws_id");
  alg.setProperty("FileBackEnd", !m_loadInMemory); //Load from file by default.
  alg.addObserver(observer);
  alg.execute();
  alg.removeObserver(observer);

  Workspace_sptr result=AnalysisDataService::Instance().retrieve("Ws_id");

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  std::string scalarName = "signal";
  vtkMDEWHexahedronFactory vtkGridFactory(m_ThresholdRange, scalarName, m_recursionDepth);
  vtkGridFactory.initialize(result);

  output->ShallowCopy(vtkGridFactory.create());

  return 1;
}

int vtkMDEWNexusReaderII::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  return 1; 
}

void vtkMDEWNexusReaderII::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkMDEWNexusReaderII::CanReadFile(const char* fname)
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

unsigned long vtkMDEWNexusReaderII::GetMTime()
{
  return Superclass::GetMTime();
}

/**
Update/Set the progress.
@parameter progress : progress increment.
*/
void vtkMDEWNexusReaderII::updateAlgorithmProgress(double progress)
{
  progressMutex.lock();
  this->SetProgressText("Executing Mantid MDEvent Loading Algorithm...");
  this->UpdateProgress(progress);
  progressMutex.unlock();
}

