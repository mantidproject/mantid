#include "MantidVatesAPI/vtkMDEWHexahedronFactory.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDBoxIterator.h"

#include <boost/math/special_functions/fpclassify.hpp>

#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkHexahedron.h>

using namespace Mantid::API;
using namespace Mantid::MDEvents;

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
    
    vtkIdType imageSizeGuess = 1000000; //TODO. Need to know up front how many boxes are to be drawn to get this right.
    vtkIdType imageSizeActual = 0;
    
    vtkPoints *points = vtkPoints::New();
    points->Allocate(imageSizeGuess); 
    
    vtkFloatArray * signals = vtkFloatArray::New();
    signals->Allocate(imageSizeGuess);
    signals->SetName(m_scalarName.c_str());
    signals->SetNumberOfComponents(1);
    
    vtkUnstructuredGrid* visualDataSet = vtkUnstructuredGrid::New();
    visualDataSet->Allocate(imageSizeGuess);

    MDBoxIterator<MDEvent<3>,3> it(m_workspace->getBox(), m_maxDepth, true);
    IMDBox3 * box;

    vtkHexahedron* theHex = vtkHexahedron::New();
    std::vector<Mantid::Geometry::Coordinate> coords;
    Mantid::signal_t signal_normalized;

    vtkIdType pointId;
    while(true)
    {
      box = it.getBox();

      signal_normalized = box->getSignalNormalized();

      if (!boost::math::isnan( signal_normalized ) && m_thresholdRange->inRange(signal_normalized))
      {
        //Add signals
        signals->InsertNextValue(static_cast<float>(signal_normalized));

        //Get the coordinates. Candidate for speed improvement.
        std::vector<Mantid::Geometry::Coordinate> coords = box->getVertexes();

        //Iterate through all coordinates. Candidate for speed improvement.
        for(size_t i = 0; i < 8; i++)
        {
          Mantid::Geometry::Coordinate& coord = coords[i];
          //Add points
          pointId = points->InsertNextPoint(coord.getX(), coord.getY(), coord.getZ());
          theHex->GetPointIds()->SetId(i, pointId);
        }
        
        //Add cells
        visualDataSet->InsertNextCell(VTK_HEXAHEDRON, theHex->GetPointIds());

        imageSizeActual++;
      }
      if (!it.next()) 
        break;
    }

    //Shrink to fit
    points->Squeeze();
    signals->Squeeze();
    visualDataSet->Squeeze();

    //Add points
    visualDataSet->SetPoints(points);
    //Add scalars
    visualDataSet->GetCellData()->SetScalars(signals);

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