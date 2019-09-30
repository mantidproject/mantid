// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ReadLock.h"

#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"

#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyVertex.h>
#include <vtkSystemIncludes.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertex.h>

#include "boost/algorithm/clamp.hpp"

#include <algorithm>
#include <iterator>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::ReadLock;

namespace Mantid {
namespace VATES {
/**
 * Constructor
 * @param scalarName : Name for scalar signal array.
 * @param numPoints : Total number of points to create.
 * @param percentToUse : Cutoff for the densest boxes.
 */
vtkSplatterPlotFactory::vtkSplatterPlotFactory(const std::string &scalarName,
                                               const size_t numPoints,
                                               const double percentToUse)
    : m_scalarName(scalarName), m_numPoints(numPoints), m_buildSortedList(true),
      m_wsName(""), slice(false), m_time(0.0),
      m_metaDataExtractor(new MetaDataExtractorUtils()),
      m_metadataJsonManager(new MetadataJsonManager()),
      m_vatesConfigurations(new VatesConfigurations()) {
  this->SetPercentToUse(percentToUse);
}

/**
 * Destructor
 */
vtkSplatterPlotFactory::~vtkSplatterPlotFactory() {}

/**
 * Generate the vtkDataSet from the objects input MDEventWorkspace (of a
 * given type an dimensionality 3+)
 * @return a fully constructed vtkUnstructuredGrid containing geometric and
 * scalar data.
 */
template <typename MDE, size_t nd>
void vtkSplatterPlotFactory::doCreate(
    typename MDEventWorkspace<MDE, nd>::sptr ws) const {
  bool VERBOSE = true;
  CPUTimer tim;
  // Acquire a scoped read-only lock to the workspace (prevent segfault
  // from algos modifying ws)
  ReadLock lock(*ws);

  // Find out how many events to plot, and the percentage of the largest
  // boxes to use.
  size_t numPoints =
      std::min(m_numPoints, static_cast<std::size_t>(ws->getNPoints()));

  std::string new_name = ws->getName();
  if (new_name != m_wsName || m_buildSortedList) {
    // First we get all the boxes, up to the given depth; with or wo the
    // slice function

    m_sortedBoxes.clear();
    if (this->slice) {
      ws->getBox()->getBoxes(m_sortedBoxes, 1000, true,
                             this->sliceImplicitFunction.get());
    } else {
      ws->getBox()->getBoxes(m_sortedBoxes, 1000, true);
    }

    if (VERBOSE) {
      std::cout << tim << " to retrieve the " << m_sortedBoxes.size()
                << " boxes down.\n";
    }

    m_wsName = new_name;
    m_buildSortedList = false;
    // get list of boxes with signal > 0 and sort
    // the list in order of decreasing signal
    m_sortedBoxes.erase(std::remove_if(m_sortedBoxes.begin(),
                                       m_sortedBoxes.end(),
                                       [](const API::IMDNode *box) {
                                         return !box || box->getNPoints() == 0;
                                       }),
                        m_sortedBoxes.end());
    this->sortBoxesByDecreasingSignal(VERBOSE);
  }
  size_t num_boxes_to_use = static_cast<size_t>(
      m_percentToUse * static_cast<double>(m_sortedBoxes.size()) / 100.0);
  if (num_boxes_to_use > 0 && num_boxes_to_use >= m_sortedBoxes.size()) {
    num_boxes_to_use = m_sortedBoxes.size() - 1;
  }

  // restrict the number of points to the
  // number of points in boxes being used
  size_t total_points_available = std::accumulate(
      m_sortedBoxes.begin(), std::next(m_sortedBoxes.begin(), num_boxes_to_use),
      size_t{0}, [](size_t value, const Mantid::API::IMDNode *box) {
        return value + box->getNPoints();
      });

  numPoints = std::min(numPoints, total_points_available);

  size_t points_per_box = 0;
  // calculate the average number of points to use per box
  if (num_boxes_to_use > 0) {
    points_per_box = numPoints / num_boxes_to_use;
  }

  points_per_box = std::max(points_per_box, size_t{1});

  if (VERBOSE) {
    std::cout << "numPoints                 = " << numPoints << '\n';
    std::cout << "num boxes above zero      = " << m_sortedBoxes.size() << '\n';
    std::cout << "num boxes to use          = " << num_boxes_to_use << '\n';
    std::cout << "total_points_available    = " << total_points_available
              << '\n';
    std::cout << "points needed per box     = " << points_per_box << '\n';
  }

  // Save the events and signals that we actually use.
  // For each box, get up to the average number of points
  // we want from each box, limited by the number of points
  // in the box.  NOTE: since boxes have different numbers
  // of events, we will not get all the events requested.
  // Also, if we are using a smaller number of points, we
  // won't get points from some of the boxes with lower signal.

  // Create the point list, one position for each point actually used
  vtkNew<vtkPoints> points;
  vtkFloatArray *pointsArray = vtkFloatArray::FastDownCast(points->GetData());
  if (!pointsArray) {
    throw std::runtime_error("Failed to cast vtkDataArray to vtkFloatArray.");
  }
  float *points_ptr = pointsArray->WritePointer(0, numPoints * 3);

  // One scalar for each point
  vtkNew<vtkFloatArray> signal;
  signal->SetName(m_scalarName.c_str());
  float *signal_ptr = signal->WritePointer(0, numPoints);

  // Create the data set.
  auto visualDataSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->dataSet = visualDataSet;

  size_t pointIndex = 0;
  for (size_t box_index = 0; box_index < num_boxes_to_use; ++box_index) {
    MDBox<MDE, nd> *box =
        dynamic_cast<MDBox<MDE, nd> *>(m_sortedBoxes[box_index]);
    if (box) {
      size_t num_from_this_box =
          std::min(points_per_box, static_cast<size_t>(box->getNPoints()));
      pointIndex += num_from_this_box;
      // verify there are never more than numPoints.
      if (pointIndex > numPoints) {
        num_from_this_box -= pointIndex - numPoints;
        pointIndex = numPoints;
      }
      // Save signal
      float signal_normalized = static_cast<float>(box->getSignalNormalized());
      signal_ptr =
          std::fill_n(signal_ptr, num_from_this_box, signal_normalized);

      const std::vector<MDE> &events = box->getConstEvents();
      for (size_t event_index = 0; event_index < num_from_this_box;
           ++event_index) {
        const MDE &ev = events[event_index];
        // Save location
        points_ptr = std::copy_n(ev.getCenter(), 3, points_ptr);
      }
      box->releaseEvents();
    }
  }

  if (VERBOSE) {
    std::cout << "Recorded data for all points\n";
    std::cout << "numPoints = " << numPoints << '\n';
    std::cout << tim << " to create " << pointIndex << " points.\n";
  }

  pointsArray->Resize(pointIndex);
  signal->Resize(pointIndex);
  // Add points and scalars
  visualDataSet->SetPoints(points.GetPointer());
  visualDataSet->GetPointData()->SetScalars(signal.GetPointer());
  visualDataSet->GetCellData()->SetScalars(signal.GetPointer());

  visualDataSet->Allocate(points->GetNumberOfPoints());
  for (vtkIdType ptId = 0; ptId < points->GetNumberOfPoints(); ++ptId) {
    visualDataSet->InsertNextCell(VTK_VERTEX, 1, &ptId);
  }
}

/**
 * Sort the boxes by their normalized signal in decreasing order
 * @param VERBOSE : if true then print when sorting happens
 */
void vtkSplatterPlotFactory::sortBoxesByDecreasingSignal(
    const bool VERBOSE) const {
  if (VERBOSE) {
    std::cout << "START SORTING\n";
  }

  std::sort(m_sortedBoxes.begin(), m_sortedBoxes.end(),
            [](IMDNode *box_1, IMDNode *box_2) {
              return box_1->getSignalNormalized() >
                     box_2->getSignalNormalized();
            });

  if (VERBOSE) {
    std::cout << "DONE SORTING\n";
  }
}

/**
 * Generate the vtkDataSet from the objects input MDHistoWorkspace (of a
 * given type an dimensionality 3D or 4D). Note that for 4D we only look at t=0
 * currently.
 * Note that this implementation is almost the same as for vtkMDHistoHexFactory.
 * @param workspace A smart pointer to the histo workspace.
 * @return A fully constructed vtkUnstructuredGrid containing geometric and
 * scalar data.
 */
void vtkSplatterPlotFactory::doCreateMDHisto(
    const IMDHistoWorkspace &workspace) const {
  // Acquire a scoped read-only lock to the workspace (prevent segfault
  // from algos modifying wworkspace)
  ReadLock lock(workspace);

  // Get the geometric information of the bins
  const int nBinsX = static_cast<int>(workspace.getXDimension()->getNBins());
  const int nBinsY = static_cast<int>(workspace.getYDimension()->getNBins());
  const int nBinsZ = static_cast<int>(workspace.getZDimension()->getNBins());

  const coord_t maxX = workspace.getXDimension()->getMaximum();
  const coord_t minX = workspace.getXDimension()->getMinimum();
  const coord_t maxY = workspace.getYDimension()->getMaximum();
  const coord_t minY = workspace.getYDimension()->getMinimum();
  const coord_t maxZ = workspace.getZDimension()->getMaximum();
  const coord_t minZ = workspace.getZDimension()->getMinimum();

  coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
  coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);
  coord_t incrementZ = (maxZ - minZ) / static_cast<coord_t>(nBinsZ);

