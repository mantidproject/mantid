#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/SaveMDWorkspaceToVTKImpl.h"

#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"

#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingViewSimple.h"
#include "MantidVatesAPI/FactoryChains.h"
#include "MantidVatesAPI/PresenterFactories.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/SingleWorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/make_unique.h"

#include <vtkSmartPointer.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <boost/make_shared.hpp>
#include <memory>

namespace {
// This progress object gets called by PV (and is used by the plugins),
// it does not have much use here.
class NullProgressAction : public Mantid::VATES::ProgressAction {
  virtual void eventRaised(double) {}
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

bool isNDWorkspace(Mantid::API::IMDWorkspace_sptr workspace,
                   const size_t dimensionality) {
  auto actualNonIntegratedDimensionality =
      workspace->getNonIntegratedDimensions().size();
  return actualNonIntegratedDimensionality == dimensionality;
}
}

namespace Mantid {
namespace VATES {

const std::string SaveMDWorkspaceToVTKImpl::structuredGridExtension = "vts";
const std::string SaveMDWorkspaceToVTKImpl::unstructuredGridExtension = "vtu";

SaveMDWorkspaceToVTKImpl::SaveMDWorkspaceToVTKImpl() { setupMembers(); }

/**
 * Save an MD workspace to a vts/vtu file.
 * @param workspace: the workspace which is to be saved.
 * @param filename: the name of the file to which the workspace is to be saved.
 * @param normalization: the visual normalization option
 * @param thresholdRange: a plolicy for the threshold range
 * @param recursionDepth: the recursion depth for MDEvent Workspaces determines
 * from which level data should be displayed
 */
void SaveMDWorkspaceToVTKImpl::saveMDWorkspace(
    Mantid::API::IMDWorkspace_sptr workspace, std::string filename,
    VisualNormalization normalization, ThresholdRange_scptr thresholdRange,
    int recursionDepth) const {
  auto isHistoWorkspace =
      boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace) !=
      nullptr;
  auto fullFilename = getFullFilename(filename, isHistoWorkspace);

  // Define a time slice.
  auto time = selectTimeSliceValue(workspace);

  // Get presenter and data set factory set up
  auto factoryChain = getDataSetFactoryChain(isHistoWorkspace, thresholdRange,
                                             normalization, time);

  auto presenter = getPresenter(isHistoWorkspace, workspace, recursionDepth);

  // Create the vtk data
  NullProgressAction nullProgressA;
  NullProgressAction nullProgressB;
  auto dataSet =
      presenter->execute(factoryChain.get(), nullProgressA, nullProgressB);

  // Do an orthogonal correction
  dataSet = getDataSetWithOrthogonalCorrection(dataSet, presenter.get(),
                                               workspace, isHistoWorkspace);

  // Write the data to the file
  vtkSmartPointer<vtkXMLWriter> writer = getXMLWriter(isHistoWorkspace);
  auto writeSuccessFlag = writeDataSetToVTKFile(writer, dataSet, fullFilename);
  if (!writeSuccessFlag) {
    throw std::runtime_error("SaveMDWorkspaceToVTK: VTK could not write "
                             "your data set to a file.");
  }
}

/**
 * Creates the correct factory chain based
 * @param isHistoWorkspace: flag if workspace is MDHisto
 * @param thresholdRange: the threshold range
 * @param normalization: the normalization option
 * @param time: the time slice info
 * @returns a data set factory
 */
std::unique_ptr<vtkDataSetFactory>
SaveMDWorkspaceToVTKImpl::getDataSetFactoryChain(
    bool isHistoWorkspace, ThresholdRange_scptr thresholdRange,
    VisualNormalization normalization, double time) const {
  std::unique_ptr<vtkDataSetFactory> factory;
  if (isHistoWorkspace) {
    factory = createFactoryChainForHistoWorkspace(thresholdRange, normalization,
                                                  time);
  } else {
    factory = createFactoryChainForEventWorkspace(thresholdRange, normalization,
                                                  time);
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
  auto view = Kernel::make_unique<Mantid::VATES::MDLoadingViewSimple>();
  auto workspaceProvider =
      Mantid::Kernel::make_unique<SingleWorkspaceProvider>(workspace);
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

/**
 * Write an unstructured grid or structured grid to a vtk file.
 * @param writer: a vtk xml writer
 * @param dataSet: the data set which is to be saved out
 * @param filename: the file name
 * @returns a vtk error flag
 */
int SaveMDWorkspaceToVTKImpl::writeDataSetToVTKFile(
    vtkXMLWriter *writer, vtkDataSet *dataSet, std::string filename) const {
  writer->SetFileName(filename.c_str());
  writer->SetInputData(dataSet);
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
    const std::string normalization) const {
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

  m_thresholds.emplace_back("IgnoreZerosThresholdRange");
  m_thresholds.emplace_back("NoThresholdRange");
}

std::vector<std::string>
SaveMDWorkspaceToVTKImpl::getAllowedThresholdsInStringRepresentation() const {
  return m_thresholds;
}

ThresholdRange_scptr SaveMDWorkspaceToVTKImpl::translateStringToThresholdRange(
    const std::string thresholdRange) const {
  if (thresholdRange == m_thresholds[0]) {
    return boost::make_shared<IgnoreZerosThresholdRange>();
  } else if (thresholdRange == m_thresholds[1]) {
    return boost::make_shared<NoThresholdRange>();
  } else {
    throw std::runtime_error("SaveMDWorkspaceToVTK: The selected threshold "
                             "range seems to be incorrect.");
  }
}

/**
 * Returns a time for a time slice
 * @param workspace: the workspace
 * @return either the first time entry in case of a 4D workspace or else 0.0
 */
double SaveMDWorkspaceToVTKImpl::selectTimeSliceValue(
    Mantid::API::IMDWorkspace_sptr workspace) const {
  double time = 0.0;
  if (is4DWorkspace(workspace)) {
    auto timeLikeDimension = workspace->getDimension(3);
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
    Mantid::API::IMDWorkspace_sptr workspace) const {
  const size_t dimensionality = 4;
  return isNDWorkspace(workspace, dimensionality);
}

/**
 * Checks if a workspace is non 3D
 * @param workspace: the workspace to check
 * @return true if the workspace is 3D else false
 */
bool SaveMDWorkspaceToVTKImpl::is3DWorkspace(
    Mantid::API::IMDWorkspace_sptr workspace) const {
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
    filename += ".";
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
      Mantid::Kernel::make_unique<SingleWorkspaceProvider>(workspace);
  applyCOBMatrixSettingsToVtkDataSet(presenter, dataSet,
                                     std::move(workspaceProvider));
  presenter->setAxisLabels(dataSet);

  return dataSet;
}
}
}
