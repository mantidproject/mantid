// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/MDHWNexusLoadingPresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on
#include "MantidAPI/AlgorithmManager.h"
#include <vtkUnstructuredGrid.h>

namespace Mantid {
namespace VATES {

/**
 * Constructor
 * @param view : MVP view
 * @param filename : name of file to load
 * @throw invalid_argument if file name is empty
 * @throw invalid_arument if view is null
 * @throw logic_error if cannot use the reader-presenter for this filetype.
 */
MDHWNexusLoadingPresenter::MDHWNexusLoadingPresenter(
    std::unique_ptr<MDLoadingView> view, const std::string &filename)
    : MDHWLoadingPresenter(std::move(view)), m_filename(filename),
      m_wsTypeName("") {
  if (this->m_filename.empty()) {
    throw std::invalid_argument("File name is an empty string.");
  }
  if (nullptr == this->m_view) {
    throw std::invalid_argument("View is NULL.");
  }
}

/**
 * Indicates whether this presenter is capable of handling the type of file that
 * is attempted to be loaded.
 * @return false if the file cannot be read.
 */
bool MDHWNexusLoadingPresenter::canReadFile() const {
  // Quick check based on extension.
  if (!canLoadFileBasedOnExtension(m_filename, ".nxs")) {
    return false;
  }
  auto file = std::make_unique<::NeXus::File>(this->m_filename);
  // MDHistoWorkspace file has a different name for the entry
  try {
    file->openGroup("MDHistoWorkspace", "NXentry");
    return true;
  } catch (::NeXus::Exception &) {
    // If the entry name does not match, then it can't read the file.
    return false;
  }
  return false;
}

/**
 * Executes the underlying algorithm to create the MVP model.
 * @param factory : visualisation factory to use.
 * @param loadingProgressUpdate : Handler for GUI updates while algorithm
 * progresses.
 * @param drawingProgressUpdate : Handler for GUI updates while
 * vtkDataSetFactory::create occurs.
 */
vtkSmartPointer<vtkDataSet>
MDHWNexusLoadingPresenter::execute(vtkDataSetFactory *factory,
                                   ProgressAction &loadingProgressUpdate,
                                   ProgressAction &drawingProgressUpdate) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  if (this->shouldLoad() || !m_histoWs)
    this->loadWorkspace(loadingProgressUpdate);

  // Create visualisation in one-shot.
  auto visualDataSet = factory->oneStepCreate(m_histoWs, drawingProgressUpdate);

  // extractMetaData needs to be re-run here because the first execution
  // of this from ::executeLoadMetadata will not have ensured that all
  // dimensions have proper range extents set.
  this->extractMetadata(*m_histoWs);

  this->appendMetadata(visualDataSet, m_histoWs->getName());
  return visualDataSet;
}

/**
 * Executes any meta-data loading required.
 */
void MDHWNexusLoadingPresenter::executeLoadMetadata() {
  using namespace Mantid::API;

  if (this->shouldLoad() || !m_histoWs) {
    // no difference between loading just the metadata or the entire workspace.
    this->loadWorkspace();
  }
  m_wsTypeName = m_histoWs->id();
  // Call base-class extraction method.
  this->extractMetadata(*m_histoWs);
}

/**
 * Destructor
 * @return
 */
MDHWNexusLoadingPresenter::~MDHWNexusLoadingPresenter() {}

/**
 * Getter for the workspace type name.
 * @return Workspace Type Name
 */
std::string MDHWNexusLoadingPresenter::getWorkspaceTypeName() {
  return m_wsTypeName;
}

std::vector<int> MDHWNexusLoadingPresenter::getExtents() {

  std::vector<int> extents(6, 0);

  if (this->shouldLoad() || !m_histoWs)
    this->loadWorkspace();

  if (m_histoWs) {
    extents[1] = static_cast<int>(m_histoWs->getXDimension()->getNBins());
    extents[3] = static_cast<int>(m_histoWs->getYDimension()->getNBins());
    extents[5] = static_cast<int>(m_histoWs->getZDimension()->getNBins());
  }

  return extents;
}

void MDHWNexusLoadingPresenter::loadWorkspace() {
  using namespace Mantid::API;
  AnalysisDataService::Instance().remove("MD_HISTO_WS_ID");

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");
  alg->initialize();
  alg->setPropertyValue("Filename", this->m_filename);
  alg->setPropertyValue("OutputWorkspace", "MD_HISTO_WS_ID");
  alg->setProperty(
      "FileBackEnd",
      !this->m_view->getLoadInMemory()); // Load from file by default.
  alg->execute();
  Workspace_sptr result =
      AnalysisDataService::Instance().retrieve("MD_HISTO_WS_ID");
  auto preTranspose =
      boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(result);
  // Perform any necessary transpose.
  MDHWLoadingPresenter::transposeWs(preTranspose, m_histoWs);
}

void MDHWNexusLoadingPresenter::loadWorkspace(
    ProgressAction &loadingProgressUpdate) {
  using namespace Mantid::API;
  Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification>
      observer(loadingProgressUpdate, &ProgressAction::handler);

  AnalysisDataService::Instance().remove("MD_HISTO_WS_ID");
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");
  alg->initialize();
  alg->setPropertyValue("Filename", this->m_filename);
  alg->setPropertyValue("OutputWorkspace", "MD_HISTO_WS_ID");
  alg->setProperty(
      "FileBackEnd",
      !this->m_view->getLoadInMemory()); // Load from file by default.
  alg->addObserver(observer);
  alg->execute();
  alg->removeObserver(observer);
  m_histoWs = AnalysisDataService::Instance()
                  .retrieveWS<Mantid::API::IMDHistoWorkspace>("MD_HISTO_WS_ID");
}
} // namespace VATES
} // namespace Mantid
