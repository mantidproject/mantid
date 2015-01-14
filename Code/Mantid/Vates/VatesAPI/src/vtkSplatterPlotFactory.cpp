#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ReadLock.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/Common.h"

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkVertex.h>
#include <vtkPoints.h>
#include <vtkPolyVertex.h>
#include <vtkSystemIncludes.h>
#include <vtkUnstructuredGrid.h>

#include <algorithm>
#include <boost/math/special_functions/fpclassify.hpp>
#include "qwt/qwt_double_interval.h"

using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::ReadLock;

namespace
{
/**
 * Comparator function to sort boxes in order of decreasing normalized signal.
 */
bool CompareNormalizedSignal( const IMDNode *box_1, const IMDNode *box_2 )
{
  double signal_1 = box_1->getSignalNormalized();
  double signal_2 = box_2->getSignalNormalized();
  return (signal_1 > signal_2);
}
}

namespace Mantid
{
namespace VATES
{
  /**
   * Constructor
   * @param thresholdRange : Threshold range strategy
   * @param scalarName : Name for scalar signal array.
   * @param numPoints : Total number of points to create.
   * @param percentToUse : Cutoff for the densest boxes.
   */
  vtkSplatterPlotFactory::vtkSplatterPlotFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t numPoints, const double percentToUse ) :
  m_thresholdRange(thresholdRange), m_scalarName(scalarName), 
  m_numPoints(numPoints), m_percentToUse(percentToUse),
  m_buildSortedList(true), m_wsName(""), dataSet(NULL),
  slice(false), sliceMask(NULL), sliceImplicitFunction(NULL),
  m_time(0.0),
  m_metaDataExtractor(new MetaDataExtractorUtils())
  {
  }

  /**
   * Destructor
   */
  vtkSplatterPlotFactory::~vtkSplatterPlotFactory()
  {
  }

