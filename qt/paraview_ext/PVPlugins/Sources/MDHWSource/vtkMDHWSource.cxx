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

using namespace Mantid::VATES;

vtkStandardNewMacro(vtkMDHWSource)

    /// Constructor
    vtkMDHWSource::vtkMDHWSource()
    : m_time(0), m_normalizationOption(AutoSelect) {
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkMDHWSource::~vtkMDHWSource() {}

/*
  Setter for the workspace name.
  @param name : workspace name to extract from ADS.
*/
void vtkMDHWSource::SetWsName(const std::string &name) {
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
std::string vtkMDHWSource::GetInputGeometryXML() {
  if (m_presenter == nullptr) {
    return std::string();
  }
  try {
    return m_presenter->getGeometryXML();
  } catch (std::runtime_error &) {
    return std::string();
  }
}

/**
 * Gets the current value of the special coordinates associated with the
 * workspace.
 * @return the special coordinates value
 */
int vtkMDHWSource::GetSpecialCoordinates() {
  if (nullptr == m_presenter) {
    return 0;
  }
  try {
    return m_presenter->getSpecialCoordinates();
  } catch (std::runtime_error &) {
    return 0;
  }
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
std::string vtkMDHWSource::GetInstrument() {
  if (nullptr == m_presenter) {
    return std::string();
  }
  try {
    return m_presenter->getInstrument();
  } catch (std::runtime_error &) {
    return std::string();
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

    /*
    Will attempt to handle drawing in 4D case and then in 3D case if that fails, and so on down to 1D
    */
    auto factory =
        std::make_unique<vtkMDHistoHex4DFactory<TimeToTimeStep>>(
            m_normalizationOption, m_time);

    factory
        ->setSuccessor(std::make_unique<vtkMDHistoHexFactory>(
            m_normalizationOption))
        .setSuccessor(std::make_unique<vtkMDHistoQuadFactory>(
            m_normalizationOption))
        .setSuccessor(std::make_unique<vtkMDHistoLineFactory>(
            m_normalizationOption))
        .setSuccessor(std::make_unique<vtkMD0DFactory>());

    auto product = m_presenter->execute(factory.get(), loadingProgressUpdate,
                                        drawingProgressUpdate);

    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    output->ShallowCopy(product);
      
    try
    {
      auto workspaceProvider = std::make_unique<ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
      m_presenter->makeNonOrthogonal(output, std::move(workspaceProvider),
                                     &drawingProgressUpdate);
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

int vtkMDHWSource::RequestInformation(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector) {
  if (m_presenter == nullptr && !m_wsName.empty()) {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MDLoadingViewAdapter<vtkMDHWSource>>(this);
    m_presenter = std::make_unique<MDHWInMemoryLoadingPresenter>(
        std::move(view),
        new ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace>, m_wsName);
  }
  if (m_presenter) {
    if (!m_presenter->canReadFile()) {
      vtkErrorMacro(<< "Cannot fetch the specified workspace from Mantid ADS.");
      return 0;
    } else {
      m_presenter->executeLoadMetadata();
      setTimeRange(outputVector);
      MDHWInMemoryLoadingPresenter *castPresenter =
          dynamic_cast<MDHWInMemoryLoadingPresenter *>(m_presenter.get());
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
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/*
Getter for the workspace type name.
*/
std::string vtkMDHWSource::GetWorkspaceTypeName() {
  if (m_presenter == nullptr) {
    return std::string();
  }
  try {
    //Forward request on to MVP presenter
    return m_presenter->getWorkspaceTypeName();
  }
  catch(std::runtime_error&)
  {
    return std::string();
  }
}

const std::string &vtkMDHWSource::GetWorkspaceName() { return m_wsName; }
