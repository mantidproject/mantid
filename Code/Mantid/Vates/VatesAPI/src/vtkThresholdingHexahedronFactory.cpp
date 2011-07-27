#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidVatesAPI/vtkThresholdingHexahedronFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::MDEvents;

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
    m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(workspace);
    // Check that a workspace has been provided.
    validateWsNotNull();
    // When the workspace can not be handled by this type, take action in the form of delegation.
    const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::ThreeDimensional)
    {
      if(this->hasSuccessor())
      {
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

  /** Method for creating a 3D IMDWorkspace type
  * @return the vtkDataSet created
  */
  vtkDataSet* vtkThresholdingHexahedronFactory::createFromAnyIMDWorkspace3D() const
  {
   
    validate();

    const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();

    
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

      //The following represent actual calculated positions.
      double posX, posY, posZ;

      double signalScalar;
      const int nPointsX = nBinsX+1;
      const int nPointsY = nBinsY+1;
      const int nPointsZ = nBinsZ+1;


      CPUTimer tim;
      bool * voxelShown = new bool[nBinsX*nBinsY*nBinsZ];

      // Array of the points that should be created, set to false
      bool * pointNeeded = new bool[nPointsX*nPointsY*nPointsZ];
      memset(pointNeeded, 0, nPointsX*nPointsY*nPointsZ*sizeof(bool));

      size_t index = 0;
      //PARALLEL_FOR_NO_WSP_CHECK()
      for (int i = 0; i < nBinsX; i++)
      {
        for (int j = 0; j < nBinsY; j++)
        {
          for (int k = 0; k < nBinsZ; k++)
          {
            index = k + (nBinsZ * j) + (nBinsZ*nBinsY*i);
            signalScalar = m_workspace->getSignalNormalizedAt(i, j, k);
            signal->InsertNextValue(static_cast<float>(signalScalar));
            if (boost::math::isnan( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
            {
              // out of range
              voxelShown[index] = false;
            }
            else
            {
              // Valid data
              voxelShown[index] = true;
              

              // Make sure all 8 neighboring points are set to true
              size_t pointIndex = i * nPointsY*nPointsZ + j*nPointsZ + k;
              pointNeeded[pointIndex] = true;  pointIndex++;
              pointNeeded[pointIndex] = true;  pointIndex += nPointsZ-1;
              pointNeeded[pointIndex] = true;  pointIndex++;
              pointNeeded[pointIndex] = true;  pointIndex += nPointsY*nPointsZ - nPointsZ - 1;
              pointNeeded[pointIndex] = true;  pointIndex++;
              pointNeeded[pointIndex] = true;  pointIndex += nPointsZ-1;
              pointNeeded[pointIndex] = true;  pointIndex++;
              pointNeeded[pointIndex] = true;
            }
            //index++;
          }
        }
      }

      std::cout << tim << " to check all the signal values." << std::endl;

      // Array with the point IDs (only set where needed)
      vtkIdType * pointIDs = new vtkIdType[nPointsX*nPointsY*nPointsZ];
      index = 0;
      for (int i = 0; i < nPointsX; i++)
      {
        posX = minX + (i * incrementX); //Calculate increment in x;
        for (int j = 0; j < nPointsY; j++)
        {
          posY = minY + (j * incrementY); //Calculate increment in y;
          for (int k = 0; k < nPointsZ; k++)
          {
            // Create the point only when needed
            if (pointNeeded[index])
            {
              posZ = minZ + (k * incrementZ); //Calculate increment in z;
              pointIDs[index] = points->InsertNextPoint(posX, posY, posZ);
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
      vtkHexahedron *theHex = vtkHexahedron::New();
      index = 0;
      for (int i = 0; i < nBinsX; i++)
      {
        for (int j = 0; j < nBinsY; j++)
        {
          for (int k = 0; k < nBinsZ; k++)
          {
            if (voxelShown[index])
            {
              

              vtkIdType id_xyz = pointIDs[(i) * nPointsY*nPointsZ + (j)*nPointsZ + k];
              vtkIdType id_dxyz = pointIDs[(i+1) * nPointsY*nPointsZ + (j)*nPointsZ + k];
              vtkIdType id_dxdyz = pointIDs[(i+1) * nPointsY*nPointsZ + (j+1)*nPointsZ + k];
              vtkIdType id_xdyz = pointIDs[(i) * nPointsY*nPointsZ + (j+1)*nPointsZ + k];

              vtkIdType id_xydz = pointIDs[(i) * nPointsY*nPointsZ + (j)*nPointsZ + k+1];
              vtkIdType id_dxydz = pointIDs[(i+1) * nPointsY*nPointsZ + (j)*nPointsZ + k+1];
              vtkIdType id_dxdydz = pointIDs[(i+1) * nPointsY*nPointsZ + (j+1)*nPointsZ + k+1];
              vtkIdType id_xdydz = pointIDs[(i) * nPointsY*nPointsZ + (j+1)*nPointsZ + k+1];

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
      return visualDataSet;


  }


  /** Method for creating a 3D or 4D data set
   *
   * @param timestep :: int index of the time step (4th dimension) in the workspace.
   *        Set to 0 for a 3D workspace.
   * @return the vtkDataSet created
   */
  vtkDataSet* vtkThresholdingHexahedronFactory::create3Dor4D(const int timestep) const
  {
    
    MDHistoWorkspace_sptr hws = boost::dynamic_pointer_cast<MDHistoWorkspace>(m_workspace);

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

    //The following represent actual calculated positions.
    double posX, posY, posZ;

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

    // For speed, get a bare array to the underlying data
    const signal_t * signalData = hws->getSignalArray();
    const coord_t inverseVolume = hws->getInverseVolumeVolume();
    const size_t * indexMultiplier = hws->getIndexMultiplier();
    size_t indexMultiplier0 = indexMultiplier[0];
    size_t indexMultiplier1 = indexMultiplier[1];
    size_t indexOffset = 0;

    // For 4D data sets only: this is an extra offset
    if (timestep > 0)
      indexOffset = indexMultiplier[2] * timestep;

    size_t index = 0;
//      PARALLEL_FOR_NO_WSP_CHECK()
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

          // The following code reproduces: signalScalar = m_workspace->getSignalNormalizedAt(x, y, z, timeoffset);
          // (but should be much faster)
          signalScalar = inverseVolume * signalData[x + indexMultiplier0*y + indexMultiplier1*z + indexOffset];

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

    // Array with the point IDs (only set where needed)
    vtkIdType * pointIDs = new vtkIdType[nPointsX*nPointsY*nPointsZ];
    index = 0;
    for (int z = 0; z < nPointsZ; z++)
    {
      posZ = minZ + (z * incrementZ); //Calculate increment in z;
      for (int y = 0; y < nPointsY; y++)
      {
        posY = minY + (y * incrementY); //Calculate increment in y;
        for (int x = 0; x < nPointsX; x++)
        {
          // Create the point only when needed
          if (pointNeeded[index])
          {
            posX = minX + (x * incrementX); //Calculate increment in x;
            pointIDs[index] = points->InsertNextPoint(posX, posY, posZ);
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
      //If the workspace is not of the MDHistoWorkspace type. Fall back on generic IMDWorkspace behaviour.
      MDHistoWorkspace_sptr hws = boost::dynamic_pointer_cast<MDHistoWorkspace>(m_workspace);
      if (!hws)
      {
        return createFromAnyIMDWorkspace3D();
      }
      else
      {
        return this->create3Dor4D(0);
      }
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
