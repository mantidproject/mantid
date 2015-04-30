#include "vtkBox.h"
#include "vtkMDEWSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVInformationKeys.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"

using namespace Mantid::VATES;

vtkStandardNewMacro(vtkMDEWSource)

/// Constructor
vtkMDEWSource::vtkMDEWSource() :  m_wsName(""), m_depth(1000), m_time(0), m_presenter(NULL)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkMDEWSource::~vtkMDEWSource()
{
  delete m_presenter;
}

/*
 Setter for the recursion depth
 @param depth : recursion depth to use
*/
void vtkMDEWSource::SetDepth(int depth)
{
  size_t temp = depth;
  if(m_depth != temp)
  {
   this->m_depth = temp;
   this->Modified();
  }
}

/*
  Setter for the workspace name.
  @param name : workspace name to extract from ADS.
*/
void vtkMDEWSource::SetWsName(std::string name)
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
const char* vtkMDEWSource::GetInputGeometryXML()
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

/**
 * Gets the current value of the special coordinates associated with the
 * workspace.
 * @return the special coordinates value
 */
int vtkMDEWSource::GetSpecialCoordinates()
{
  if (NULL == m_presenter)
  {
    return 0;
  }
  try
  {
    return m_presenter->getSpecialCoordinates();
  }
  catch (std::runtime_error &)
  {
    return 0;
  }
}

/**
 * Gets the minimum value of the data associated with the 
 * workspace.
 * @return The minimum value of the workspace data.
 */
double vtkMDEWSource::GetMinValue()
{
  if (NULL == m_presenter)
  {
    return 0.0;
  }
  try
  {
    return m_presenter->getMinValue();
  }
  catch (std::runtime_error &)
  {
    return 0;
  }
}

/**
 * Gets the maximum value of the data associated with the 
 * workspace.
 * @return The maximum value of the workspace data.
 */
double vtkMDEWSource::GetMaxValue()
{
  if (NULL == m_presenter)
  {
    return 0.0;
  }
  try
  {
    return m_presenter->getMaxValue();
  }
  catch (std::runtime_error &)
  {
    return 0;
  }
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
const char* vtkMDEWSource::GetInstrument()
{
  if (NULL == m_presenter)
  {
    return "";
  }
  try
  {
    return m_presenter->getInstrument().c_str();
  }
  catch (std::runtime_error &)
  {
    return "";
  }
}


int vtkMDEWSource::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  if(m_presenter->canReadFile())
  {
    using namespace Mantid::VATES;
    //get the info objects
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      // usually only one actual step requested
      m_time =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    FilterUpdateProgressAction<vtkMDEWSource> loadingProgressUpdate(this, "Loading...");
    FilterUpdateProgressAction<vtkMDEWSource> drawingProgressUpdate(this, "Drawing...");

    ThresholdRange_scptr thresholdRange(new IgnoreZerosThresholdRange());
    vtkMDHexFactory* hexahedronFactory = new vtkMDHexFactory(thresholdRange, "signal");
    vtkMDQuadFactory* quadFactory = new vtkMDQuadFactory(thresholdRange, "signal");
    vtkMDLineFactory* lineFactory = new vtkMDLineFactory(thresholdRange, "signal");

    hexahedronFactory->SetSuccessor(quadFactory);
    quadFactory->SetSuccessor(lineFactory);

    hexahedronFactory->setTime(m_time);
    vtkDataSet* product = m_presenter->execute(hexahedronFactory, loadingProgressUpdate, drawingProgressUpdate);

    //-------------------------------------------------------- Corrects problem whereby boundaries not set propertly in PV.
    vtkBox* box = vtkBox::New();
    box->SetBounds(product->GetBounds());
    vtkPVClipDataSet* clipper = vtkPVClipDataSet::New();
    clipper->SetInputData(product);
    clipper->SetClipFunction(box);
    clipper->SetInsideOut(true);
    clipper->Update();
    vtkDataSet* clipperOutput = clipper->GetOutput();
    //--------------------------------------------------------

    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    output->ShallowCopy(clipperOutput);

    m_presenter->setAxisLabels(output);

    clipper->Delete();
  }
  return 1;
}

int vtkMDEWSource::RequestInformation(vtkInformation *vtkNotUsed(request), vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  if(m_presenter == NULL && !m_wsName.empty())
  {
    m_presenter = new MDEWInMemoryLoadingPresenter(new MDLoadingViewAdapter<vtkMDEWSource>(this), new ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>, m_wsName);
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

void vtkMDEWSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDEWSource::setTimeRange(vtkInformationVector* outputVector)
{
  if(m_presenter->hasTDimensionAvailable())
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkPVInformationKeys::TIME_LABEL_ANNOTATION(),
                 m_presenter->getTimeStepLabel().c_str());
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
size_t vtkMDEWSource::getRecursionDepth() const
{
  return this->m_depth;
}

/*
Getter for the load in memory status
@return true.
*/
bool vtkMDEWSource::getLoadInMemory() const
{
  return true;
}

/*Getter for the time
@return the time.
*/
double vtkMDEWSource::getTime() const
{
  return m_time;
}

/*
Setter for the algorithm progress.
@param progress : progress increment
@param message : progress message
*/
void vtkMDEWSource::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgressText(message.c_str());
  this->SetProgress(progress);
}

/*
Getter for the workspace type name.
*/
char* vtkMDEWSource::GetWorkspaceTypeName()
{
  if(m_presenter == NULL)
  {
    return const_cast<char*>("");
  }
  try
  {
    //Forward request on to MVP presenter
    typeName = m_presenter->getWorkspaceTypeName();
    return const_cast<char*>(typeName.c_str());
  }
  catch(std::runtime_error&)
  {
    return const_cast<char*>("");
  }
}

const char* vtkMDEWSource::GetWorkspaceName()
{
  return m_wsName.c_str();
}
