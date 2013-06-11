#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/ProgressAction.h"

#include <algorithm>

#include <boost/math/special_functions/fpclassify.hpp>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include "vtkSystemIncludes.h"
#include "MantidKernel/ReadLock.h"

using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::ReadLock;


namespace
{
  // ----------------------------------------------------------------------------------------------
  /*
   * Comparator function to sort boxes in order of decreasing normalized signal.
   */
  bool CompareNormalizedSignal( const IMDNode * box_1, const IMDNode * box_2 )
  {
    double signal_1 = box_1->getSignalNormalized();
    double signal_2 = box_2->getSignalNormalized();
    return ( signal_1 > signal_2 );
  }
}


namespace Mantid
{
  namespace VATES
  {
  
  /*Constructor
  @Param thresholdRange : Threshold range strategy
  @scalarName : Name for scalar signal array.
  */
  vtkSplatterPlotFactory::vtkSplatterPlotFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t numPoints, const double percentToUse ) :
  m_thresholdRange(thresholdRange), m_scalarName(scalarName), 
  m_numPoints(numPoints), m_percentToUse(percentToUse)
  {
  }

  /// Destructor
  vtkSplatterPlotFactory::~vtkSplatterPlotFactory()
  {
  }


  //-------------------------------------------------------------------------------------------------
  /* Generate the vtkDataSet from the objects input MDEventWorkspace (of a given type an dimensionality 3+)
   *
   * @return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
  */
  template<typename MDE, size_t nd>
  void vtkSplatterPlotFactory::doCreate(typename MDEventWorkspace<MDE, nd>::sptr ws) const
  {
    bool VERBOSE = true;
    CPUTimer tim;
    // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
    ReadLock lock(*ws);

    // Find out how many events to plot, and the percentage of the largest boxes to use.
    size_t totalPoints = ws->getNPoints();
    size_t numPoints = m_numPoints;
    if ( numPoints > totalPoints )
    {
      numPoints = totalPoints;
    }

    double percent_to_use = m_percentToUse;  
    if ( percent_to_use <= 0 )                   // fail safe limits on fraction of boxes to use
    {
      percent_to_use = 5;
    }

    if ( percent_to_use > 100 )
    {
      percent_to_use = 100;
    }

    // First we get all the boxes, up to the given depth; with or wo the slice function
    std::vector<API::IMDNode *> boxes;
    if (this->slice)
      ws->getBox()->getBoxes(boxes, 1000, true, this->sliceImplicitFunction);
    else
      ws->getBox()->getBoxes(boxes, 1000, true);

    if (VERBOSE) std::cout << tim <<" to retrieve the "<< boxes.size() <<" boxes down."<< std::endl;

                                                // get list of boxes with signal > 0 and sort
                                                // the list in order of decreasing signal
    std::vector< MDBox<MDE,nd> * > sorted_boxes;
    sorted_boxes.reserve( 100000 );
    for ( size_t i = 0; i < boxes.size(); i++ )
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
      if (box)
      {
        size_t newPoints = box->getNPoints();
        if (newPoints > 0)
        {
          sorted_boxes.push_back( box );
        } // box has any points
      } // box is valid MDBox
    } // For each box

    if ( VERBOSE ) std::cout << "START SORTING" << std::endl;
    std::sort( sorted_boxes.begin(), sorted_boxes.end(), CompareNormalizedSignal );
    if ( VERBOSE ) std::cout << "DONE SORTING" << std::endl;

    size_t num_boxes_to_use = (size_t)(percent_to_use * (double)sorted_boxes.size() / 100.0);
    if ( num_boxes_to_use <= 0 )
    {
      num_boxes_to_use = 1;
    }

    if ( num_boxes_to_use >= sorted_boxes.size() )
    {
      num_boxes_to_use = sorted_boxes.size()-1;
    }

                                            // restrict the number of points to the
                                            // number of points in boxes being used
    size_t total_points_available = 0;
    for ( size_t i = 0; i < num_boxes_to_use; i++ )
    {
      size_t newPoints = sorted_boxes[ i ]->getNPoints();
      total_points_available += newPoints;
    }

    if ( numPoints > total_points_available )
    {
      numPoints = total_points_available;
    }

    size_t points_per_box = 0;             // calculate the average number of points to
    if ( num_boxes_to_use > 0 )            // to use per box
      points_per_box = numPoints / num_boxes_to_use;

    if ( points_per_box < 1 )
      points_per_box = 1;

