#include "vtkBox.h"
#include "vtkMDHWSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVInformationKeys.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"


#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"

using namespace Mantid::VATES;

vtkStandardNewMacro(vtkMDHWSource)

/// Constructor
vtkMDHWSource::vtkMDHWSource() :  m_wsName(""), m_time(0), m_presenter(NULL), m_normalizationOption(AutoSelect)
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

/**
 * Gets the current value of the special coordinates associated with the
 * workspace.
 * @return the special coordinates value
 */
int vtkMDHWSource::GetSpecialCoordinates()
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
double vtkMDHWSource::GetMinValue()
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
    return 0.0;
  }
}

/**
 * Gets the maximum value of the data associated with the 
 * workspace.
 * @return The maximum value of the workspace data.
 */
double vtkMDHWSource::GetMaxValue()
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
    return 0.0;
  }
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
const char* vtkMDHWSource::GetInstrument()
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

/**
Set the normalization option. This is how the signal data will be normalized before viewing.
@param option : Normalization option
*/
void vtkMDHWSource::SetNormalization(int option)
{
  m_normalizationOption = static_cast<Mantid::VATES::VisualNormalization>(option);
  this->Modified();
}


int vtkMDHWSource::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  if(m_presenter->canReadFile())
  {
    using namespace Mantid::VATES;
    //get the info objects
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      // Extracts the actual time.
      m_time =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    FilterUpdateProgressAction<vtkMDHWSource> loadingProgressUpdate(this, "Loading...");
    FilterUpdateProgressAction<vtkMDHWSource> drawingProgressUpdate(this, "Drawing...");

    ThresholdRange_scptr thresholdRange(new IgnoreZerosThresholdRange());

    /*
    Will attempt to handle drawing in 4D case and then in 3D case if that fails, and so on down to 1D
    */
    vtkMD0DFactory* zeroDFactory = new vtkMD0DFactory;
    vtkMDHistoLineFactory* lineFactory = new vtkMDHistoLineFactory(thresholdRange, m_normalizationOption);
    vtkMDHistoQuadFactory* quadFactory = new vtkMDHistoQuadFactory(thresholdRange, m_normalizationOption);
    vtkMDHistoHexFactory* hexFactory = new vtkMDHistoHexFactory(thresholdRange, m_normalizationOption);
    vtkMDHistoHex4DFactory<TimeToTimeStep> *factory = new vtkMDHistoHex4DFactory<TimeToTimeStep>(thresholdRange, m_normalizationOption, m_time);

    factory->SetSuccessor(hexFactory);
    hexFactory->SetSuccessor(quadFactory);
    quadFactory->SetSuccessor(lineFactory);
    lineFactory->SetSuccessor(zeroDFactory);

    vtkDataSet* product = m_presenter->execute(factory, loadingProgressUpdate, drawingProgressUpdate);
      
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    output->ShallowCopy(product);
    product->Delete();
      
    try
    {
      m_presenter->makeNonOrthogonal(output);
    }
    catch (std::invalid_argument &e)
    {
    std::string error = e.what();
      vtkDebugMacro(<< "Workspace does not have correct information to "
                    << "plot non-orthogonal axes. " << error);
      // Add the standard change of basis matrix and set the boundaries
      m_presenter->setDefaultCOBandBoundaries(output);
    }
    m_presenter->setAxisLabels(output);
  }
  return 1;
}

int vtkMDHWSource::RequestInformation(vtkInformation *vtkNotUsed(request), vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  if(m_presenter == NULL && !m_wsName.empty())
  {
    m_presenter = new MDHWInMemoryLoadingPresenter(new MDLoadingViewAdapter<vtkMDHWSource>(this),
                                                   new ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace>,
                                                   m_wsName);
  }
  if (m_presenter) {
    if (!m_presenter->canReadFile()) {
      vtkErrorMacro(<< "Cannot fetch the specified workspace from Mantid ADS.");
      return 0;
    } else {
      m_presenter->executeLoadMetadata();
      setTimeRange(outputVector);
      MDHWInMemoryLoadingPresenter *castPresenter =
          dynamic_cast<MDHWInMemoryLoadingPresenter *>(m_presenter);
      if (castPresenter) {
        std::vector<int> extents = castPresenter->getExtents();
        outputVector->GetInformationObject(0)
            ->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), &extents[0],
                  static_cast<int>(extents.size()));
      }
      return 1;
    }
  } else //(m_presenter == NULL)
  {
    // updater information has been called prematurely. We will reexecute once
    // all attributes are setup.
    return 1;
  }

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

const char* vtkMDHWSource::GetWorkspaceName()
{
  return m_wsName.c_str();
}