  /**
   * Generate the vtkDataSet from the objects input MDEventWorkspace (of a
   * given type an dimensionality 3+)
   * @return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
   */
  template<typename MDE, size_t nd>
  void vtkSplatterPlotFactory::doCreate(typename MDEventWorkspace<MDE, nd>::sptr ws) const
  {
    bool VERBOSE = true;
    CPUTimer tim;
    // Acquire a scoped read-only lock to the workspace (prevent segfault
    // from algos modifying ws)
    ReadLock lock(*ws);

    // Find out how many events to plot, and the percentage of the largest
    // boxes to use.
    size_t totalPoints = ws->getNPoints();
    size_t numPoints = m_numPoints;

    if (numPoints > totalPoints)
    {
      numPoints = totalPoints;
    }

    double percent_to_use = m_percentToUse;
    // Fail safe limits on fraction of boxes to use
    if (percent_to_use <= 0)
    {
      percent_to_use = 5;
    }

    if (percent_to_use > 100)
    {
      percent_to_use = 100;
    }

    // First we get all the boxes, up to the given depth; with or wo the
    // slice function
    std::vector<API::IMDNode *> boxes;
    if (this->slice)
    {
      ws->getBox()->getBoxes(boxes, 1000, true, this->sliceImplicitFunction);
    }
    else
    {
      ws->getBox()->getBoxes(boxes, 1000, true);
    }

    if (VERBOSE)
    {
      std::cout << tim << " to retrieve the "<< boxes.size() << " boxes down."<< std::endl;
    }

    std::string new_name = ws->getName();
    if (new_name != m_wsName || m_buildSortedList)
    {
      m_wsName = new_name;
      m_buildSortedList = false;
      m_sortedBoxes.clear();
      // get list of boxes with signal > 0 and sort
      // the list in order of decreasing signal
      for (size_t i = 0; i < boxes.size(); i++)
      {
        MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
        if (box)
        {
          size_t newPoints = box->getNPoints();
          if (newPoints > 0)
          {
            m_sortedBoxes.push_back(box);
          }
        }
      }

      if (VERBOSE)
      {
        std::cout << "START SORTING" << std::endl;
      }
      std::sort(m_sortedBoxes.begin(), m_sortedBoxes.end(),
                CompareNormalizedSignal);
      if (VERBOSE)
      {
        std::cout << "DONE SORTING" << std::endl;
      }
    }
    size_t num_boxes_to_use = static_cast<size_t>(percent_to_use * static_cast<double>(m_sortedBoxes.size()) / 100.0);
    if (num_boxes_to_use >= m_sortedBoxes.size())
    {
      num_boxes_to_use = m_sortedBoxes.size()-1;
    }

    // restrict the number of points to the
    // number of points in boxes being used
    size_t total_points_available = 0;
    for (size_t i = 0; i < num_boxes_to_use; i++)
    {
      size_t newPoints = m_sortedBoxes[i]->getNPoints();
      total_points_available += newPoints;
    }

    if (numPoints > total_points_available)
    {
      numPoints = total_points_available;
    }

    size_t points_per_box = 0;
    // calculate the average number of points to use per box
    if (num_boxes_to_use > 0)
    {
      points_per_box = numPoints / num_boxes_to_use;
    }

    if (points_per_box < 1)
    {
      points_per_box = 1;
    }

    if (VERBOSE)
    {
      std::cout << "numPoints                 = " << numPoints << std::endl;
      std::cout << "num boxes in all          = " << boxes.size() << std::endl;
      std::cout << "num boxes above zero      = " << m_sortedBoxes.size() << std::endl;
      std::cout << "num boxes to use          = " << num_boxes_to_use << std::endl;
      std::cout << "total_points_available    = " << total_points_available << std::endl;
      std::cout << "points needed per box     = " << points_per_box << std::endl;
    }

    // First save the events and signals that we actually use.
    // For each box, get up to the average number of points
    // we want from each box, limited by the number of points
    // in the box.  NOTE: since boxes have different numbers
    // of events, we will not get all the events requested.
    // Also, if we are using a smaller number of points, we
    // won't get points from some of the boxes with lower signal.

    std::vector<float>            saved_signals;
    std::vector<const coord_t*>   saved_centers;
    std::vector<size_t>           saved_n_points_in_cell;
    saved_signals.reserve(numPoints);
    saved_centers.reserve(numPoints);
    saved_n_points_in_cell.reserve(numPoints);

    double maxSignalScalar = 0;
    double minSignalScalar = VTK_DOUBLE_MAX;

    size_t pointIndex = 0;
    size_t box_index  = 0;
    bool   done       = false;
    while (box_index < num_boxes_to_use && !done)
    {
      MDBox<MDE,nd> *box = dynamic_cast<MDBox<MDE,nd> *>(m_sortedBoxes[box_index]);
      box_index++;
      if (NULL == box)
      {
        continue;
      }
      float signal_normalized = static_cast<float>(box->getSignalNormalized());
      maxSignalScalar = maxSignalScalar > signal_normalized ? maxSignalScalar:signal_normalized;
      minSignalScalar = minSignalScalar > signal_normalized ? signal_normalized : minSignalScalar;
      size_t newPoints = box->getNPoints();
      size_t num_from_this_box = points_per_box;
      if (num_from_this_box > newPoints)
      {
        num_from_this_box = newPoints;
      }
      const std::vector<MDE> & events = box->getConstEvents();
      size_t startPointIndex = pointIndex;
      size_t event_index = 0;
      while (event_index < num_from_this_box && !done)
      {
        const MDE & ev = events[event_index];
        event_index++;
        const coord_t * center = ev.getCenter();
        // Save location
        saved_centers.push_back(center);
        pointIndex++;
        if (pointIndex >= numPoints)
        {
          done = true;
        }
      }
      box->releaseEvents();
      // Save signal
      saved_signals.push_back(signal_normalized);
      // Save cell size
      saved_n_points_in_cell.push_back(pointIndex-startPointIndex);
    } 

    numPoints = saved_centers.size();
    size_t numCells = saved_signals.size();

    if (VERBOSE)
    {
      std::cout << "Recorded data for all points" << std::endl;
      std::cout << "numPoints = " << numPoints << std::endl;
      std::cout << "numCells  = " << numCells << std::endl;
    }

    // Create the point list, one position for each point actually used
    vtkPoints *points = vtkPoints::New();
    points->Allocate(numPoints);
    points->SetNumberOfPoints(numPoints);

    // The list of IDs of points used, one ID per point, since points
    // are not reused to form polygon facets, etc.
    vtkIdType *ids = new vtkIdType[numPoints];

    // Only one scalar for each cell, NOT one per point
    vtkFloatArray *signal = vtkFloatArray::New();
    signal->Allocate(numCells);
    signal->SetName(m_scalarName.c_str());

    // Create the data set.  Need space for each cell, not for each point
    vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
    this->dataSet = visualDataSet;
    visualDataSet->Allocate(numCells);
    // Now copy the saved point, cell and signal info into vtk data structures
    pointIndex = 0;
    for (size_t cell_i = 0; cell_i < numCells; cell_i++)
    {
      size_t startPointIndex = pointIndex;
      for (size_t point_i = 0; point_i < saved_n_points_in_cell[cell_i]; point_i++)
      {
        points->SetPoint(pointIndex, saved_centers[pointIndex]);
        ids[pointIndex] = pointIndex;
        pointIndex++;
      }
      signal->InsertNextTuple1(saved_signals[cell_i]);
      visualDataSet->InsertNextCell(VTK_POLY_VERTEX, saved_n_points_in_cell[cell_i], ids+startPointIndex);
    }

    if (VERBOSE)
    {
      std::cout << tim << " to create " << pointIndex << " points." << std::endl;
    }

    // Shrink to fit
    //points->Squeeze();
    signal->Squeeze();
    visualDataSet->Squeeze();
    
    // Add points and scalars
    visualDataSet->SetPoints(points);
    visualDataSet->GetCellData()->SetScalars(signal);

    delete [] ids;
  }

