#include "vtkMDEWNexusReader.h"
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
#include "vtkNew.h"

#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDLoadingViewAdapter.h"

#include <QtDebug>

vtkStandardNewMacro(vtkMDEWNexusReader)

    using namespace Mantid::VATES;

vtkMDEWNexusReader::vtkMDEWNexusReader()
    : FileName{nullptr}, m_loadInMemory{false}, m_depth{1}, m_time{0},
      m_normalization{NoNormalization} {
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkMDEWNexusReader::~vtkMDEWNexusReader() { this->SetFileName(nullptr); }

void vtkMDEWNexusReader::SetDepth(int depth) {
  size_t temp = depth;
  if (m_depth != temp) {
    this->m_depth = temp;
    this->Modified();
  }
}

size_t vtkMDEWNexusReader::getRecursionDepth() const { return this->m_depth; }

bool vtkMDEWNexusReader::getLoadInMemory() const { return m_loadInMemory; }

double vtkMDEWNexusReader::getTime() const { return m_time; }

/**
  Sets algorithm in-memory property. If this is changed, the file is reloaded.
  @param inMemory : true if the entire file should be loaded into memory.
*/
void vtkMDEWNexusReader::SetInMemory(bool inMemory) {
  if (m_loadInMemory != inMemory) {
    this->Modified();
  }
  m_loadInMemory = inMemory;
}

/**
  Gets the geometry xml from the workspace. Allows object panels to configure
  themeselves.
  @return geometry xml const * char reference.
*/
std::string vtkMDEWNexusReader::GetInputGeometryXML() {
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
@param option : Normalization option chosen by index.
*/
void vtkMDEWNexusReader::SetNormalization(int option) {
  m_normalization = static_cast<Mantid::VATES::VisualNormalization>(option);
  this->Modified();
}

int vtkMDEWNexusReader::RequestData(
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

  FilterUpdateProgressAction<vtkMDEWNexusReader> loadingProgressAction(
      this, "Loading...");
  FilterUpdateProgressAction<vtkMDEWNexusReader> drawingProgressAction(
      this, "Drawing...");

  auto hexahedronFactory =
      std::make_unique<vtkMDHexFactory>(m_normalization);

  hexahedronFactory
      ->setSuccessor(
          std::make_unique<vtkMDQuadFactory>(m_normalization))
      .setSuccessor(
          std::make_unique<vtkMDLineFactory>(m_normalization));

  hexahedronFactory->setTime(m_time);
  vtkDataSet *product = m_presenter->execute(
      hexahedronFactory.get(), loadingProgressAction, drawingProgressAction);

  //-------------------------------------------------------- Corrects problem
  //whereby boundaries not set propertly in PV.
  vtkNew<vtkBox> box;
  box->SetBounds(product->GetBounds());
  vtkNew<vtkPVClipDataSet> clipper;
  clipper->SetInputData(0, product);
  clipper->SetClipFunction(box.GetPointer());
  clipper->SetInsideOut(true);
  clipper->Update();
  vtkDataSet *clipperOutput = clipper->GetOutput();
  //--------------------------------------------------------

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->ShallowCopy(clipperOutput);

  m_presenter->setAxisLabels(output);

  return 1;
}

int vtkMDEWNexusReader::RequestInformation(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector) {
  if (m_presenter == nullptr) {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MDLoadingViewAdapter<vtkMDEWNexusReader>>(
            this);
    m_presenter = std::make_unique<MDEWEventNexusLoadingPresenter>(
        std::move(view), FileName);
    m_presenter->executeLoadMetadata();
    setTimeRange(outputVector);
  }
  return 1;
}

void vtkMDEWNexusReader::PrintSelf(ostream &os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}

int vtkMDEWNexusReader::CanReadFile(const char *fname) {
  std::unique_ptr<MDLoadingView> view =
      std::make_unique<MDLoadingViewAdapter<vtkMDEWNexusReader>>(
          this);
  MDEWEventNexusLoadingPresenter temp(std::move(view), fname);
  return temp.canReadFile();
}

vtkMTimeType vtkMDEWNexusReader::GetMTime() { return Superclass::GetMTime(); }

/**
  Update/Set the progress.
  @param progress : progress increment.
  @param message : progress message.
*/
void vtkMDEWNexusReader::updateAlgorithmProgress(double progress,
                                                 const std::string &message) {
  std::lock_guard<std::mutex> lock(progressMutex);
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDEWNexusReader::setTimeRange(vtkInformationVector *outputVector) {
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
std::string vtkMDEWNexusReader::GetWorkspaceTypeName() {
  // Forward request on to MVP presenter
  return m_presenter->getWorkspaceTypeName();
}
