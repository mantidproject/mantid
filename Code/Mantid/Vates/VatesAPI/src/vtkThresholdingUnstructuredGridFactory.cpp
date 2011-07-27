#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace VATES
{

  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const double timestep)
  : vtkThresholdingHexahedronFactory(thresholdRange,  scalarName),
    m_timestep(timestep)
  {
  }

    /**
  Assigment operator
  @param other : vtkThresholdingHexahedronFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>& vtkThresholdingUnstructuredGridFactory<TimeMapper>::operator=(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_thresholdRange = other.m_thresholdRange;
      this->m_workspace = other.m_workspace;
      this->m_timestep = other.m_timestep;
      this->m_timeMapper = other.m_timeMapper;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other)
   : vtkThresholdingHexahedronFactory(other),
     m_timestep(other.m_timestep), m_timeMapper(other.m_timeMapper)
  {
    
  }


  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(workspace);
    // Check that a workspace has been provided.
    validateWsNotNull();
    // When the workspace can not be handled by this type, take action in the form of delegation.
    size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::FourDimensional)
    {
      if(this->hasSuccessor())
      {
        m_successor->initialize(m_workspace);
        return;
      }
      else
      {
        throw std::runtime_error("There is no successor factory set for this vtkThresholdingUnstructuredGridFactory type");
      }
    }

    double tMax = m_workspace->getTDimension()->getMaximum();
    double tMin = m_workspace->getTDimension()->getMinimum();
    size_t nbins = m_workspace->getTDimension()->getNBins();

    m_timeMapper = TimeMapper::construct(tMin, tMax, nbins);

    //Setup range values according to whatever strategy object has been injected.
    m_thresholdRange->setWorkspace(m_workspace);
    m_thresholdRange->calculate();
  }

  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::validate() const
  {
    validateWsNotNull();
  }

  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::create() const
  {
    validate();

    size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::FourDimensional)
    {
      return m_successor->create();
    }
    else
    { 
      MDHistoWorkspace_sptr hws = boost::dynamic_pointer_cast<MDHistoWorkspace>(m_workspace);
      if (!hws)
      {
        return createFromAnyIMDWorkspace4D();
      }
      else
      {
        // Create the mesh in a 4D mode
        return this->create3Dor4D(m_timeMapper(m_timestep));
      }
    }
  }


  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::~vtkThresholdingUnstructuredGridFactory()
  {
  }

  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createMeshOnly() const
  {
    throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
  }

  template<typename TimeMapper>
  vtkFloatArray* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createScalarArray() const
  {
    throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
  }

  /*
  Creates an vtk dataset from an IMDWorkspace.
  @return a fully constructed vtkUnstructuredGrid containing mesh and scalar values from the IMDWorkspace.
  */
  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createFromAnyIMDWorkspace4D() const
  {
    validate();

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
          signalScalar = m_workspace->getSignalNormalizedAt(i, j, k, m_timeMapper(m_timestep));
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

  template class vtkThresholdingUnstructuredGridFactory<TimeToTimeStep>;
  template class vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep>;

}
}
