// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkMDHexFactory.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDNode.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ReadLock.h"

#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include <iterator>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::ReadLock;

namespace Mantid {

namespace VATES {

/*Constructor
  @param normalizationOption : Info object setting how normalization should be
  done.
  @param maxDepth : Maximum depth to search to
  */
vtkMDHexFactory::vtkMDHexFactory(const VisualNormalization normalizationOption,
                                 const size_t maxDepth)
    : m_normalizationOption(normalizationOption), m_maxDepth(maxDepth),
      slice(false), m_time(0) {}

/// Destructor
vtkMDHexFactory::~vtkMDHexFactory() {}

//-------------------------------------------------------------------------------------------------
/* Generate the vtkDataSet from the objects input MDEventWorkspace (of a given
 *type an dimensionality 3+)
 *
 * @param ws: workspace to draw from
 * @return a fully constructed vtkUnstructuredGrid containing geometric and
 *scalar data.
 */
template <typename MDE, size_t nd>
void vtkMDHexFactory::doCreate(
    typename MDEventWorkspace<MDE, nd>::sptr ws) const {
  bool VERBOSE = true;
  CPUTimer tim;
  // Acquire a scoped read-only lock to the workspace (prevent segfault from
  // algos modifying ws)
  ReadLock lock(*ws);

  // First we get all the boxes, up to the given depth; with or wo the slice
  // function
  std::vector<API::IMDNode *> boxes;
  if (this->slice)
    ws->getBox()->getBoxes(boxes, m_maxDepth, true,
                           this->sliceImplicitFunction.get());
  else
    ws->getBox()->getBoxes(boxes, m_maxDepth, true);

  vtkIdType numBoxes = boxes.size();
  vtkIdType imageSizeActual = 0;

  if (VERBOSE)
    std::cout << tim << " to retrieve the " << numBoxes
              << " boxes down to depth " << m_maxDepth << '\n';

  // Create 8 points per box.
  vtkNew<vtkPoints> points;
  vtkFloatArray *pointsArray = vtkFloatArray::FastDownCast(points->GetData());
  float *pointsPtr = pointsArray->WritePointer(0, numBoxes * 8 * 3);

  // One scalar per box
  vtkNew<vtkFloatArray> signals;
  signals->SetName(ScalarName.c_str());
  signals->SetNumberOfComponents(1);
  float *signalsPtr = signals->WritePointer(0, numBoxes);

  // To cache the signal
  std::vector<float> signalCache(numBoxes, 0);

  // True for boxes that we will use
  // We do not use vector<bool> here because of the parallelization below
  // Simultaneous access to different elements of vector<bool> is not safe
  auto useBox = std::make_unique<bool[]>(numBoxes);
  memset(useBox.get(), 0, sizeof(bool) * numBoxes);

  // Create the data set (will outlive this object - output of create)
  auto visualDataSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->dataSet = visualDataSet;
  visualDataSet->Allocate(numBoxes);

  vtkNew<vtkIdList> hexPointList;
  hexPointList->SetNumberOfIds(8);
  auto hexPointList_ptr = hexPointList->WritePointer(0, 8);

  NormFuncIMDNodePtr normFunction =
      makeMDEventNormalizationFunction(m_normalizationOption, ws.get());

  // This can be parallelized
  // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for schedule (dynamic) )
    for (int ii = 0; ii < int(boxes.size()); ii++) {
      // Get the box here
      size_t i = static_cast<size_t>(ii);
      API::IMDNode *box = boxes[i];
      Mantid::signal_t signal_normalized = (box->*normFunction)();

      if (std::isnormal(signal_normalized)) {
        // Cache the signal and using of it
        signalCache[i] = static_cast<float>(signal_normalized);
        useBox[i] = true;

        // Get the coordinates.
        size_t numVertexes = 0;
        std::unique_ptr<coord_t[]> coords;

        // If slicing down to 3D, specify which dimensions to keep.
        if (this->slice) {
          coords = box->getVertexesArray(numVertexes, 3, this->sliceMask.get());
        } else {
          coords = box->getVertexesArray(numVertexes);
        }
        if (numVertexes == 8) {
          std::copy_n(coords.get(), 24, std::next(pointsPtr, i * 24));
        }
      } else {
        useBox[i] = false;
      }
    } // For each box

    if (VERBOSE)
      std::cout << tim << " to create the necessary points.\n";
    // Add points
    visualDataSet->SetPoints(points.GetPointer());

    for (size_t i = 0; i < boxes.size(); i++) {
      if (useBox[i]) {
        // The bare point ID
        vtkIdType pointId = i * 8;

        // Add signal
        *signalsPtr = signalCache[i];
        std::advance(signalsPtr, 1);

        const std::array<vtkIdType, 8> idList{{0, 1, 3, 2, 4, 5, 7, 6}};

        std::transform(
            idList.begin(), idList.end(), hexPointList_ptr,
            std::bind(std::plus<vtkIdType>(), std::placeholders::_1, pointId));

        // Add cells
        visualDataSet->InsertNextCell(VTK_HEXAHEDRON,
                                      hexPointList.GetPointer());

        imageSizeActual++;
      }
    } // for each box.

    // Shrink to fit
    signals->Resize(imageSizeActual);
    signals->Squeeze();
    visualDataSet->Squeeze();

    // Add scalars
    visualDataSet->GetCellData()->SetScalars(signals.GetPointer());

    // Hedge against empty data sets
    if (visualDataSet->GetNumberOfPoints() <= 0) {
      vtkNullUnstructuredGrid nullGrid;
      visualDataSet = nullGrid.createNullData();
      this->dataSet = visualDataSet;
    }

    if (VERBOSE)
      std::cout << tim << " to create " << imageSizeActual << " hexahedrons.\n";
}

//-------------------------------------------------------------------------------------------------
/*
Generate the vtkDataSet from the objects input IMDEventWorkspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@Return a fully constructed vtkUnstructuredGrid containing geometric and scalar
data.
*/
vtkSmartPointer<vtkDataSet>
vtkMDHexFactory::create(ProgressAction &progressUpdating) const {
  this->dataSet = tryDelegatingCreation<IMDEventWorkspace, 3>(
      m_workspace, progressUpdating, false);
  if (this->dataSet) {
    return this->dataSet;
  } else {
    IMDEventWorkspace_sptr imdws =
        this->castAndCheck<IMDEventWorkspace, 3>(m_workspace, false);

    size_t nd = imdws->getNumDims();
    if (nd > 3) {
      // Slice from >3D down to 3D
      this->slice = true;
      this->sliceMask = std::make_unique<bool[]>(nd);
      this->sliceImplicitFunction = boost::make_shared<MDImplicitFunction>();

      // Make the mask of dimensions
      for (size_t d = 0; d < nd; d++)
        this->sliceMask[d] = (d < 3);

      // Define where the slice is in 4D
      std::vector<coord_t> point(nd, 0);

      // Define two opposing planes that point in all higher dimensions
      std::vector<coord_t> normal1(nd, 0);
      std::vector<coord_t> normal2(nd, 0);
      for (size_t d = 3; d < nd; d++) {
        normal1[d] = +1.0;
        normal2[d] = -1.0;
      }
      // This creates a slice which is one bin thick in the 4th dimension
      // m_time assumed to satisfy: dim_min <= m_time < dim_max
      // but does not have to be a bin centre
      point[3] = getPreviousBinBoundary(imdws);
      sliceImplicitFunction->addPlane(MDPlane(normal1, point));
      point[3] = getNextBinBoundary(imdws);
      sliceImplicitFunction->addPlane(MDPlane(normal2, point));

    } else {
      // Direct 3D, so no slicing
      this->slice = false;
    }
    progressUpdating.eventRaised(0.1);
    // Macro to call the right instance of the
    CALL_MDEVENT_FUNCTION(this->doCreate, imdws);
    progressUpdating.eventRaised(1.0);

    // The macro does not allow return calls, so we used a member variable.
    return this->dataSet;
  }
}

/*
 * Get the next highest bin boundary
 */
coord_t
vtkMDHexFactory::getNextBinBoundary(const IMDEventWorkspace_sptr &imdws) const {
  auto t_dim = imdws->getTDimension();
  coord_t bin_width = t_dim->getBinWidth();
  coord_t dim_min = t_dim->getMinimum();
  return roundUp(coord_t(m_time) - dim_min, bin_width) + dim_min;
}

/*
 * Get the previous bin boundary, or the current one if m_time is on a boundary
 */
coord_t vtkMDHexFactory::getPreviousBinBoundary(
    const IMDEventWorkspace_sptr &imdws) const {
  auto t_dim = imdws->getTDimension();
  coord_t bin_width = t_dim->getBinWidth();
  coord_t dim_min = t_dim->getMinimum();
  return roundDown(coord_t(m_time) - dim_min, bin_width) + dim_min;
}

/*
 * Round up to next multiple of factor
 * Where "up" means towards +ve infinity
 */
coord_t roundUp(const coord_t num_to_round, const coord_t factor) {
  return (std::floor(num_to_round / factor) + 1) * factor;
}

/*
 * Round down to next multiple of factor
 * Where "down" means towards -ve infinity
 */
coord_t roundDown(const coord_t num_to_round, const coord_t factor) {
  return std::floor(num_to_round / factor) * factor;
}

/*
Initalize the factory with the workspace. This allows top level decision on what
factory to use, but allows presenter/algorithms to pass in the
dataobjects (workspaces) to run against at a later time. If workspace is not an
IMDEventWorkspace, attempts to use any run-time successor set.
@Param ws : Workspace to use.
*/
void vtkMDHexFactory::initialize(const Mantid::API::Workspace_sptr &ws) {
  IMDEventWorkspace_sptr imdws = doInitialize<IMDEventWorkspace, 3>(ws, false);
  m_workspace = imdws;
}

/// Validate the current object.
void vtkMDHexFactory::validate() const {
  if (!m_workspace) {
    throw std::runtime_error("Invalid vtkMDHexFactory. Workspace is null");
  }
}

/** Sets the recursion depth to a specified level in the workspace.
 */
void vtkMDHexFactory::setRecursionDepth(size_t depth) { m_maxDepth = depth; }

/*
Set the time value.
*/
void vtkMDHexFactory::setTime(double time) { m_time = time; }
} // namespace VATES
} // namespace Mantid