    if ( VERBOSE )
    {
      std::cout << "numPoints                 = " << numPoints << std::endl;
      std::cout << "num boxes in all          = " << boxes.size() << std::endl;
      std::cout << "num boxes above zero      = " << sorted_boxes.size() << std::endl;
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
    saved_signals.reserve( numPoints );
    saved_centers.reserve( numPoints );
    saved_n_points_in_cell.reserve( numPoints );
  
    size_t pointIndex = 0;
    size_t box_index  = 0;
    bool   done       = false;
    while ( box_index < num_boxes_to_use && !done )                   // "For" each box
    {
      MDBox<MDE,nd> * box = sorted_boxes[box_index];
      box_index++;
      float signal_normalized = float(box->getSignalNormalized());
      size_t newPoints = box->getNPoints();
      size_t num_from_this_box = points_per_box;
      if ( num_from_this_box > newPoints )
      {
        num_from_this_box = newPoints;
      }
      const std::vector<MDE> & events = box->getConstEvents();
      size_t startPointIndex = pointIndex;
      size_t event_index = 0;
      while ( event_index < num_from_this_box && !done )              // "For" each event we
      {                                                               // get from this box
        const MDE & ev = events[ event_index ];
        event_index++;
        const coord_t * center = ev.getCenter();  
        saved_centers.push_back( center );                            // save location
        pointIndex++;
        if ( pointIndex >= numPoints )
        {
          done = true;
        }
      }
      box->releaseEvents();
      saved_signals.push_back( signal_normalized );                   // save signal
      saved_n_points_in_cell.push_back( pointIndex-startPointIndex ); // save cell size
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
    vtkIdType * ids = new vtkIdType[numPoints];

    // Only one scalar for each cell, NOT one per point
    vtkFloatArray * signals = vtkFloatArray::New();
    signals->Allocate(numCells);
    signals->SetName(m_scalarName.c_str());

    // Create the data set.  Need space for each cell, not for each point
    vtkUnstructuredGrid * visualDataSet = vtkUnstructuredGrid::New();
    this->dataSet = visualDataSet;
    visualDataSet->Allocate(numCells);
                                                                 // Now copy the saved point, cell
                                                                 // and signal info into vtk data
                                                                 // structures
    pointIndex = 0;
    for ( size_t cell_i = 0; cell_i < numCells; cell_i++ )       // for each cell
    {
      size_t startPointIndex = pointIndex;
      for ( size_t point_i = 0; point_i < saved_n_points_in_cell[ cell_i ]; point_i++ )  // for each point in cell
      {
        points->SetPoint( pointIndex, saved_centers[pointIndex] );
        ids[pointIndex] = pointIndex;
        pointIndex++;
      }
      signals->InsertNextTuple1( saved_signals[cell_i] );
      visualDataSet->InsertNextCell(VTK_POLY_VERTEX, saved_n_points_in_cell[ cell_i ], ids+startPointIndex);
    }

    if (VERBOSE) std::cout << tim << " to create " << pointIndex << " points." << std::endl;

    //Shrink to fit
    //points->Squeeze();
    signals->Squeeze();
    visualDataSet->Squeeze();

    //Add points and scalars
    visualDataSet->SetPoints(points);
    visualDataSet->GetCellData()->SetScalars(signals);
  }


  //-------------------------------------------------------------------------------------------------
  /*
  Generate the vtkDataSet from the objects input IMDEventWorkspace
  @param progressUpdating: Reporting object to pass progress information up the stack.
  @return fully constructed vtkDataSet.
  */
  vtkDataSet* vtkSplatterPlotFactory::create(ProgressAction& progressUpdating) const
  {
    UNUSED_ARG(progressUpdating);
    validate();

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
      for (size_t d=0; d<nd; d++)
        this->sliceMask[d] = (d<3);

      // Define where the slice is in 4D
      // TODO: Where to slice? Right now is just 0
      std::vector<coord_t> point(nd, 0);

      // Define two opposing planes that point in all higher dimensions
      std::vector<coord_t> normal1(nd, 0);
      std::vector<coord_t> normal2(nd, 0);
      for (size_t d=3; d<nd; d++)
      {
        normal1[d] = +1.0;
        normal2[d] = -1.0;
      }
      // This creates a 0-thickness region to slice in.
      sliceImplicitFunction->addPlane( MDPlane(normal1, point) );
      sliceImplicitFunction->addPlane( MDPlane(normal2, point) );
    }
    else
    {
      // Direct 3D, so no slicing
      this->slice = false;
    }

    // Macro to call the right instance of the
    CALL_MDEVENT_FUNCTION(this->doCreate, m_workspace);

    // Clean up
    if (this->slice)
    {
      delete this->sliceMask;
      delete this->sliceImplicitFunction;
    }

    // The macro does not allow return calls, so we used a member variable.
    return this->dataSet;
  }

 /*
  Initalize the factory with the workspace. This allows top level decision on what factory to use, but allows presenter/algorithms to pass in the
  dataobjects (workspaces) to run against at a later time. If workspace is not an IMDEventWorkspace, throws an invalid argument exception.
  @Param ws : Workspace to use.
  */
  void vtkSplatterPlotFactory::initialize(Mantid::API::Workspace_sptr ws)
  {
    this->m_workspace = boost::dynamic_pointer_cast<IMDEventWorkspace>(ws);
    if(!m_workspace)
      throw std::invalid_argument("Workspace is null or not IMDEventWorkspace");
  }

  /// Validate the current object.
  void vtkSplatterPlotFactory::validate() const
  { 
    if(!m_workspace)
    {
      throw std::runtime_error("Invalid vtkSplatterPlotFactory. Workspace is null");
    }
    if (m_workspace->getNumDims() < 3)
      throw std::runtime_error("Invalid vtkSplatterPlotFactory. Workspace must have at least 3 dimensions.");
  }

  /** Sets the number of points to show
  */
  void vtkSplatterPlotFactory::SetNumberOfPoints(size_t points)
  {
    m_numPoints = points;
  }


  /**
   *  Set the size of the initial portion of the sorted list of boxes that 
   *  will will be used when getting events to plot as points.
   *
   *  @percentToUse  The portion of the list to use, as a percentage. 
   *                 NOTE: This must be more than 0 and no more than 100
   *                 and whatever value is passed in will be restricted
   *                 to the interval (0,100].
   */
  void vtkSplatterPlotFactory::SetPercentToUse( double percentToUse )
  {
    if ( percentToUse <= 0 )
      m_percentToUse = 5;
    else if ( percentToUse > 100 )
      m_percentToUse = 100;
    else 
      m_percentToUse = percentToUse;
  }

  }
}
