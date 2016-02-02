#include "MantidVatesAPI/SaveMDWorkspaceToVTKImpl.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/Normalization.h"

#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"

#include "MantidVatesAPI/MDLoadingViewSimple.h"
#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/PresenterUtilities.h"

#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"

#include "MantidGeometry/MDGeometry/IMDDimension.h"

#include <vtkSmartPointer.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkStructuredGridWriter.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"

#include "MantidKernel/make_unique.h"
#include <memory>
#include <boost/make_shared.hpp>

namespace {
class NullProgressAction : public Mantid::VATES::ProgressAction
{
  virtual void eventRaised(double)
  {
  }
};

/**
* Get full file path with the correct extension
* @param filename: the name of the file but without the extension
* @param extension: the extension type
*/
std::string getFullFilePathWithExtension(std::string filename, const std::string extension) {
  auto fullFilename = filename + extension;
  return fullFilename;
}
}


namespace Mantid {
namespace VATES {

const std::string SaveMDWorkspaceToVTKImpl::structuredGridExtension = ".vts";
const std::string SaveMDWorkspaceToVTKImpl::unstructuredGridExtension = ".vtu";

SaveMDWorkspaceToVTKImpl::SaveMDWorkspaceToVTKImpl() {
  setupMembers();
}

/**
 * Save an MDHisto workspace to a vts file.
 * @param histoWS: the histo workspace which is to be saved.
 * @param filename: the name of the file to which the workspace is to be saved.
 * @param normalization: the visual normalization option
 * @param thresholdRange: a plolicy for the threshold range
 */
void SaveMDWorkspaceToVTKImpl::saveMDHistoWorkspace(Mantid::API::IMDHistoWorkspace_sptr histoWS,
  std::string filename, VisualNormalization normalization, ThresholdRange_scptr thresholdRange) const {

  auto fullFilename = getFullFilePathWithExtension(filename, structuredGridExtension);

  // Define a time slice.
  double time = selectTimeSliceValue(histoWS);

  // Set up a presenter
  std::unique_ptr<MDLoadingView> view = Kernel::make_unique<Mantid::VATES::MDLoadingViewSimple>();

  auto presenter = createInMemoryPresenter<MDHWInMemoryLoadingPresenter, Mantid::API::IMDHistoWorkspace>(std::move(view), histoWS->name());

  // Set up vtk data factory chain
  auto factoryChain = createFactoryChain<vtkMDHistoHex4DFactory, vtkMDHistoHexFactory,
                                         vtkMDHistoQuadFactory, vtkMDHistoLineFactory,
                                         vtkMD0DFactory>(thresholdRange, normalization, time);

  // Create the vtk data
  NullProgressAction nullProgressA;
  NullProgressAction nullProgressB;
  auto dataSet = presenter->execute(factoryChain.get(), nullProgressA, nullProgressB);

  // Do an orthogonal correction
  applyCOBMatrixSettingsToVtkDataSet(presenter.get(), dataSet);
  presenter->setAxisLabels(dataSet);

  // Write the data to the file
  auto writer = vtkSmartPointer<vtkXMLStructuredGridWriter>::New();
  writeDataSetToVTKFile(writer, dataSet, fullFilename);
}

/**
 * Save an MDEvent workspace to a vtu file.
 * @param eventWS: the event workspace which is to be saved.
 * @param filename: the name of the file to which the workspace is to be saved.
 * @param normalization: the visual normalization option
 * @param thresholdRange: a plolicy for the threshold range
 */
void SaveMDWorkspaceToVTKImpl::saveMDEventWorkspace(Mantid::API::IMDEventWorkspace_sptr eventWS,
  std::string filename, VisualNormalization normalization, ThresholdRange_scptr thresholdRange) const {

}

/**
 * Write an unstructured grid or structured grid to a vtk file.
 */
void SaveMDWorkspaceToVTKImpl::writeDataSetToVTKFile(vtkXMLWriter* writer, vtkDataSet* dataSet, std::string filename) const {
  writer->SetFileName(filename.c_str());
  writer->SetInputData(dataSet);
  writer->Write();
}

/**
 * Get all allowed normalizations
 * @returns all allowed normalization options as strings
 */
std::vector<std::string> SaveMDWorkspaceToVTKImpl::getAllowedNormalizationsInStringRepresentation() const {
  std::vector<std::string> normalizations;
  for (auto it = m_normalizations.begin(); it != m_normalizations.end(); ++it) {
    normalizations.push_back(it->first);
  }

  return normalizations;
}

VisualNormalization SaveMDWorkspaceToVTKImpl::translateStringToVisualNormalization(const std::string normalization) const {
  return m_normalizations.at(normalization);
}

void SaveMDWorkspaceToVTKImpl::setupMembers() {
  m_normalizations.insert(std::make_pair("AutoSelect", VisualNormalization::AutoSelect));
  m_normalizations.insert(std::make_pair("NoNormalization", VisualNormalization::NoNormalization));
  m_normalizations.insert(std::make_pair("NumEventsNormalization", VisualNormalization::NumEventsNormalization));
  m_normalizations.insert(std::make_pair("VolumeNormalization", VisualNormalization::VolumeNormalization));

  m_thresholds.push_back("IgnoreZerosThresholdRange");
  m_thresholds.push_back("NoThresholdRange");
}

std::vector<std::string> SaveMDWorkspaceToVTKImpl::getAllowedThresholdsInStringRepresentation() const {
  return m_thresholds;
}


ThresholdRange_scptr SaveMDWorkspaceToVTKImpl::translateStringToThresholdRange(const std::string thresholdRange) const {
  if (thresholdRange == m_thresholds[0]) {
      return boost::make_shared<IgnoreZerosThresholdRange>();
  } else if (thresholdRange == m_thresholds[1]) {
      return boost::make_shared<NoThresholdRange>();
  } else {
    throw std::runtime_error("SaveMDWorkspaceToVTK: The selected threshold range seems to be incorrect.");
  }
}


/**
 * Returns a time for a time slice
 * @param workspace: the workspace
 * @return either the first time entry in case of a 4D workspace or else 0.0
 */
double SaveMDWorkspaceToVTKImpl::selectTimeSliceValue(Mantid::API::IMDWorkspace_sptr workspace) const {
  double time = 0.0;
  if (is4DWorkspace(workspace)) {
      auto timeLikeDimension = workspace->getDimension(3);
      time = static_cast<double>(timeLikeDimension->getMinimum());
  }
  return time;
}

bool SaveMDWorkspaceToVTKImpl::is4DWorkspace(Mantid::API::IMDWorkspace_sptr workspace) const {
  auto actualNonIntegratedDimensionality = workspace->getNonIntegratedDimensions().size();
  const size_t dimensionsWithTime = 4;
  return actualNonIntegratedDimensionality == dimensionsWithTime;
}

}
}
