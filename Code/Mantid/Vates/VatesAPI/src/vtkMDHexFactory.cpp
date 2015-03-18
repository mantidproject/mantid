#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
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
  vtkMDHexFactory::vtkMDHexFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t maxDepth) :
    m_thresholdRange(thresholdRange), m_scalarName(scalarName), m_maxDepth(maxDepth),
    dataSet(NULL), slice(false), sliceMask(NULL), sliceImplicitFunction(NULL), m_time(0)
  {
  }

  /// Destructor
  vtkMDHexFactory::~vtkMDHexFactory()
  {
  }

  //-------------------------------------------------------------------------------------------------
  /* Generate the vtkDataSet from the objects input MDEventWorkspace (of a given type an dimensionality 3+)
  *
  * @param ws: workspace to draw from
  * @return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
  */
  template<typename MDE, size_t nd>
  void vtkMDHexFactory::doCreate(typename MDEventWorkspace<MDE, nd>::sptr ws) const
  {
    bool VERBOSE = true;
    CPUTimer tim;
    // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
    ReadLock lock(*ws);

    // First we get all the boxes, up to the given depth; with or wo the slice function
    std::vector<API::IMDNode *> boxes;
    if (this->slice)
      ws->getBox()->getBoxes(boxes, m_maxDepth, true, this->sliceImplicitFunction);
    else
      ws->getBox()->getBoxes(boxes, m_maxDepth, true);


    vtkIdType numBoxes = boxes.size();
    vtkIdType imageSizeActual = 0;

    if (VERBOSE) std::cout << tim << " to retrieve the " << numBoxes << " boxes down to depth " << m_maxDepth << std::endl;

    // Create 8 points per box.
    vtkPoints *points = vtkPoints::New();
    points->Allocate(numBoxes * 8);
    points->SetNumberOfPoints(numBoxes * 8);

    // One scalar per box
    vtkFloatArray * signals = vtkFloatArray::New();
    signals->Allocate(numBoxes);
    signals->SetName(m_scalarName.c_str());
    signals->SetNumberOfComponents(1);
    //signals->SetNumberOfValues(numBoxes);

    // To cache the signal
    float * signalArray = new float[numBoxes];

    // True for boxes that we will use
    bool * useBox = new bool[numBoxes];
    memset(useBox, 0, sizeof(bool)*numBoxes);

    // Create the data set
    vtkUnstructuredGrid * visualDataSet = vtkUnstructuredGrid::New();
    this->dataSet = visualDataSet;
    visualDataSet->Allocate(numBoxes);

    vtkIdList * hexPointList = vtkIdList::New();
    hexPointList->SetNumberOfIds(8);

    // This can be parallelized
    // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for schedule (dynamic) )
      for (int ii=0; ii<int(boxes.size()); ii++)
      {
        // Get the box here
        size_t i = size_t(ii);
        API::IMDNode * box = boxes[i];
        Mantid::signal_t signal_normalized= box->getSignalNormalized();

        if (!isSpecial( signal_normalized ) && m_thresholdRange->inRange(signal_normalized))
        {
          // Cache the signal and using of it
          signalArray[i] = float(signal_normalized);
          useBox[i] = true;

          //Get the coordinates.
          size_t numVertexes = 0;
          coord_t * coords;

          // If slicing down to 3D, specify which dimensions to keep.
          if (this->slice)
            coords = box->getVertexesArray(numVertexes, 3, this->sliceMask);
          else
            coords = box->getVertexesArray(numVertexes);

          if (numVertexes == 8)
          {
            //Iterate through all coordinates. Candidate for speed improvement.
            for(size_t v = 0; v < numVertexes; v++)
            {
              coord_t * coord = coords + v*3;
              // Set the point at that given ID
              points->SetPoint(i*8 + v, coord[0], coord[1], coord[2]);
              std::string msg;
            }

          } // valid number of vertexes returned

          // Free memory
          delete [] coords;
        }
      } // For each box

      if (VERBOSE) std::cout << tim << " to create the necessary points." << std::endl;
      //Add points
      visualDataSet->SetPoints(points);

      for (size_t i=0; i<boxes.size(); i++)
      {
        if (useBox[i])
        {
          // The bare point ID
          vtkIdType pointIds = i * 8;

          //Add signal
          signals->InsertNextValue(signalArray[i]);

          hexPointList->SetId(0, pointIds + 0); //xyx
          hexPointList->SetId(1, pointIds + 1); //dxyz
          hexPointList->SetId(2, pointIds + 3); //dxdyz
          hexPointList->SetId(3, pointIds + 2); //xdyz
          hexPointList->SetId(4, pointIds + 4); //xydz
          hexPointList->SetId(5, pointIds + 5); //dxydz
          hexPointList->SetId(6, pointIds + 7); //dxdydz
          hexPointList->SetId(7, pointIds + 6); //xdydz

          //Add cells
          visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexPointList);


          double bounds[6];

          visualDataSet->GetCellBounds(imageSizeActual, bounds);

          if(bounds[0] < -10 || bounds[2] < -10 ||bounds[4]< -10)
          {
            std::string msg = "";
          }
          imageSizeActual++;
        }
      } // for each box.

      delete[] signalArray;
      delete[] useBox;

      //Shrink to fit
      signals->Squeeze();
      visualDataSet->Squeeze();

      //Add scalars
      visualDataSet->GetCellData()->SetScalars(signals);

      // Hedge against empty data sets
      if (visualDataSet->GetNumberOfPoints() <= 0)
      {
        visualDataSet->Delete();
        vtkNullUnstructuredGrid nullGrid;
        visualDataSet = nullGrid.createNullData();
        this->dataSet = visualDataSet;
      }

      if (VERBOSE) std::cout << tim << " to create " << imageSizeActual << " hexahedrons." << std::endl;

  }


  //-------------------------------------------------------------------------------------------------
  /*
  Generate the vtkDataSet from the objects input IMDEventWorkspace
  @param progressUpdating: Reporting object to pass progress information up the stack.
  @Return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
  */
  vtkDataSet* vtkMDHexFactory::create(ProgressAction& progressUpdating) const
  {
    this->dataSet = tryDelegatingCreation<IMDEventWorkspace, 3>(m_workspace, progressUpdating, false);
    if(this->dataSet != NULL)
    {
      return this->dataSet;
    }
    else
    {
      IMDEventWorkspace_sptr imdws = this->castAndCheck<IMDEventWorkspace, 3>(m_workspace, false);

      size_t nd = imdws->getNumDims();
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
        point[3] = coord_t(m_time); //Specifically for 4th/time dimension.

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

        //coord_t pointA[4] = {0, 0, 0, -1.0};
        //coord_t pointB[4] = {0, 0, 0, +2.0};
      }
      else
      {
        // Direct 3D, so no slicing
        this->slice = false;
      }
      progressUpdating.eventRaised(0.1);
      // Macro to call the right instance of the
      CALL_MDEVENT_FUNCTION(this->doCreate, imdws);
      progressUpdating.eventRaised(1.0);

      // Clean up
      if (this->slice)
      {
        delete[] this->sliceMask;
        delete this->sliceImplicitFunction;
      }

      // The macro does not allow return calls, so we used a member variable.
      return this->dataSet;
    }
  }

  /*
  Initalize the factory with the workspace. This allows top level decision on what factory to use, but allows presenter/algorithms to pass in the
  dataobjects (workspaces) to run against at a later time. If workspace is not an IMDEventWorkspace, attempts to use any run-time successor set.
  @Param ws : Workspace to use.
  */
  void vtkMDHexFactory::initialize(Mantid::API::Workspace_sptr ws)
  {
    IMDEventWorkspace_sptr imdws = doInitialize<IMDEventWorkspace, 3>(ws, false);
    m_workspace = imdws;
    
    //Setup range values according to whatever strategy object has been injected.
    m_thresholdRange->setWorkspace(ws);
    m_thresholdRange->calculate();
  }

  /// Validate the current object.
  void vtkMDHexFactory::validate() const
  { 
    if(!m_workspace)
    {
      throw std::runtime_error("Invalid vtkMDHexFactory. Workspace is null");
    }
  }

  /** Sets the recursion depth to a specified level in the workspace.
  */
  void vtkMDHexFactory::setRecursionDepth(size_t depth)
  {
    m_maxDepth = depth;
  }

  /*
  Set the time value.
  */
  void vtkMDHexFactory::setTime(double time)
  {
    m_time = time;
  }

  }
}
