// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAlgorithms/SaveMDWorkspaceToVTKImpl.h"
#include "MantidVatesAPI/Normalization.h"

#include "MantidVatesAPI/FactoryChains.h"
#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingViewSimple.h"
#include "MantidVatesAPI/PresenterFactories.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/SingleWorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Logger.h"


#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <boost/make_shared.hpp>
#include <boost/math/special_functions/round.hpp>
#include <memory>
#include <utility>

namespace {
// This progress object gets called by PV (and is used by the plugins),
// it does not have much use here.
class NullProgressAction : public Mantid::VATES::ProgressAction {
  void eventRaised(double) override {}
};

bool has_suffix(const std::string &stringToCheck, const std::string &suffix) {
  auto isStringLargerThanSuffix = stringToCheck.size() >= suffix.size();
  auto isSuffixInString = false;
  if (isStringLargerThanSuffix) {
    isSuffixInString =
        stringToCheck.compare(stringToCheck.size() - suffix.size(),
                              suffix.size(), suffix) == 0;
  }
  return isSuffixInString;
}

bool isNDWorkspace(const Mantid::API::IMDWorkspace &workspace,
                   const size_t dimensionality) {
  auto actualNonIntegratedDimensionality =
      workspace.getNonIntegratedDimensions().size();
  return actualNonIntegratedDimensionality == dimensionality;
}
} // namespace

namespace Mantid {
namespace VATES {

const std::string SaveMDWorkspaceToVTKImpl::structuredGridExtension = ".vts";
const std::string SaveMDWorkspaceToVTKImpl::unstructuredGridExtension = ".vtu";

SaveMDWorkspaceToVTKImpl::SaveMDWorkspaceToVTKImpl(SaveMDWorkspaceToVTK *parent)
    : m_progress(parent, 0.0, 1.0, 101) {
  setupMembers();
}

/**
 * Save an MD workspace to a vts/vtu file.
 * @param workspace: the workspace which is to be saved.
 * @param filename: the name of the file to which the workspace is to be saved.
 * @param normalization: the visual normalization option
 * @param recursionDepth: the recursion depth for MDEvent Workspaces determines
 * @param compressorType: the compression type used by VTK
 * from which level data should be displayed
 */
void SaveMDWorkspaceToVTKImpl::saveMDWorkspace(
    const Mantid::API::IMDWorkspace_sptr &workspace,
    const std::string &filename, VisualNormalization normalization,
    int recursionDepth, const std::string &compressorType) {
  auto isHistoWorkspace =
      boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace) !=
      nullptr;
  auto fullFilename = getFullFilename(filename, isHistoWorkspace);

  const vtkXMLWriter::CompressorType compressor = [&compressorType] {
    if (compressorType == "NONE") {
      return vtkXMLWriter::NONE;
    } else if (compressorType == "ZLIB") {
      return vtkXMLWriter::ZLIB;
    } else {
      // This should never happen.
      Mantid::Kernel::Logger g_log("SaveMDWorkspaceToVTK");
      g_log.warning("Incorrect CompressorType: " + compressorType +
                    ". Using CompressorType=NONE.");
      return vtkXMLWriter::NONE;
    }
  }();
  // Define a time slice.
  auto time = selectTimeSliceValue(*workspace);

  // Get presenter and data set factory set up
  auto factoryChain =
      getDataSetFactoryChain(isHistoWorkspace, normalization, time);

  auto presenter = getPresenter(isHistoWorkspace, workspace, recursionDepth);

  // Create the vtk data
  NullProgressAction nullProgressA;
  NullProgressAction nullProgressB;
  auto dataSet =
      presenter->execute(factoryChain.get(), nullProgressA, nullProgressB);

  // Do an orthogonal correction
  dataSet = getDataSetWithOrthogonalCorrection(dataSet, presenter.get(),
                                               workspace, isHistoWorkspace);

  // ParaView 5.1 checks the range of the entire signal array, including blank
  // cells.
  if (isHistoWorkspace) {
    auto structuredGrid = vtkStructuredGrid::SafeDownCast(dataSet);
    vtkIdType imageSize = structuredGrid->GetNumberOfCells();
    vtkNew<vtkFloatArray> signal;
    signal->SetNumberOfComponents(1);
    signal->SetNumberOfTuples(imageSize);
    auto oldSignal = structuredGrid->GetCellData()->GetScalars();
    for (vtkIdType index = 0; index < imageSize; ++index) {
      if (structuredGrid->IsCellVisible(index)) {
        signal->SetComponent(index, 0, oldSignal->GetTuple1(index));
      } else {
        signal->SetComponent(index, 0, std::numeric_limits<float>::quiet_NaN());
      }
    }
    structuredGrid->GetCellData()->SetScalars(signal.GetPointer());
  }

