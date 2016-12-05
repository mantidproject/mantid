#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidVatesAPI/FactoryChains.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <qwt_double_interval.h>

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
  if (NULL == repository) {
    throw std::invalid_argument("The repository is NULL");
  }
  if (nullptr == m_view) {
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
  } else if (NULL ==
             boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(
                 m_repository->fetchWorkspace(m_wsName)).get()) {
    // The workspace can be found, but is not an IMDHistoWorkspace.
    bCanReadIt = false;
  } else {
    // The workspace is present, and is of the correct type.
    bCanReadIt = true;
  }
  return bCanReadIt;
}

namespace {
class CellVisibility {
public:
  explicit CellVisibility(const unsigned char *array)
      : MASKED_CELL_VALUE(vtkDataSetAttributes::HIDDENCELL |
                          vtkDataSetAttributes::REFINEDCELL),
        InputCellGhostArray(array) {}
  bool operator()(vtkIdType id) const {
    if (InputCellGhostArray)
      return !(this->InputCellGhostArray[id] & this->MASKED_CELL_VALUE);
    else
      return true;
  }

private:
  const unsigned char MASKED_CELL_VALUE;
  const unsigned char *InputCellGhostArray;
};

void ComputeScalarRange(vtkStructuredGrid *grid, double *cellRange) {
  auto cga = grid->GetCellGhostArray();
  CellVisibility isCellVisible(cga ? cga->GetPointer(0) : nullptr);
  vtkDataArray *cellScalars = grid->GetCellData()->GetScalars();
  double minvalue = VTK_DOUBLE_MAX;
  double maxvalue = VTK_DOUBLE_MIN;
  int num = boost::numeric_cast<int>(grid->GetNumberOfCells());
#if defined(_OPENMP) && _OPENMP >= 200805
  PRAGMA_OMP(parallel for reduction(min : minvalue) reduction(max : maxvalue))
#endif
  for (int id = 0; id < num; id++) {
    if (isCellVisible(id)) {
      double s = cellScalars->GetComponent(id, 0);
      minvalue = std::min(minvalue, s);
      maxvalue = std::max(maxvalue, s);
    }
  }
  cellRange[0] = minvalue;
  cellRange[1] = maxvalue;
}
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

  // Update the meta data min and max values with the values of the visual data
  // set. This is necessary since we want the full data range of the visual
  // data set and not of the actual underlying data set.

  // vtkStructuredGrid::GetScalarRange(...) is slow and single-threaded.
  // Until this is addressed in VTK, we are better of doing the calculation
  // ourselves.
  // 600x600x600 vtkStructuredGrid, every other cell blank
  // structuredGrid->GetScalarRange(range) : 2.36625s
  // structuredGrid->GetCellData()->GetScalars()->GetRange(range) : 1.01453s
  // ComputeScalarRange(structuredGrid,range): 0.086104s
  double range[2];
  if (auto structuredGrid = vtkStructuredGrid::SafeDownCast(visualDataSet)) {
    ComputeScalarRange(structuredGrid, range);
  } else {
    // should never happen
    visualDataSet->GetScalarRange(range);
  }

  this->m_metadataJsonManager->setMinValue(range[0]);
  this->m_metadataJsonManager->setMaxValue(range[1]);

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

  // Set the minimum and maximum of the workspace data.
  QwtDoubleInterval minMaxContainer =
      m_metaDataExtractor->getMinAndMax(histoWs);
  m_metadataJsonManager->setMinValue(minMaxContainer.minValue());
  m_metadataJsonManager->setMaxValue(minMaxContainer.maxValue());

  // Set the instrument which is associated with the workspace.
  m_metadataJsonManager->setInstrument(
      m_metaDataExtractor->extractInstrument(m_cachedVisualHistoWs));

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
