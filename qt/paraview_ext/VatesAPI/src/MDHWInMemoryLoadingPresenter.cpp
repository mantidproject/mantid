#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/FactoryChains.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

namespace Mantid {
namespace VATES {

/*
Constructor
@param view : MVP view
@param repository : Object for accessing the workspaces
@param wsName : Name of the workspace to use.
@throw invalid_argument if the workspace name is empty
@throw invalid_argument if the repository is null
@throw invalid_arument if view is null
*/
MDHWInMemoryLoadingPresenter::MDHWInMemoryLoadingPresenter(
    std::unique_ptr<MDLoadingView> view, WorkspaceProvider *repository,
    std::string wsName)
    : MDHWLoadingPresenter(std::move(view)), m_repository(repository),
      m_wsName(wsName), m_wsTypeName(""), m_specialCoords(-1) {
  if (m_wsName.empty()) {
    throw std::invalid_argument("The workspace name is empty.");
  }
  if (!repository) {
    throw std::invalid_argument("The repository is NULL");
  }
  if (!m_view) {
    throw std::invalid_argument("View is NULL.");
  }
}

/*
Indicates whether this presenter is capable of handling the type of file that is
attempted to be loaded.
@return false if the file cannot be read.
*/
bool MDHWInMemoryLoadingPresenter::canReadFile() const {
  bool bCanReadIt = true;
  if (!m_repository->canProvideWorkspace(m_wsName)) {
    // The workspace does not exist.
    bCanReadIt = false;
  } else if (boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(
                 m_repository->fetchWorkspace(m_wsName))) {
    // The workspace can be found, but is not an IMDHistoWorkspace.
    bCanReadIt = true;
  } else {
    // The workspace is present, and is of the correct type.
    bCanReadIt = false;
  }
  return bCanReadIt;
}

/*
Executes the underlying algorithm to create the MVP model.
@param factory : visualisation factory to use.
@param  : Handler for GUI updates while algorithm progresses.
@param drawingProgressUpdate : Handler for GUI updates while
vtkDataSetFactory::create occurs.
*/
vtkSmartPointer<vtkDataSet>
MDHWInMemoryLoadingPresenter::execute(vtkDataSetFactory *factory,
                                      ProgressAction &,
                                      ProgressAction &drawingProgressUpdate) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
  IMDHistoWorkspace_sptr histoWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws);

  MDHWLoadingPresenter::transposeWs(histoWs, m_cachedVisualHistoWs);

  // factory->setRecursionDepth(this->m_view->getRecursionDepth());
  auto visualDataSet = factory->oneStepCreate(
      m_cachedVisualHistoWs,
      drawingProgressUpdate); // HACK: progressUpdate should be
                              // argument for drawing!

  /*extractMetaData needs to be re-run here because the first execution of this
   from ::executeLoadMetadata will not have ensured that all dimensions
   have proper range extents set.
  */
  this->extractMetadata(*m_cachedVisualHistoWs);

  // Transposed workpace is temporary, outside the ADS, and does not have a
  // name. so get it from pre-transposed.
  // If this fails, create a default name with a time stamp
  auto name = histoWs->getName();
  if (name.empty()) {
    name = createTimeStampedName("HistoWS");
  }
  this->appendMetadata(visualDataSet, name);
  return visualDataSet;
}

/**
 Executes any meta-data loading required.
*/
void MDHWInMemoryLoadingPresenter::executeLoadMetadata() {
  using namespace Mantid::API;

  Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
  IMDHistoWorkspace_sptr histoWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws);
  m_wsTypeName = histoWs->id();
  m_specialCoords = histoWs->getSpecialCoordinateSystem();

  MDHWLoadingPresenter::transposeWs(histoWs, m_cachedVisualHistoWs);

  // Set the instrument which is associated with the workspace.
  m_metadataJsonManager->setInstrument(
      m_metaDataExtractor->extractInstrument(m_cachedVisualHistoWs.get()));

  // Set the special coordinates
  m_metadataJsonManager->setSpecialCoordinates(m_specialCoords);

  // Call base-class extraction method.
  this->extractMetadata(*m_cachedVisualHistoWs);
}

/// Destructor
MDHWInMemoryLoadingPresenter::~MDHWInMemoryLoadingPresenter() {}

/*
 * Getter for the workspace type name.
 * @return Workspace Type Name
 */
std::string MDHWInMemoryLoadingPresenter::getWorkspaceTypeName() {
  return m_wsTypeName;
}

/**
 * Getter for the special coordinates.
 * @return the special coordinates value
 */
int MDHWInMemoryLoadingPresenter::getSpecialCoordinates() {
  return m_specialCoords;
}

std::vector<int> MDHWInMemoryLoadingPresenter::getExtents() {
  using namespace Mantid::API;
  Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
  IMDHistoWorkspace_sptr histoWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws);
  MDHWLoadingPresenter::transposeWs(histoWs, m_cachedVisualHistoWs);
  std::vector<int> extents(6, 0);
  extents[1] =
      static_cast<int>(m_cachedVisualHistoWs->getXDimension()->getNBins());
  extents[3] =
      static_cast<int>(m_cachedVisualHistoWs->getYDimension()->getNBins());
  extents[5] =
      static_cast<int>(m_cachedVisualHistoWs->getZDimension()->getNBins());
  return extents;
}
}
}