  const int imageSize = (nBinsX) * (nBinsY) * (nBinsZ);

  // VTK structures
  vtkNew<vtkFloatArray> signal;
  signal->Allocate(imageSize);
  signal->SetName(m_scalarName.c_str());
  signal->SetNumberOfComponents(1);

  vtkNew<vtkPoints> points;
  points->Allocate(static_cast<int>(imageSize));

  // Set up the actual vtkDataSet, here the vtkUnstructuredGrid, the cell type
  // we choose here is the vtk_poly_vertex
  auto visualDataSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->dataSet = visualDataSet;
  visualDataSet->Allocate(imageSize);

  // Create the vertex structure.
  vtkNew<vtkVertex> vertex;

  // Check if the workspace requires 4D handling.
  bool do4D = doMDHisto4D(&workspace);

  // Get the transformation that takes the points in the TRANSFORMED space back
  // into the ORIGINAL (not-rotated) space.
  Mantid::API::CoordTransform const *transform = nullptr;
  if (m_useTransform) {
    transform = workspace.getTransformToOriginal();
  }

  Mantid::coord_t in[3];
  Mantid::coord_t out[3];

  for (int z = 0; z < nBinsZ; z++) {
    in[2] = (minZ + (static_cast<coord_t>(z) + 0.5f) * incrementZ);
    for (int y = 0; y < nBinsY; y++) {
      in[1] = (minY + (static_cast<coord_t>(y) + 0.5f) * incrementY);
      for (int x = 0; x < nBinsX; x++) {
        // Get the signalScalar
        signal_t signalScalar =
            this->extractScalarSignal(workspace, do4D, x, y, z);

        // Make sure that the signal is not bad and is in the range and larger
        // than 0
        if (std::isfinite(signalScalar) &&
            (signalScalar > static_cast<signal_t>(0.0))) {
          in[0] = (minX + (static_cast<coord_t>(x) + 0.5f) * incrementX);
          // Create the transformed value if required
          if (transform) {
            transform->apply(in, out);
          } else {
            memcpy(&out, &in, sizeof in);
          }

          // Store the signal
          signal->InsertNextValue(static_cast<float>(signalScalar));

          vtkIdType id = points->InsertNextPoint(out);

          vertex->GetPointIds()->SetId(0, id);

          visualDataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());
        }
      }
    }
  }

