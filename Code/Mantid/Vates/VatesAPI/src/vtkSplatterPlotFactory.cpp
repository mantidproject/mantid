#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
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

namespace Mantid
{
  namespace VATES
  {
  
  /*Constructor
  @Param thresholdRange : Threshold range strategy
  @scalarName : Name for scalar signal array.
  */
  vtkSplatterPlotFactory::vtkSplatterPlotFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t numPoints) :
  m_thresholdRange(thresholdRange), m_scalarName(scalarName), m_numPoints(numPoints)
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

    // Find out how many events to skip before making a point
    size_t totalPoints = ws->getNPoints();
    size_t interval = totalPoints / m_numPoints;
    if (interval == 0) interval = 1;

    // How many points will we ACTUALLY have?
    size_t numPoints = (totalPoints / interval);
    std::cout << "Plotting points at an interval of " << interval << ", will give " << numPoints << " points." << std::endl;

    // First we get all the boxes, up to the given depth; with or wo the slice function
    std::vector<IMDBox<MDE,nd> *> boxes;
    if (this->slice)
      ws->getBox()->getBoxes(boxes, 1000, true, this->sliceImplicitFunction);
    else
      ws->getBox()->getBoxes(boxes, 1000, true);

    if (VERBOSE) std::cout << tim << " to retrieve the " << boxes.size() << " boxes down." << std::endl;

    // Create all the points
    vtkPoints *points = vtkPoints::New();
    points->Allocate(numPoints);
    points->SetNumberOfPoints(numPoints);
    
    // Same number of scalars. Create them all
    vtkFloatArray * signals = vtkFloatArray::New();
    signals->Allocate(numPoints);
    signals->SetName(m_scalarName.c_str());

    // Create the data set
    vtkUnstructuredGrid * visualDataSet = vtkUnstructuredGrid::New();
    this->dataSet = visualDataSet;
    visualDataSet->Allocate(numPoints);

    // Point we are creating
    size_t pointIndex = 0;
    // A counter for points
    size_t pointsCounted = 0;

    // The list of IDs to use
    vtkIdType * ids = new vtkIdType[numPoints];

    // This can be parallelized
    for (int ii=0; ii<int(boxes.size()); ii++)
    {
      // Get the box here
      size_t i = size_t(ii);
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
      if (box)
      {
        size_t newPoints = box->getNPoints();
        if (newPoints > 0)
        {
          if (pointsCounted + newPoints > interval)
          {
            // Cache the normalized signal
            float signal_normalized = float(box->getSignalNormalized());
            // Where in the ID list does this box start?
            size_t startPointIndex = pointIndex;

            // Show at least an event from this box
            const std::vector<MDE> & events = box->getConstEvents();
            // Start here
            size_t j = interval-pointsCounted;
            for (; j < events.size(); j += interval)
            {
              const MDE & ev = events[j];
              const coord_t * center = ev.getCenter();
              // TODO: Handle reduced dimensions
              //std::cout << box->getId() << " event at " << ev.getCenter(0) << "," << ev.getCenter(1) << "," << ev.getCenter(2) << std::endl;
              //points->SetPoint(pointIndex, center[0], center[1], center[2]);
              points->SetPoint(pointIndex, center);
              // Save for the point set
              ids[pointIndex] = pointIndex;
              pointIndex++;
              if (pointIndex > numPoints)
              {
                std::cout << "Exceeded allocated points!" << std::endl;
                ii = int(boxes.size());
                break;
              }
            }
            // Set the points counted to the remainder of what was skipped
            pointsCounted = (events.size() - (j-interval));
            // Done with the event list
            box->releaseEvents();

            // The signal will match that point
            signals->InsertNextTuple1(signal_normalized);
            // Create a "poly vertex" set of vertexes starting at this point in the box
            visualDataSet->InsertNextCell(VTK_POLY_VERTEX, pointIndex-startPointIndex, ids+startPointIndex);
          }
          else
          {
            // Skip it. Too few events, we are going on
            pointsCounted += newPoints;
          }
        } // box has any points
      } // box is valid MDBox
    } // For each box
    if (VERBOSE) std::cout << tim << " to create " << pointIndex << " points." << std::endl;

    //Shrink to fit
//    points->Squeeze();
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

  }
}
