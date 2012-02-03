#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidVatesAPI/vtkThresholdingHexahedronFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include "MantidAPI/NullCoordTransform.h"
#include "MantidKernel/ReadLock.h"

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::MDEvents;
using Mantid::Kernel::ReadLock;

namespace Mantid
{
namespace VATES 
{

  vtkThresholdingHexahedronFactory::vtkThresholdingHexahedronFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) :
  m_scalarName(scalarName), m_thresholdRange(thresholdRange)
  {
  }

  /**
  Assigment operator
  @param other : vtkThresholdingHexahedronFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  vtkThresholdingHexahedronFactory& vtkThresholdingHexahedronFactory::operator=(const vtkThresholdingHexahedronFactory& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_thresholdRange = other.m_thresholdRange;
      this->m_workspace = other.m_workspace;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  vtkThresholdingHexahedronFactory::vtkThresholdingHexahedronFactory(const vtkThresholdingHexahedronFactory& other)
  {
   this->m_scalarName = other.m_scalarName;
   this->m_thresholdRange = other.m_thresholdRange;
   this->m_workspace = other.m_workspace;
  }

  void vtkThresholdingHexahedronFactory::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = boost::dynamic_pointer_cast<MDHistoWorkspace>(workspace);
    // Check that a workspace has been provided.
    validateWsNotNull();
    // When the workspace can not be handled by this type, take action in the form of delegation.
    const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::ThreeDimensional)
    {
      if(this->hasSuccessor())
      {
        m_successor->setUseTransform(m_useTransform);
        m_successor->initialize(m_workspace);
        return;
      }
      else
      {
        throw std::runtime_error("There is no successor factory set for this vtkThresholdingHexahedronFactory type");
      }
    }

    //Setup range values according to whatever strategy object has been injected.
    m_thresholdRange->setWorkspace(m_workspace);
    m_thresholdRange->calculate();
  }

  void vtkThresholdingHexahedronFactory::validateWsNotNull() const
  {
    
    if(NULL == m_workspace.get())
    {
      throw std::runtime_error("IMDWorkspace is null");
    }
  }

  void vtkThresholdingHexahedronFactory::validate() const
  {
    validateWsNotNull();
  }


  /** Method for creating a 3D or 4D data set
   *
   * @param timestep :: index of the time step (4th dimension) in the workspace.
   *        Set to 0 for a 3D workspace.
   * @param do4D :: if true, create a 4D dataset, else to 3D
   * @return the vtkDataSet created
   */
  vtkDataSet* vtkThresholdingHexahedronFactory::create3Dor4D(size_t timestep, bool do4D) const
  {
    // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
    ReadLock lock(*m_workspace);

    const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
    const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );
    const int nBinsZ = static_cast<int>( m_workspace->getZDimension()->getNBins() );

    const double maxX = m_workspace-> getXDimension()->getMaximum();
    const double minX = m_workspace-> getXDimension()->getMinimum();
    const double maxY = m_workspace-> getYDimension()->getMaximum();
    const double minY = m_workspace-> getYDimension()->getMinimum();
    const double maxZ = m_workspace-> getZDimension()->getMaximum();
    const double minZ = m_workspace-> getZDimension()->getMinimum();

    double incrementX = (maxX - minX) / (nBinsX);
    double incrementY = (maxY - minY) / (nBinsY);
    double incrementZ = (maxZ - minZ) / (nBinsZ);

    const int imageSize = (nBinsX ) * (nBinsY ) * (nBinsZ );
    vtkPoints *points = vtkPoints::New();
    points->Allocate(static_cast<int>(imageSize));

    vtkFloatArray * signal = vtkFloatArray::New();
    signal->Allocate(imageSize);
    signal->SetName(m_scalarName.c_str());
    signal->SetNumberOfComponents(1);

    double signalScalar;
    const int nPointsX = nBinsX+1;
    const int nPointsY = nBinsY+1;
    const int nPointsZ = nBinsZ+1;

    CPUTimer tim;

    /* The idea of the next chunk of code is that you should only
     create the points that will be needed; so an array of pointNeeded
     is set so that all required vertices are marked, and created in a second step. */

    // Array of the points that should be created, set to false
    bool * pointNeeded = new bool[nPointsX*nPointsY*nPointsZ];
    memset(pointNeeded, 0, nPointsX*nPointsY*nPointsZ*sizeof(bool));
    // Array with true where the voxel should be shown
    bool * voxelShown = new bool[nBinsX*nBinsY*nBinsZ];

    size_t index = 0;