  visualDataSet->SetPoints(points.GetPointer());
  visualDataSet->GetCellData()->SetScalars(signal.GetPointer());
  visualDataSet->Squeeze();
}

/**
 * Set the signals, pointIDs and points for bins which are valid to be displayed
 * @param workspace Smart pointer to the MDHisto workspace.
 * @param do4D If the workspace contains time.
 * @param x The x coordinate.
 * @param y The y coordinate.
 * @param z The z coordinate.
 * @returns The scalar signal.
 */
signal_t
vtkSplatterPlotFactory::extractScalarSignal(const IMDHistoWorkspace &workspace,
                                            bool do4D, const int x, const int y,
                                            const int z) const {
  signal_t signalScalar;

  if (do4D) {
    signalScalar = workspace.getSignalNormalizedAt(
        static_cast<size_t>(x), static_cast<size_t>(y), static_cast<size_t>(z),
        static_cast<size_t>(m_time));
  } else {
    signalScalar = workspace.getSignalNormalizedAt(
        static_cast<size_t>(x), static_cast<size_t>(y), static_cast<size_t>(z));
  }

  return signalScalar;
}

/**
 * Check if the MDHisto workspace is 3D or 4D in nature
 * @param workspace The MDHisto workspace
 * @returns Is the workspace 4D?
 */
