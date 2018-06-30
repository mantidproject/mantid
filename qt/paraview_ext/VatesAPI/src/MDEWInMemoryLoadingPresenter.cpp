#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <vtkUnstructuredGrid.h>

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
MDEWInMemoryLoadingPresenter::MDEWInMemoryLoadingPresenter(
    std::unique_ptr<MDLoadingView> view, WorkspaceProvider *repository,
    std::string wsName)
    : MDEWLoadingPresenter(std::move(view)), m_repository(repository),
      m_wsName(wsName), m_wsTypeName(""), m_specialCoords(-1) {
  if (m_wsName.empty()) {
    throw std::invalid_argument("The workspace name is empty.");
  }
  if (nullptr == repository) {
    throw std::invalid_argument("The repository is NULL");
  }
  if (nullptr == m_view) {
    throw std::invalid_argument("View is NULL.");
  }
}

/*
Indicates whether this presenter is capable of handling the type of file that
is attempted to be loaded.
@return false if the file cannot be read.
*/
bool MDEWInMemoryLoadingPresenter::canReadFile() const {
  bool bCanReadIt = true;
  if (!m_repository->canProvideWorkspace(m_wsName)) {
    // The workspace does not exist.
    bCanReadIt = false;
  } else if (nullptr ==
             boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(
                 m_repository->fetchWorkspace(m_wsName))
                 .get()) {
    // The workspace can be found, but is not an IMDEventWorkspace.
    bCanReadIt = false;
  } else {
    // The workspace is present, and is of the correct type.
    bCanReadIt = true;
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
MDEWInMemoryLoadingPresenter::execute(vtkDataSetFactory *factory,
                                      ProgressAction &,
                                      ProgressAction &drawingProgressUpdate) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
  IMDEventWorkspace_sptr eventWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws);

  factory->setRecursionDepth(this->m_view->getRecursionDepth());
  auto visualDataSet = factory->oneStepCreate(eventWs, drawingProgressUpdate);

  /*extractMetaData needs to be re-run here because the first execution of this
    from ::executeLoadMetadata will not have ensured that all dimensions
    have proper range extents set.
  */
  this->extractMetadata(*eventWs);

  this->appendMetadata(visualDataSet, eventWs->getName());
  return visualDataSet;
}

/**
 Executes any meta-data loading required.
*/
void MDEWInMemoryLoadingPresenter::executeLoadMetadata() {
  using namespace Mantid::API;

  Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
  IMDEventWorkspace_sptr eventWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws);
  m_wsTypeName = eventWs->id();
  m_specialCoords = eventWs->getSpecialCoordinateSystem();

  // Set the instrument which is associated with the workspace.
  m_metadataJsonManager->setInstrument(
      m_metaDataExtractor->extractInstrument(eventWs.get()));

  // Set the special coordinates
  m_metadataJsonManager->setSpecialCoordinates(m_specialCoords);

  // Call base-class extraction method.
  this->extractMetadata(*eventWs);
}

/// Destructor
MDEWInMemoryLoadingPresenter::~MDEWInMemoryLoadingPresenter() {}

/*
 Getter for the workspace type name.
 @return Workspace Type Name
*/
std::string MDEWInMemoryLoadingPresenter::getWorkspaceTypeName() {
  return m_wsTypeName;
}

/**
 * Getter for the special coordinates.
 * @return the special coordinates value
 */
int MDEWInMemoryLoadingPresenter::getSpecialCoordinates() {
  return m_specialCoords;
}
} // namespace VATES
} // namespace Mantid
