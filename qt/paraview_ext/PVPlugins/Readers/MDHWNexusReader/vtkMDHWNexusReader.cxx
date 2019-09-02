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

#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"

vtkStandardNewMacro(vtkMDHWNexusReader)

    using namespace Mantid::VATES;

vtkMDHWNexusReader::vtkMDHWNexusReader()
    : FileName{nullptr}, m_loadInMemory(false), m_depth(1), m_time(0),
      m_normalizationOption(AutoSelect) {
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkMDHWNexusReader::~vtkMDHWNexusReader() { this->SetFileName(nullptr); }

void vtkMDHWNexusReader::SetDepth(int depth) {
  size_t temp = depth;
  if (m_depth != temp) {
    this->m_depth = temp;
    this->Modified();
  }
}

size_t vtkMDHWNexusReader::getRecursionDepth() const { return this->m_depth; }

bool vtkMDHWNexusReader::getLoadInMemory() const { return m_loadInMemory; }

double vtkMDHWNexusReader::getTime() const { return m_time; }

/**
 * Sets algorithm in-memory property. If this is changed, the file is reloaded.
 * @param inMemory : true if the entire file should be loaded into memory.
 */
void vtkMDHWNexusReader::SetInMemory(bool inMemory) {
  if (m_loadInMemory != inMemory) {
    this->Modified();
  }
  m_loadInMemory = inMemory;
}

/**
 * Gets the geometry xml from the workspace. Allows object panels to configure
 * themeselves.
 * @return geometry xml const * char reference.
 */
std::string vtkMDHWNexusReader::GetInputGeometryXML() {
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
Set the normalization option. This is how the signal data will be normalized
before viewing.
@param option : Normalization option
*/
void vtkMDHWNexusReader::SetNormalization(int option) {
  m_normalizationOption =
      static_cast<Mantid::VATES::VisualNormalization>(option);
  this->Modified();
}

int vtkMDHWNexusReader::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector) {

  using namespace Mantid::VATES;
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())) {
    // usually only one actual step requested
    m_time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  FilterUpdateProgressAction<vtkMDHWNexusReader> loadingProgressAction(
      this, "Loading...");
  FilterUpdateProgressAction<vtkMDHWNexusReader> drawingProgressAction(
      this, "Drawing...");

  // Will attempt to handle drawing in 4D case and then in 3D case
  // if that fails.
  auto factory =
      std::make_unique<vtkMDHistoHex4DFactory<TimeToTimeStep>>(
          m_normalizationOption, m_time);
  factory->setSuccessor(
      std::make_unique<vtkMDHistoHexFactory>(m_normalizationOption));

  auto product = m_presenter->execute(factory.get(), loadingProgressAction,
                                      drawingProgressAction);

  vtkDataSet *output = vtkDataSet::GetData(outInfo);
  output->ShallowCopy(product);

  try {
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    m_presenter->makeNonOrthogonal(output, std::move(workspaceProvider),
                                   &drawingProgressAction);
  } catch (std::invalid_argument &e) {
    std::string error = e.what();
    vtkDebugMacro(<< "Workspace does not have correct information to "
                  << "plot non-orthogonal axes. " << error);
  }
  m_presenter->setAxisLabels(output);

  return 1;
}

int vtkMDHWNexusReader::RequestInformation(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector) {
  if (m_presenter == nullptr) {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MDLoadingViewAdapter<vtkMDHWNexusReader>>(
            this);
    m_presenter = std::make_unique<MDHWNexusLoadingPresenter>(
        std::move(view), FileName);
  }

  if (m_presenter == nullptr) {
    // updater information has been called prematurely. We will reexecute once
    // all attributes are setup.
    return 1;
  }
  if (!m_presenter->canReadFile()) {
    vtkErrorMacro(<< "Cannot fetch the specified workspace from Mantid ADS.");
    return 0;
  }

  m_presenter->executeLoadMetadata();
  setTimeRange(outputVector);
  std::vector<int> extents = m_presenter->getExtents();
  outputVector->GetInformationObject(0)
      ->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), &extents[0],
            static_cast<int>(extents.size()));
  return 1;
}

void vtkMDHWNexusReader::PrintSelf(ostream &os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}

int vtkMDHWNexusReader::CanReadFile(const char *fname) {
  std::unique_ptr<MDLoadingView> view =
      std::make_unique<MDLoadingViewAdapter<vtkMDHWNexusReader>>(
          this);
  MDHWNexusLoadingPresenter temp(std::move(view), fname);
  return temp.canReadFile();
}

vtkMTimeType vtkMDHWNexusReader::GetMTime() { return Superclass::GetMTime(); }

/**
  Update/Set the progress.
  @param progress : progress increment.
  @param message : progress message.
*/
void vtkMDHWNexusReader::updateAlgorithmProgress(double progress,
                                                 const std::string &message) {
  std::lock_guard<std::mutex> lockGuard(progressMutex);
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDHWNexusReader::setTimeRange(vtkInformationVector *outputVector) {
  if (m_presenter->hasTDimensionAvailable()) {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkPVInformationKeys::TIME_LABEL_ANNOTATION(),
                 m_presenter->getTimeStepLabel().c_str());
    std::vector<double> timeStepValues = m_presenter->getTimeStepValues();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &timeStepValues[0], static_cast<int>(timeStepValues.size()));
    double timeRange[2];
    timeRange[0] = timeStepValues.front();
    timeRange[1] = timeStepValues.back();

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
}

/*
Getter for the workspace type name.
*/
std::string vtkMDHWNexusReader::GetWorkspaceTypeName() {
  // Forward request on to MVP presenter
  return m_presenter->getWorkspaceTypeName();
}