bool vtkSplatterPlotFactory::doMDHisto4D(
    const IMDHistoWorkspace *workspace) const {
  bool bExactMatch = true;
  if (workspace &&
      checkWorkspace<IMDHistoWorkspace, 4>(*workspace, bExactMatch)) {
    return true;
  }
  return false;
}

/**
 * Generate the vtkDataSet from the objects input IMDEventWorkspace
 * @param progressUpdating : Reporting object to pass progress information up
 * the stack.
 * @return fully constructed vtkDataSet.
 */
vtkSmartPointer<vtkDataSet>
vtkSplatterPlotFactory::create(ProgressAction &progressUpdating) const {
  UNUSED_ARG(progressUpdating);

  // If initialize() wasn't run, we don't have a workspace.
  if (!m_workspace) {
    throw std::runtime_error(
        "Invalid vtkSplatterPlotFactory. Workspace is null");
  }

  size_t nd = m_workspace->getNumDims();

  Mantid::Kernel::ReadLock lock(*m_workspace);
  if (nd > 3) {
    // Slice from >3D down to 3D
    this->slice = true;
    this->sliceMask = std::make_unique<bool[]>(nd);
    this->sliceImplicitFunction = boost::make_shared<MDImplicitFunction>();
    // Make the mask of dimensions
    // TODO: Smarter mapping
    for (size_t d = 0; d < nd; d++)
      this->sliceMask[d] = (d < 3);

    // Define where the slice is in 4D
    // TODO: Where to slice? Right now is just 0
    std::vector<coord_t> point(nd, 0);
    point[3] = coord_t(m_time); // Specifically for 4th/time dimension.

    // Define two opposing planes that point in all higher dimensions
    std::vector<coord_t> normal1(nd, 0);
    std::vector<coord_t> normal2(nd, 0);
    for (size_t d = 3; d < nd; d++) {
      normal1[d] = +1.0;
      normal2[d] = -1.0;
    }
    // This creates a 0-thickness region to slice in.
    sliceImplicitFunction->addPlane(MDPlane(normal1, point));
    sliceImplicitFunction->addPlane(MDPlane(normal2, point));
  } else {
    // Direct 3D, so no slicing
    this->slice = false;
  }

  // Macro to call the right instance of the
  CALL_MDEVENT_FUNCTION(this->doCreate, m_workspace);

  // Set the instrument
  m_instrument = m_metaDataExtractor->extractInstrument(m_workspace.get());

  // Check for the workspace type, i.e. if it is MDHisto or MDEvent
  IMDEventWorkspace_sptr eventWorkspace =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(m_workspace);
  IMDHistoWorkspace_sptr histoWorkspace =
      boost::dynamic_pointer_cast<IMDHistoWorkspace>(m_workspace);

  if (eventWorkspace) {
    // Macro to call the right instance of the
    CALL_MDEVENT_FUNCTION(this->doCreate, eventWorkspace);
  } else {
    this->doCreateMDHisto(*histoWorkspace);
  }

  // Add metadata in json format
  this->addMetadata();

  // The macro does not allow return calls, so we used a member variable.
  return this->dataSet;
}

/**
 * Initalize the factory with the workspace. This allows top level decision
 * on what factory to use, but allows presenter/algorithms to pass in the
 * dataobjects (workspaces) to run against at a later time. If workspace is
 * not an IMDEventWorkspace, throws an invalid argument exception.
 * @param ws : Workspace to use.
 */
