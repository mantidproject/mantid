#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidVatesAPI/vtkMDEWHexahedronFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::API;
using namespace Mantid::MDEvents;
using Mantid::Kernel::CPUTimer;

namespace Mantid
{
  namespace VATES
  {
  
  /*Constructor
  @Param thresholdRange : Threshold range strategy
  @scalarName : Name for scalar signal array.
  */
  vtkMDEWHexahedronFactory::vtkMDEWHexahedronFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t maxDepth) :
  m_thresholdRange(thresholdRange), m_scalarName(scalarName), m_maxDepth(maxDepth)
  {
  }

  /// Destructor
  vtkMDEWHexahedronFactory::~vtkMDEWHexahedronFactory()
  {
  }

  /*
  Generate the vtkDataSet from the objects input IMDEventWorkspace
  @Return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
  */
  vtkDataSet* vtkMDEWHexahedronFactory::create() const
  {
    validate();
    CPUTimer tim;
    
    // First we get all the boxes, up to the given depth
    std::vector<IMDBox3 *> boxes;
    m_workspace->getBox()->getBoxes(boxes, m_maxDepth, true);

    vtkIdType numBoxes = boxes.size();
    vtkIdType imageSizeActual = 0;
    
    std::cout << tim << " to retrieve the " << numBoxes << " boxes down to depth " << m_maxDepth << std::endl;

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
    vtkUnstructuredGrid* visualDataSet = vtkUnstructuredGrid::New();
    visualDataSet->Allocate(numBoxes);

    IMDBox3 * box;

    std::vector<Mantid::Geometry::Coordinate> coords;
    Mantid::signal_t signal_normalized;

    vtkIdList * hexPointList = vtkIdList::New();
    hexPointList->SetNumberOfIds(8);

    // This can be parallelized
    PRAGMA_OMP( parallel for schedule (dynamic) )
    for (int ii=0; ii<int(boxes.size()); ii++)
    {
      // Get the box here
      size_t i = size_t(ii);
      box = boxes[i];

      signal_normalized = box->getSignalNormalized();

      if (!boost::math::isnan( signal_normalized ) && m_thresholdRange->inRange(signal_normalized))
      {
        // Cache the signal and using of it
        signalArray[i] = float(signal_normalized);
        useBox[i] = true;

        //Get the coordinates.
        size_t numVertexes = 0;
        coord_t * coords = box->getVertexesArray(numVertexes);

        if (numVertexes == 8)
        {
          //Iterate through all coordinates. Candidate for speed improvement.
          for(size_t v = 0; v < numVertexes; v++)
          {
            coord_t * coord = coords + v*3;
            // Set the point at that given ID
            points->SetPoint(i*8 + v, coord[0], coord[1], coord[2]);
          }

        } // valid number of vertexes returned

        // Free memory
        delete [] coords;
      }
    } // For each box

    std::cout << tim << " to create the necessary points." << std::endl;

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
        imageSizeActual++;
      }
    } // for each box.

    //Shrink to fit
    signals->Squeeze();
    visualDataSet->Squeeze();

    //Add points
    visualDataSet->SetPoints(points);
    //Add scalars
    visualDataSet->GetCellData()->SetScalars(signals);

    std::cout << tim << " to create " << imageSizeActual << " hexahedrons." << std::endl;

    return visualDataSet;
  }
  
  /*
  Create as Mesh Only. Legacy method
  @Return Nothing. throws on invoke.
  */
  vtkDataSet* vtkMDEWHexahedronFactory::createMeshOnly() const
  {
    throw std::runtime_error("Invalid usage. Cannot call vtkMDEWHexahedronFactory::createMeshOnly()");
  }

  /*
  Create as Mesh Only. Legacy method
  @Return Nothing. throws on invoke.
  */
  vtkFloatArray* vtkMDEWHexahedronFactory::createScalarArray() const
  {
    throw std::runtime_error("Invalid usage. Cannot call vtkMDEWHexahedronFactory::createScalarArray()");
  }

 /*
  Initalize the factory with the workspace. This allows top level decision on what factory to use, but allows presenter/algorithms to pass in the
  dataobjects (workspaces) to run against at a later time. If workspace is not an IMDEventWorkspace, throws an invalid argument exception.
  @Param ws : Workspace to use.
  */
  void vtkMDEWHexahedronFactory::initialize(Mantid::API::Workspace_sptr ws)
  {
    if(!ws)
    {
      throw std::runtime_error("Workspace is null");
    }
    MDEventWorkspace3_sptr temp = boost::dynamic_pointer_cast<MDEventWorkspace3>(ws);
    if(!temp)
    {
      std::string message = "Cannot initialize vtkMDEWHexahedronFactory with workspace of type: " + ws->getName();
      throw std::invalid_argument(message);
    }
    this->m_workspace = temp;
  }

  /// Validate the current object.
  void vtkMDEWHexahedronFactory::validate() const
  { 
    if(!m_workspace)
    {
      throw std::runtime_error("Invalid vtkMDEWHexahedronFactory. Workspace is null");
    }
  }

  }
}
