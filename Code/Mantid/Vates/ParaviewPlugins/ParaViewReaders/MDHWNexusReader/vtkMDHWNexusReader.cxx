#include "vtkMDHWNexusReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVInformationKeys.h"
#include "vtkBox.h"
#include "vtkUnstructuredGrid.h"

#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"

vtkStandardNewMacro(vtkMDHWNexusReader)

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;

vtkMDHWNexusReader::vtkMDHWNexusReader() :
  m_presenter(NULL),
  m_loadInMemory(false),
  m_depth(1),
  m_time(0)
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkMDHWNexusReader::~vtkMDHWNexusReader()
{
  delete m_presenter;
  this->SetFileName(0);
}

void vtkMDHWNexusReader::SetDepth(int depth)
{
  size_t temp = depth;
  if(m_depth != temp)
  {
   this->m_depth = temp;
   this->Modified();
  }
}

size_t vtkMDHWNexusReader::getRecursionDepth() const
{
  return this->m_depth;
}

bool vtkMDHWNexusReader::getLoadInMemory() const
{
  return m_loadInMemory;
}

double vtkMDHWNexusReader::getTime() const
{
  return m_time;
}

/**
 * Sets algorithm in-memory property. If this is changed, the file is reloaded.
 * @param inMemory : true if the entire file should be loaded into memory.
 */
void vtkMDHWNexusReader::SetInMemory(bool inMemory)
{
  if(m_loadInMemory != inMemory)
  {
    this->Modified(); 
  }
  m_loadInMemory = inMemory;
}

/**
 * Gets the geometry xml from the workspace. Allows object panels to configure themeselves.
 * @return geometry xml const * char reference.
 */
const char* vtkMDHWNexusReader::GetInputGeometryXML()
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

int vtkMDHWNexusReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{

  using namespace Mantid::VATES;
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);


  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // usually only one actual step requested
    m_time =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  FilterUpdateProgressAction<vtkMDHWNexusReader> loadingProgressAction(this, "Loading...");
  FilterUpdateProgressAction<vtkMDHWNexusReader> drawingProgressAction(this, "Drawing...");

  ThresholdRange_scptr thresholdRange(new IgnoreZerosThresholdRange());

  // Will attempt to handle drawing in 4D case and then in 3D case
  // if that fails.
  vtkMDHistoHexFactory* successor = new vtkMDHistoHexFactory(thresholdRange, "signal");
  vtkMDHistoHex4DFactory<TimeToTimeStep> *factory = new vtkMDHistoHex4DFactory<TimeToTimeStep>(thresholdRange, "signal", m_time);
  factory->SetSuccessor(successor);

  vtkDataSet* product = m_presenter->execute(factory, loadingProgressAction, drawingProgressAction);
  
  //-------------------------------------------------------- Corrects problem whereby boundaries not set propertly in PV.
  vtkBox* box = vtkBox::New();
  box->SetBounds(product->GetBounds());
  vtkPVClipDataSet* clipper = vtkPVClipDataSet::New();
  clipper->SetInputData(0, product);
  clipper->SetClipFunction(box);
  clipper->SetInsideOut(true);
  clipper->Update();
  vtkDataSet* clipperOutput = clipper->GetOutput();
   //--------------------------------------------------------

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->ShallowCopy(clipperOutput);
  try
  {
    m_presenter->makeNonOrthogonal(output);
  }
  catch (std::invalid_argument &e)
  {
	std::string error = e.what();
    vtkDebugMacro(<< "Workspace does not have correct information to "
                  << "plot non-orthogonal axes. " << error);
  }
  m_presenter->setAxisLabels(output);

  clipper->Delete();
  
  return 1;
}

int vtkMDHWNexusReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if(m_presenter == NULL)
  {
    m_presenter = new MDHWNexusLoadingPresenter(new MDLoadingViewAdapter<vtkMDHWNexusReader>(this), FileName);
    m_presenter->executeLoadMetadata();
    setTimeRange(outputVector);
  }
  return 1;
}

void vtkMDHWNexusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkMDHWNexusReader::CanReadFile(const char* fname)
{
  MDHWNexusLoadingPresenter temp(new MDLoadingViewAdapter<vtkMDHWNexusReader>(this), fname);
  return temp.canReadFile();
}

unsigned long vtkMDHWNexusReader::GetMTime()
{
  return Superclass::GetMTime();
}

/**
  Update/Set the progress.
  @param progress : progress increment.
  @param message : progress message.
*/
void vtkMDHWNexusReader::updateAlgorithmProgress(double progress, const std::string& message)
{
  progressMutex.lock();
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
  progressMutex.unlock();
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDHWNexusReader::setTimeRange(vtkInformationVector* outputVector)
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
Getter for the workspace type name.
*/
char* vtkMDHWNexusReader::GetWorkspaceTypeName()
{
  //Forward request on to MVP presenter
  typeName = m_presenter->getWorkspaceTypeName();
  return const_cast<char*>(typeName.c_str());
}