void vtkSplatterPlotFactory::initialize(const Mantid::API::Workspace_sptr &ws) {
  this->m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(ws);
  this->validate();
}

/**
 * Validate the current object.
 */
void vtkSplatterPlotFactory::validate() const {
  if (!m_workspace) {
    throw std::invalid_argument("Workspace is null or not IMDEventWorkspace");
  }

  if (m_workspace->getNumDims() < 3) {
    throw std::runtime_error("Invalid vtkSplatterPlotFactory. Workspace must "
                             "have at least 3 dimensions.");
  }

  // Make sure that the workspace is either an MDEvent Workspace or an
  // MDHistoWorkspace
  IMDEventWorkspace_sptr eventWorkspace =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(m_workspace);
  IMDHistoWorkspace_sptr histoWorkspace =
      boost::dynamic_pointer_cast<IMDHistoWorkspace>(m_workspace);

  if (!eventWorkspace && !histoWorkspace) {
    throw std::runtime_error(
        "Workspace is neither an IMDHistoWorkspace nor an IMDEventWorkspace.");
  }
}

/**
 * Add meta data to the visual data set.
 */
void vtkSplatterPlotFactory::addMetadata() const {
  if (this->dataSet) {
    m_metadataJsonManager->setInstrument(
        m_metaDataExtractor->extractInstrument(m_workspace.get()));
    m_metadataJsonManager->setSpecialCoordinates(
        static_cast<int>(m_workspace->getSpecialCoordinateSystem()));

    // Append metadata
    std::string jsonString = m_metadataJsonManager->getSerializedJson();
    vtkNew<vtkFieldData> outputFD;

    // Add metadata to dataset.
    MetadataToFieldData convert;
    convert(outputFD.GetPointer(), jsonString,
            m_vatesConfigurations->getMetadataIdJson());
    dataSet->SetFieldData(outputFD.GetPointer());
  }
}

/**
 * Write the xml metadata from the underlying source into the vktArray of the
 * @param fieldData The field data from the underlying source
 * @param dataSet The splatterplot data set.
 */
void vtkSplatterPlotFactory::setMetadata(vtkFieldData *fieldData,
                                         vtkDataSet *dataSet) {
  // Extract the xml-metadata part of the fieldData and the json-metadata from
  // the dataset
  FieldDataToMetadata convertFtoM;
  std::string xmlString = convertFtoM(fieldData, XMLDefinitions::metaDataId());
  std::string jsonString = convertFtoM(
      dataSet->GetFieldData(), m_vatesConfigurations->getMetadataIdJson());

  // Create a new field data array
  MetadataToFieldData convertMtoF;
  vtkNew<vtkFieldData> outputFD;
  outputFD->ShallowCopy(fieldData);
  convertMtoF(outputFD.GetPointer(), xmlString, XMLDefinitions::metaDataId());
  convertMtoF(outputFD.GetPointer(), jsonString,
              m_vatesConfigurations->getMetadataIdJson());
  dataSet->SetFieldData(outputFD.GetPointer());
}

/**
 * Sets the number of points to show
 * @param points : The total number of points to plot.
 */
void vtkSplatterPlotFactory::SetNumberOfPoints(size_t points) {
  m_numPoints = points;
}

/**
 * Set the size of the initial portion of the sorted list of boxes that
 * will will be used when getting events to plot as points.
 *
 * @param percentToUse : The portion of the list to use, as a percentage.
 *                       NOTE: This must be more than 0 and no more than 100
 *                       and whatever value is passed in will be restricted
 *                       to the interval (0,100].
 */
void vtkSplatterPlotFactory::SetPercentToUse(double percentToUse) {
  m_percentToUse = boost::algorithm::clamp(
      percentToUse, std::numeric_limits<double>::min(), 100.0);
}

/**
 * Set the time value.
 * @param time : the time
 */
void vtkSplatterPlotFactory::setTime(double time) {
  if (m_time != time) {
    m_buildSortedList = true;
  }
  m_time = time;
}

/**
 * Getter for the instrument.
 * @returns The name of the instrument which is associated with the workspace.
 */
const std::string &vtkSplatterPlotFactory::getInstrument() {
  return m_instrument;
}
} // namespace VATES
} // namespace Mantid