    for (int z = 0; z < nBinsZ; z++)
    {
      for (int y = 0; y < nBinsY; y++)
      {
        for (int x = 0; x < nBinsX; x++)
        {
          /* NOTE: It is very important to match the ordering of the two arrays
           * (the one used in MDHistoWorkspace and voxelShown/pointNeeded).
           * If you access the array in the wrong way and can't cache it on L1/L2 cache, I got a factor of 8x slowdown.
           */
          //index = x + (nBinsX * y) + (nBinsX*nBinsY*z);

          if (do4D)
            signalScalar = m_workspace->getSignalNormalizedAt(x,y,z, timestep);
          else
            signalScalar = m_workspace->getSignalNormalizedAt(x,y,z);

          if (boost::math::isnan( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
          {
            // out of range
            voxelShown[index] = false;
          }
          else
          {
            // Valid data
            voxelShown[index] = true;
            signal->InsertNextValue(static_cast<float>(signalScalar));

            // Make sure all 8 neighboring points are set to true
            size_t pointIndex = x + (nPointsX * y) + (nPointsX*nPointsY*z); //(Note this index is different then the other one)
            pointNeeded[pointIndex] = true;  pointIndex++;
            pointNeeded[pointIndex] = true;  pointIndex += nPointsX-1;
            pointNeeded[pointIndex] = true;  pointIndex++;
            pointNeeded[pointIndex] = true;  pointIndex += nPointsX*nPointsY - nPointsX - 1;
            pointNeeded[pointIndex] = true;  pointIndex++;
            pointNeeded[pointIndex] = true;  pointIndex += nPointsX-1;
            pointNeeded[pointIndex] = true;  pointIndex++;
            pointNeeded[pointIndex] = true;
          }
          index++;
        }
      }
    }

    std::cout << tim << " to check all the signal values." << std::endl;

    // Get the transformation that takes the points in the TRANSFORMED space back into the ORIGINAL (not-rotated) space.
    Mantid::API::CoordTransform* transform = NULL;
    if (m_useTransform)
      transform = m_workspace->getTransformToOriginal();

    Mantid::coord_t in[3]; 
    Mantid::coord_t out[3];
            
    // Array with the point IDs (only set where needed)
    vtkIdType * pointIDs = new vtkIdType[nPointsX*nPointsY*nPointsZ];
    index = 0;
    for (int z = 0; z < nPointsZ; z++)
    {
      in[2] = (minZ + (z * incrementZ)); //Calculate increment in z;
      for (int y = 0; y < nPointsY; y++)
      {
        in[1] = (minY + (y * incrementY)); //Calculate increment in y;
        for (int x = 0; x < nPointsX; x++)
        {
          // Create the point only when needed
          if (pointNeeded[index])
          {
            in[0] = (minX + (x * incrementX)); //Calculate increment in x;
            if (transform)
            {
              transform->apply(in, out);
              pointIDs[index] = points->InsertNextPoint(out);
            }
            else
            {
              pointIDs[index] = points->InsertNextPoint(in);
            }
          }
          index++;
        }
      }
    }

    std::cout << tim << " to create the needed points." << std::endl;

    vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
    visualDataSet->Allocate(imageSize);
    visualDataSet->SetPoints(points);
    visualDataSet->GetCellData()->SetScalars(signal);

    // ------ Hexahedron creation ----------------
    // It is approx. 40 x faster to create the hexadron only once, and reuse it for each voxel.
    vtkHexahedron *theHex = vtkHexahedron::New();
    index = 0;
    for (int z = 0; z < nBinsZ; z++)
    {
      for (int y = 0; y < nBinsY; y++)
      {
        for (int x = 0; x < nBinsX; x++)
        {
          if (voxelShown[index])
          {
            //Only create topologies for those cells which are not sparse.
            // create a hexahedron topology
            vtkIdType id_xyz =    pointIDs[(x)   + (y)*nPointsX + z*nPointsX*nPointsY];
            vtkIdType id_dxyz =   pointIDs[(x+1) + (y)*nPointsX + z*nPointsX*nPointsY];
            vtkIdType id_dxdyz =  pointIDs[(x+1) + (y+1)*nPointsX + z*nPointsX*nPointsY];
            vtkIdType id_xdyz =   pointIDs[(x)   + (y+1)*nPointsX + z*nPointsX*nPointsY];

            vtkIdType id_xydz =   pointIDs[(x)   + (y)*nPointsX + (z+1)*nPointsX*nPointsY];
            vtkIdType id_dxydz =  pointIDs[(x+1) + (y)*nPointsX + (z+1)*nPointsX*nPointsY];
            vtkIdType id_dxdydz = pointIDs[(x+1) + (y+1)*nPointsX + (z+1)*nPointsX*nPointsY];
            vtkIdType id_xdydz =  pointIDs[(x)   + (y+1)*nPointsX + (z+1)*nPointsX*nPointsY];

            //create the hexahedron
            theHex->GetPointIds()->SetId(0, id_xyz);
            theHex->GetPointIds()->SetId(1, id_dxyz);
            theHex->GetPointIds()->SetId(2, id_dxdyz);
            theHex->GetPointIds()->SetId(3, id_xdyz);
            theHex->GetPointIds()->SetId(4, id_xydz);
            theHex->GetPointIds()->SetId(5, id_dxydz);
            theHex->GetPointIds()->SetId(6, id_dxdydz);
            theHex->GetPointIds()->SetId(7, id_xdydz);

            visualDataSet->InsertNextCell(VTK_HEXAHEDRON, theHex->GetPointIds());
          }
          index++;
        }
      }
    }
    theHex->Delete();

    std::cout << tim << " to create and add the hexadrons." << std::endl;


    points->Delete();
    signal->Delete();
    visualDataSet->Squeeze();
    delete [] pointIDs;
    delete [] voxelShown;
    delete [] pointNeeded;
    return visualDataSet;

  }


  /** Create the data set;
   * will call the successor if the # of dimensions is not for this factory.
   * @return
   */
  vtkDataSet* vtkThresholdingHexahedronFactory::create() const
  {
    validate();

    const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::ThreeDimensional)
    {
      return m_successor->create();
    }
    else
    {
      // Create in 3D mode
      return this->create3Dor4D(0, false);
    }
  }


  vtkThresholdingHexahedronFactory::~vtkThresholdingHexahedronFactory()
  {
  }

  vtkDataSet* vtkThresholdingHexahedronFactory::createMeshOnly() const
  {
    throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
  }

  vtkFloatArray* vtkThresholdingHexahedronFactory::createScalarArray() const
  {
    throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
  }

}
}
