#include "vtkMDEWSource.h"

#include "vtkBox.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVInformationKeys.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/BoxInfo.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"
#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"

#include <boost/optional.hpp>

using namespace Mantid::VATES;

vtkStandardNewMacro(vtkMDEWSource)

    /// Constructor
    vtkMDEWSource::vtkMDEWSource()
    : m_depth(1000), m_time(0), m_normalization(AutoSelect) {
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkMDEWSource::~vtkMDEWSource() = default;

/*
 Setter for the recursion depth
 @param depth : recursion depth to use
*/
void vtkMDEWSource::SetDepth(int depth) {
  size_t temp = depth;
  if (m_depth != temp) {
    this->m_depth = temp;
    this->Modified();
  }
}

/*
  Setter for the workspace name.
  @param name : workspace name to extract from ADS.
*/
void vtkMDEWSource::SetWsName(const std::string &name) {
  if (m_wsName != name && !name.empty()) {
    m_wsName = name;
    this->Modified();
  }
}

/**
  Gets the geometry xml from the workspace. Allows object panels to configure
  themeselves.
  @return geometry xml const * char reference.
*/
std::string vtkMDEWSource::GetInputGeometryXML() {
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
int vtkMDEWSource::GetSpecialCoordinates() {
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
std::string vtkMDEWSource::GetInstrument() {
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
Set the normalization option. This is how the signal data will be normalized
before viewing.
@param option : Normalization option
*/
void vtkMDEWSource::SetNormalization(int option) {
  m_normalization = static_cast<Mantid::VATES::VisualNormalization>(option);
  this->Modified();
}

int vtkMDEWSource::RequestData(vtkInformation *, vtkInformationVector **,
                               vtkInformationVector *outputVector) {
  if (m_presenter->canReadFile()) {
    using namespace Mantid::VATES;
    // get the info objects
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())) {
      // usually only one actual step requested
      m_time =
          outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    FilterUpdateProgressAction<vtkMDEWSource> loadingProgressUpdate(
        this, "Loading...");
    FilterUpdateProgressAction<vtkMDEWSource> drawingProgressUpdate(
        this, "Drawing...");

    auto hexahedronFactory =
        std::make_unique<vtkMDHexFactory>(m_normalization);

    hexahedronFactory
        ->setSuccessor(
            std::make_unique<vtkMDQuadFactory>(m_normalization))
        .setSuccessor(
            std::make_unique<vtkMDLineFactory>(m_normalization))
        .setSuccessor(std::make_unique<vtkMD0DFactory>());

    hexahedronFactory->setTime(m_time);
    vtkSmartPointer<vtkDataSet> product;
    product = m_presenter->execute(
        hexahedronFactory.get(), loadingProgressUpdate, drawingProgressUpdate);

    // Corrects problem whereby boundaries not set propertly in PV.
    auto box = vtkSmartPointer<vtkBox>::New();
    box->SetBounds(product->GetBounds());
    auto clipper = vtkSmartPointer<vtkPVClipDataSet>::New();
    clipper->SetInputData(product);
    clipper->SetClipFunction(box);
    clipper->SetInsideOut(true);
    clipper->Update();
    auto clipperOutput = clipper->GetOutput();
    //--------------------------------------------------------

    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));
    output->ShallowCopy(clipperOutput);

    try {
      auto workspaceProvider = std::make_unique<
          ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
      m_presenter->makeNonOrthogonal(output, std::move(workspaceProvider),
                                     &drawingProgressUpdate);
    } catch (std::invalid_argument &e) {
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

GNU_DIAG_OFF("strict-aliasing")
int vtkMDEWSource::RequestInformation(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector) {
  if (!m_presenter && !m_wsName.empty()) {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MDLoadingViewAdapter<vtkMDEWSource>>(this);
    m_presenter = std::make_unique<MDEWInMemoryLoadingPresenter>(
        std::move(view),
        new ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>, m_wsName);
    if (m_presenter->canReadFile()) {
      // If the MDEvent workspace has had top level splitting applied to it,
      // then use the a deptgit stah of 1
      auto workspaceProvider = std::make_unique<
          ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>>();
      if (auto split = Mantid::VATES::findRecursionDepthForTopLevelSplitting(
              m_wsName, *workspaceProvider)) {
        SetDepth(split.get());
      }

      m_presenter->executeLoadMetadata();
      setTimeRange(outputVector);
    } else {
      vtkErrorMacro(<< "Cannot fetch the specified workspace from Mantid ADS.");
    }
  }
  return 1;
}

void vtkMDEWSource::PrintSelf(ostream &os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDEWSource::setTimeRange(vtkInformationVector *outputVector) {
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
Getter for the recursion depth.
@return depth
*/
size_t vtkMDEWSource::getRecursionDepth() const { return this->m_depth; }

/*
Getter for the load in memory status
@return true.
*/
bool vtkMDEWSource::getLoadInMemory() const { return true; }

/*Getter for the time
@return the time.
*/
double vtkMDEWSource::getTime() const { return m_time; }

/*
Setter for the algorithm progress.
@param progress : progress increment
@param message : progress message
*/
void vtkMDEWSource::updateAlgorithmProgress(double progress,
                                            const std::string &message) {
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/*
Getter for the workspace type name.
*/
std::string vtkMDEWSource::GetWorkspaceTypeName() {
  if (m_presenter == nullptr) {
    return std::string();
  }
  try {
    // Forward request on to MVP presenter
    return m_presenter->getWorkspaceTypeName();
  } catch (std::runtime_error &) {
    return std::string();
  }
}

const std::string &vtkMDEWSource::GetWorkspaceName() { return m_wsName; }