  /**
   * Generate the vtkDataSet from the objects input MDHistoWorkspace (of a
   * given type an dimensionality 3D or 4D). Note that for 4D we only look at t=0 currently.
   * Note that this implementation is almost the same as for vtkMDHistoHexFactory.
   * @param workspace A smart pointer to the histo workspace.
   * @return A fully constructed vtkUnstructuredGrid containing geometric and scalar data.
   */
  void vtkSplatterPlotFactory::doCreateMDHisto(IMDHistoWorkspace_sptr workspace) const
  {
    // Acquire a scoped read-only lock to the workspace (prevent segfault
    // from algos modifying wworkspace)
    ReadLock lock(*workspace);

    // Get the geometric information of the bins
    const int nBinsX = static_cast<int>(workspace->getXDimension()->getNBins());
    const int nBinsY = static_cast<int>(workspace->getYDimension()->getNBins());
    const int nBinsZ = static_cast<int>(workspace->getZDimension()->getNBins());

    const coord_t maxX = workspace->getXDimension()->getMaximum();
    const coord_t minX = workspace->getXDimension()->getMinimum();
    const coord_t maxY = workspace->getYDimension()->getMaximum();
    const coord_t minY = workspace->getYDimension()->getMinimum();
    const coord_t maxZ = workspace->getZDimension()->getMaximum();
    const coord_t minZ = workspace->getZDimension()->getMinimum();

    coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
    coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);
    coord_t incrementZ = (maxZ - minZ) / static_cast<coord_t>(nBinsZ);
  
    const int imageSize = (nBinsX)*(nBinsY)*(nBinsZ);

    // VTK structures
    vtkFloatArray *signal = vtkFloatArray::New();
    signal->Allocate(imageSize);
    signal->SetName(m_scalarName.c_str());
    signal->SetNumberOfComponents(1);

    vtkPoints *points = vtkPoints::New();
    points->Allocate(static_cast<int>(imageSize));

    // Set up the actual vtkDataSet, here the vtkUnstructuredGrid, the cell type 
    // we choose here is the vtk_poly_vertex
    vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
    this->dataSet = visualDataSet;
    visualDataSet->Allocate(imageSize);
    
    // Create the vertex structure.
    vtkVertex* vertex = vtkVertex::New();

    // Check if the workspace requires 4D handling.
    bool do4D = doMDHisto4D(workspace);

    // Get the transformation that takes the points in the TRANSFORMED space back into the ORIGINAL (not-rotated) space.
    Mantid::API::CoordTransform* transform = NULL;
    if (m_useTransform)
    {
     transform = workspace->getTransformToOriginal();
    }

    Mantid::coord_t in[3]; 
    Mantid::coord_t out[3];