  // Write the data to the file
  vtkSmartPointer<vtkXMLWriter> writer = getXMLWriter(isHistoWorkspace);
  auto writeSuccessFlag =
      writeDataSetToVTKFile(writer, dataSet, fullFilename, compressor);
  if (!writeSuccessFlag) {
    throw std::runtime_error("SaveMDWorkspaceToVTK: VTK could not write "
                             "your data set to a file.");
  }
}

/**
 * Creates the correct factory chain based
 * @param isHistoWorkspace: flag if workspace is MDHisto
 * @param normalization: the normalization option
 * @param time: the time slice info
 * @returns a data set factory
 */
std::unique_ptr<vtkDataSetFactory>
SaveMDWorkspaceToVTKImpl::getDataSetFactoryChain(
    bool isHistoWorkspace, VisualNormalization normalization,
    double time) const {
  std::unique_ptr<vtkDataSetFactory> factory;
  if (isHistoWorkspace) {
    factory = createFactoryChainForHistoWorkspace(normalization, time);
  } else {
    factory = createFactoryChainForEventWorkspace(normalization, time);
  }
  return factory;
}

/**
 * Creates the correct factory chain based
 * @param isHistoWorkspace: flag if workspace is MDHisto
 * @param workspace: the workspace
 * @param recursionDepth: the recursion depth
 * @returns a presenter for either MDHisto or MDEvent
 */
std::unique_ptr<MDLoadingPresenter>
SaveMDWorkspaceToVTKImpl::getPresenter(bool isHistoWorkspace,
                                       Mantid::API::IMDWorkspace_sptr workspace,
                                       int recursionDepth) const {
  std::unique_ptr<MDLoadingPresenter> presenter = nullptr;
  auto view = std::make_unique<Mantid::VATES::MDLoadingViewSimple>();
  auto workspaceProvider =
      std::make_unique<SingleWorkspaceProvider>(workspace);
  if (isHistoWorkspace) {
    InMemoryPresenterFactory<MDHWInMemoryLoadingPresenter,
                             EmptyWorkspaceNamePolicy>
        presenterFactory;
    presenter = presenterFactory.create(std::move(view), workspace,
                                        std::move(workspaceProvider));
  } else {
    view->setRecursionDepth(recursionDepth);
    InMemoryPresenterFactory<MDEWInMemoryLoadingPresenter,
                             EmptyWorkspaceNamePolicy>
        presenterFactory;
    presenter = presenterFactory.create(std::move(view), workspace,
                                        std::move(workspaceProvider));
  }
  return presenter;
}

void SaveMDWorkspaceToVTKImpl::progressFunction(vtkObject *caller,
                                                long unsigned, void *) {
  vtkAlgorithm *testFilter = vtkAlgorithm::SafeDownCast(caller);
  if (!testFilter)
    return;
  const char *progressText = testFilter->GetProgressText();

  int progress = boost::math::iround(testFilter->GetProgress() * 100.0);
  if (progressText) {
    this->m_progress.report(progress, progressText);
  } else {
    this->m_progress.report(progress);
  }
}

/**
 * Write an unstructured grid or structured grid to a vtk file.
 * @param writer: a vtk xml writer
 * @param dataSet: the data set which is to be saved out
 * @param filename: the file name
 * @param compressor: the compression type used by VTK
 * @returns a vtk error flag
 */
int SaveMDWorkspaceToVTKImpl::writeDataSetToVTKFile(
    vtkXMLWriter *writer, vtkDataSet *dataSet, const std::string &filename,
    vtkXMLWriter::CompressorType compressor) {
  writer->AddObserver(vtkCommand::ProgressEvent, this,
                      &SaveMDWorkspaceToVTKImpl::progressFunction);
  writer->SetFileName(filename.c_str());
  writer->SetInputData(dataSet);
  writer->SetCompressorType(compressor);
  // Required for large (>4GB?) files.
  writer->SetHeaderTypeToUInt64();
  return writer->Write();
}

/**
 * Get all allowed normalizations
 * @returns all allowed normalization options as strings
 */
std::vector<std::string>
SaveMDWorkspaceToVTKImpl::getAllowedNormalizationsInStringRepresentation()
    const {
  std::vector<std::string> normalizations;
  for (auto it = m_normalizations.begin(); it != m_normalizations.end(); ++it) {
    normalizations.push_back(it->first);
  }

  return normalizations;
}

VisualNormalization
SaveMDWorkspaceToVTKImpl::translateStringToVisualNormalization(
    const std::string &normalization) const {
  return m_normalizations.at(normalization);
}

void SaveMDWorkspaceToVTKImpl::setupMembers() {
  m_normalizations.emplace("AutoSelect", VisualNormalization::AutoSelect);
  m_normalizations.emplace("NoNormalization",
                           VisualNormalization::NoNormalization);
  m_normalizations.emplace("NumEventsNormalization",
                           VisualNormalization::NumEventsNormalization);
  m_normalizations.emplace("VolumeNormalization",
                           VisualNormalization::VolumeNormalization);
}

/**
 * Returns a time for a time slice
 * @param workspace: the workspace
 * @return either the first time entry in case of a 4D workspace or else 0.0
 */
double SaveMDWorkspaceToVTKImpl::selectTimeSliceValue(
    const Mantid::API::IMDWorkspace &workspace) const {
  double time = 0.0;
  if (is4DWorkspace(workspace)) {
    auto timeLikeDimension = workspace.getDimension(3);
    time = static_cast<double>(timeLikeDimension->getMinimum());
  }
  return time;
}

/**
 * Checks if a workspace is 4D
 * @param workspace: the workspace to check
 * @return true if the workspace is 4D else false
 */
bool SaveMDWorkspaceToVTKImpl::is4DWorkspace(
    const Mantid::API::IMDWorkspace &workspace) const {
  const size_t dimensionality = 4;
  return isNDWorkspace(workspace, dimensionality);
}

/**
 * Checks if a workspace is non 3D
 * @param workspace: the workspace to check
 * @return true if the workspace is 3D else false
 */
bool SaveMDWorkspaceToVTKImpl::is3DWorkspace(
    const Mantid::API::IMDWorkspace &workspace) const {
  const size_t dimensionality = 3;
  return isNDWorkspace(workspace, dimensionality);
}

/**
 * Gets the full file name including the correct suffix
 * @param filename: the name of the file except for the suffix
 * @param isHistoWorkspace: flag if the workspace is an MDHistoWorkspace or not
 * @return a full file path including a suffix
 */
std::string
SaveMDWorkspaceToVTKImpl::getFullFilename(std::string filename,
                                          bool isHistoWorkspace) const {
  const auto extension =
      isHistoWorkspace ? structuredGridExtension : unstructuredGridExtension;
  if (!has_suffix(filename, extension)) {
    filename += extension;
  }
  return filename;
}

/**
 * Gets the correct vtk xml writer. For MDHisto workspaces a
 * vtkXMLStructuredGridWriter is required
 * which writes into vts files. For MDEvent workspaces a
 * vtkXMLUnstructuredGridWriter is required
 * which writes into vtu files.
 * @param isHistoWorkspace: flag if the workspace is an MDHistoWorkspace or not
 * @return an vtk xml writer
 */
vtkSmartPointer<vtkXMLWriter>
SaveMDWorkspaceToVTKImpl::getXMLWriter(bool isHistoWorkspace) const {
  vtkSmartPointer<vtkXMLWriter> writer;
  if (isHistoWorkspace) {
    writer = vtkSmartPointer<vtkXMLStructuredGridWriter>::New();
  } else {
    writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  }
  return writer;
}

/**
 * Applies a orthogonal correction to a vtk dataset
 * @param dataSet: the data set to which the correction will be applied
 * @param presenter: the presenter
 * @param workspace: the workspace form which the visual data set was derived
 * @param isHistoWorkspace: flag if the workspace is an MDHistoWorkspace or not
 * @return a data set with orthogonal correction if this is required
 */
vtkSmartPointer<vtkDataSet>
SaveMDWorkspaceToVTKImpl::getDataSetWithOrthogonalCorrection(
    vtkSmartPointer<vtkDataSet> dataSet, MDLoadingPresenter *presenter,
    Mantid::API::IMDWorkspace_sptr workspace, bool isHistoWorkspace) const {
  if (!isHistoWorkspace) {
    vtkSmartPointer<vtkPVClipDataSet> clipped = getClippedDataSet(dataSet);
    dataSet = clipped->GetOutput();
  }

  auto workspaceProvider =
      std::make_unique<SingleWorkspaceProvider>(workspace);
  applyCOBMatrixSettingsToVtkDataSet(presenter, dataSet,
                                     std::move(workspaceProvider));
  presenter->setAxisLabels(dataSet);

  return dataSet;
}
} // namespace VATES
} // namespace Mantid
