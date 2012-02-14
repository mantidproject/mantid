#include "vtkBox.h"
#include "vtkMDHWSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"

using namespace Mantid::VATES;

vtkCxxRevisionMacro(vtkMDHWSource, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkMDHWSource);

/// Constructor
vtkMDHWSource::vtkMDHWSource() :  m_wsName(""), m_time(0), m_presenter(NULL)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkMDHWSource::~vtkMDHWSource()
{
  delete m_presenter;
}

/*
  Setter for the workspace name.
  @param name : workspace name to extract from ADS.
*/
void vtkMDHWSource::SetWsName(std::string name)
{
  if(m_wsName != name && name != "")
  {
    m_wsName = name;
    this->Modified();
  }
}

/**
  Gets the geometry xml from the workspace. Allows object panels to configure themeselves.
  @return geometry xml const * char reference.
*/
const char* vtkMDHWSource::GetInputGeometryXML()
{
  if(m_presenter == NULL)
  {
    return "";
  }
  try
  {
    return m_presenter->getGeometryXML().c_str();
  }
  catch(std::runtime_error&)
  {
    return "";
  }
}


int vtkMDHWSource::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  if(m_presenter->canReadFile())
  {
    using namespace Mantid::VATES;
    //get the info objects
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
      // Extracts the actual time.
      m_time =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
    }

    FilterUpdateProgressAction<vtkMDHWSource> updateHandler(this, "Loading...");

    ThresholdRange_scptr thresholdRange(new IgnoreZerosThresholdRange());

    /*
    Will attempt to handle drawing in 4D case and then in 3D case if that fails.
    */
    vtkMDHistoHexFactory* successor = new vtkMDHistoHexFactory(thresholdRange, "signal");
    vtkMDHistoHex4DFactory<TimeToTimeStep> *factory = new vtkMDHistoHex4DFactory<TimeToTimeStep>(thresholdRange, "signal", m_time);
    factory->SetSuccessor(successor);
    factory->setCheckDimensionality(false);

    vtkDataSet* product = m_presenter->execute(factory, updateHandler);

    //-------------------------------------------------------- Corrects problem whereby boundaries not set propertly in PV.
    vtkBox* box = vtkBox::New();
    box->SetBounds(product->GetBounds());
    vtkPVClipDataSet* clipper = vtkPVClipDataSet::New();
    clipper->SetInput(product);
    clipper->SetClipFunction(box);
    clipper->SetInsideOut(true);
    clipper->Update();
    vtkDataSet* clipperOutput = clipper->GetOutput();
    //--------------------------------------------------------

    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    output->ShallowCopy(clipperOutput);

    clipper->Delete();
  }
  return 1;
}

int vtkMDHWSource::RequestInformation(vtkInformation *vtkNotUsed(request), vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  if(m_presenter == NULL && !m_wsName.empty())
  {
    m_presenter = new MDHWInMemoryLoadingPresenter(new MDLoadingViewAdapter<vtkMDHWSource>(this), new ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace>, m_wsName);
    if(!m_presenter->canReadFile())
    {
      vtkErrorMacro(<<"Cannot fetch the specified workspace from Mantid ADS.");
    }
    else
    {
      m_presenter->executeLoadMetadata();
      setTimeRange(outputVector);
    }
  }
  return 1;
}

void vtkMDHWSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDHWSource::setTimeRange(vtkInformationVector* outputVector)
{
  if(m_presenter->hasTDimensionAvailable())
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    std::vector<double> timeStepValues = m_presenter->getTimeStepValues();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0],
      static_cast<int> (timeStepValues.size()));
    double timeRange[2];
    timeRange[0] = timeStepValues.front();
    timeRange[1] = timeStepValues.back();

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
}

/*
Getter for the recursion depth.
@return depth
*/
size_t vtkMDHWSource::getRecursionDepth() const
{
  return 0;
}

/*
Getter for the load in memory status
@return true.
*/
bool vtkMDHWSource::getLoadInMemory() const
{
  return true;
}

/*Getter for the time
@return the time.
*/
double vtkMDHWSource::getTime() const
{
  return m_time;
}

/*
Setter for the algorithm progress.
@param progress : progress increment
@param message : progress message 
*/
void vtkMDHWSource::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgress(progress);
  this->SetProgressText(message.c_str());
}

/*
Getter for the workspace type name.
*/
char* vtkMDHWSource::GetWorkspaceTypeName()
{
  if(m_presenter == NULL)
  {
    return "";
  }
  try
  {
    //Forward request on to MVP presenter
    typeName = m_presenter->getWorkspaceTypeName();
    return const_cast<char*>(typeName.c_str());
  }
  catch(std::runtime_error&)
  {
    return "";
  }
}

const char* vtkMDHWSource::GetWorkspaceName()
{
  return m_wsName.c_str();
}