    signal_t signalScalar;

    size_t index = 0;

    for (int z = 0; z < nBinsZ; z++)
    {
      in[2] = (minZ + (static_cast<coord_t>(z)*incrementZ + static_cast<coord_t>(0.5)*incrementZ)); 
      for (int y = 0; y < nBinsY; y++)
      {
        in[1] = (minY + (static_cast<coord_t>(y)*incrementY + static_cast<coord_t>(0.5)*incrementY)); 
        for (int x = 0; x < nBinsX; x++)
        {
          // Get the signalScalar
          signalScalar = this->extractScalarSignal(workspace, do4D, x, y, z);

          // Make sure that the signal is not bad and is in the range and larger than 0
          if (!Mantid::VATES::isSpecial(static_cast<double>(signalScalar)) && m_thresholdRange->inRange(signalScalar) && (signalScalar > static_cast<signal_t>(0.0)))
          {
            in[0] = (minX + (static_cast<coord_t>(x) * incrementX + static_cast<coord_t>(0.5)*incrementX)); 

            // Create the transformed value if required
            if (transform)
            {
              transform->apply(in, out);
            }
            else
            {
              memcpy(&out, &in, sizeof in);
            }

            // Store the signal
            signal->InsertNextValue(static_cast<float>(signalScalar));

            vtkIdType id = points->InsertNextPoint(out);
              
            vertex->GetPointIds()->SetId(0,id);
              
            visualDataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());
          }
          index++;
        }
      }
    }
    
    vertex->Delete();

    visualDataSet->SetPoints(points);
    visualDataSet->GetCellData()->SetScalars(signal);

    points->Delete();
    signal->Delete();
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
  signal_t vtkSplatterPlotFactory::extractScalarSignal(IMDHistoWorkspace_sptr workspace,
                                                     bool do4D, const int x, const int y, const int z) const
  {
    signal_t signalScalar;

    if (do4D)
    {
      signalScalar = workspace->getSignalNormalizedAt(static_cast<size_t>(x),static_cast<size_t>(y),static_cast<size_t>(z), static_cast<size_t>(m_time));
    }
    else
    {
      signalScalar = workspace->getSignalNormalizedAt(static_cast<size_t>(x),static_cast<size_t>(y),static_cast<size_t>(z));
    }

    return signalScalar;
  }

  /**
   * Check if the MDHisto workspace is 3D or 4D in nature
   * @param workspace The MDHisto workspace
   * @returns Is the workspace 4D?
   */
  bool vtkSplatterPlotFactory::doMDHisto4D(IMDHistoWorkspace_sptr workspace) const
  {
    bool do4D = false;
    
    bool bExactMatch = true;
 
    IMDHistoWorkspace_sptr workspace4D = castAndCheck<IMDHistoWorkspace, 4>(workspace, bExactMatch); 
    
    if (workspace4D)
    {
      do4D = true;
    }
    
    return do4D;
  }


  /**
   * Generate the vtkDataSet from the objects input IMDEventWorkspace
   * @param progressUpdating : Reporting object to pass progress information up the stack.
   * @return fully constructed vtkDataSet.
   */
  vtkDataSet* vtkSplatterPlotFactory::create(ProgressAction& progressUpdating) const
  {
    UNUSED_ARG(progressUpdating);

    // If initialize() wasn't run, we don't have a workspace.
    if(!m_workspace)
    {
      throw std::runtime_error("Invalid vtkSplatterPlotFactory. Workspace is null");
    }

    size_t nd = m_workspace->getNumDims();
     
    Mantid::Kernel::ReadLock lock(*m_workspace);
    if (nd > 3)
    {
      // Slice from >3D down to 3D
      this->slice = true;
      this->sliceMask = new bool[nd];
      this->sliceImplicitFunction = new MDImplicitFunction();
      // Make the mask of dimensions
      // TODO: Smarter mapping
      for (size_t d = 0; d < nd; d++)
        this->sliceMask[d] = (d < 3);

      // Define where the slice is in 4D
      // TODO: Where to slice? Right now is just 0
      std::vector<coord_t> point(nd, 0);
      point[3] = coord_t(m_time); //Specifically for 4th/time dimension.

      // Define two opposing planes that point in all higher dimensions
      std::vector<coord_t> normal1(nd, 0);
      std::vector<coord_t> normal2(nd, 0);
      for (size_t d = 3; d < nd; d++)
      {
        normal1[d] = +1.0;
        normal2[d] = -1.0;
      }
      // This creates a 0-thickness region to slice in.
      sliceImplicitFunction->addPlane(MDPlane(normal1, point));
      sliceImplicitFunction->addPlane(MDPlane(normal2, point));
    }
    else
    {
      // Direct 3D, so no slicing
      this->slice = false;
    }

    // Macro to call the right instance of the
    CALL_MDEVENT_FUNCTION(this->doCreate, m_workspace);


    // Set the instrument
    m_instrument = m_metaDataExtractor->extractInstrument(m_workspace);
    double* range = NULL;

    if (dataSet)
    {
      range = dataSet->GetScalarRange();
    }

    if (range)
    {
      m_minValue = range[0];
      m_maxValue = range[1];
    }


    // Check for the workspace type, i.e. if it is MDHisto or MDEvent
    IMDEventWorkspace_sptr eventWorkspace = boost::dynamic_pointer_cast<IMDEventWorkspace>(m_workspace);
    IMDHistoWorkspace_sptr histoWorkspace = boost::dynamic_pointer_cast<IMDHistoWorkspace>(m_workspace);

    if (eventWorkspace)
    {
      // Macro to call the right instance of the
      CALL_MDEVENT_FUNCTION(this->doCreate, eventWorkspace);
    }
    else 
    {
      this->doCreateMDHisto(histoWorkspace);
    }

    // Clean up
    if (this->slice)
    {
      delete[] this->sliceMask;
      delete this->sliceImplicitFunction;
    }

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
  void vtkSplatterPlotFactory::initialize(Mantid::API::Workspace_sptr ws)
  {
    this->m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(ws);
    this->validate();
  }

  /**
   * Validate the current object.
   */
  void vtkSplatterPlotFactory::validate() const
  { 
    if(!m_workspace)
    {
      throw std::invalid_argument("Workspace is null or not IMDEventWorkspace");
    }

    if (m_workspace->getNumDims() < 3)
    {
      throw std::runtime_error("Invalid vtkSplatterPlotFactory. Workspace must have at least 3 dimensions.");
    }

    // Make sure that the workspace is either an MDEvent Workspace or an MDHistoWorkspace
    IMDEventWorkspace_sptr eventWorkspace = boost::dynamic_pointer_cast<IMDEventWorkspace>(m_workspace);
    IMDHistoWorkspace_sptr histoWorkspace = boost::dynamic_pointer_cast<IMDHistoWorkspace>(m_workspace);
    
    if (!eventWorkspace && !histoWorkspace)
    {
      throw std::runtime_error("Workspace is neither an IMDHistoWorkspace nor an IMDEventWorkspace.");
    }
  }

  /**
   * Sets the number of points to show
   * @param points : The total number of points to plot.
   */
  void vtkSplatterPlotFactory::SetNumberOfPoints(size_t points)
  {
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
  void vtkSplatterPlotFactory::SetPercentToUse(double percentToUse)
  {
    if (percentToUse <= 0)
    {
      m_percentToUse = 5;
    }
    else if (percentToUse > 100)
    {
      m_percentToUse = 100;
    }
    else 
    {
      m_percentToUse = percentToUse;
    }
  }

  /**
   * Set the time value.
   * @param time : the time
   */
  void vtkSplatterPlotFactory::setTime(double time)
  {
    if (m_time != time)
    {
      m_buildSortedList = true;
    }
    m_time = time;
  }

    /**
    * Getter for the minimum value;
    * @return The minimum value of the data set.
    */
  double vtkSplatterPlotFactory::getMinValue()
  {
    return m_minValue;
  }

  /**
  * Getter for the maximum value;
  * @return The maximum value of the data set.
  */
  double vtkSplatterPlotFactory::getMaxValue()
  {
    return m_maxValue;
  }

  /**
  * Getter for the instrument.
  * @returns The name of the instrument which is associated with the workspace.
  */
  const std::string& vtkSplatterPlotFactory::getInstrument()
  {
    return m_instrument;
  }
}
}
