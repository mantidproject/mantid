#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidKernel/ReadLock.h"

#include "vtkNew.h"
#include "vtkStructuredGrid.h"
#include "vtkDoubleArray.h"

using Mantid::API::IMDWorkspace;
using Mantid::API::IMDHistoWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::DataObjects;
using Mantid::Kernel::ReadLock;

namespace Mantid
{
namespace VATES 
{

  vtkMDHistoHexFactory::vtkMDHistoHexFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) :
  m_scalarName(scalarName), m_thresholdRange(thresholdRange)
  {
  }

  /**
  Assigment operator
  @param other : vtkMDHistoHexFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  vtkMDHistoHexFactory& vtkMDHistoHexFactory::operator=(const vtkMDHistoHexFactory& other)
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
  vtkMDHistoHexFactory::vtkMDHistoHexFactory(const vtkMDHistoHexFactory& other)
  {
   this->m_scalarName = other.m_scalarName;
   this->m_thresholdRange = other.m_thresholdRange;
   this->m_workspace = other.m_workspace;
  }

  void vtkMDHistoHexFactory::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = doInitialize<MDHistoWorkspace, 3>(workspace);

    //Setup range values according to whatever strategy object has been injected.
    m_thresholdRange->setWorkspace(workspace);
    m_thresholdRange->calculate();
  }

  void vtkMDHistoHexFactory::validateWsNotNull() const
  {
    
    if(NULL == m_workspace.get())
    {
      throw std::runtime_error("IMDWorkspace is null");
    }
  }

  void vtkMDHistoHexFactory::validate() const
  {
    validateWsNotNull();
  }


  /** Method for creating a 3D or 4D data set
   *
   * @param timestep :: index of the time step (4th dimension) in the workspace.
   *        Set to 0 for a 3D workspace.
   * @param do4D :: if true, create a 4D dataset, else to 3D
   * @param progressUpdate: Progress updating. passes progress information up the stack.
   * @return the vtkDataSet created
   */
  vtkDataSet* vtkMDHistoHexFactory::create3Dor4D(size_t timestep, bool do4D, ProgressAction & progressUpdate) const
  {
    // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
    ReadLock lock(*m_workspace);

    const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
    const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );
    const int nBinsZ = static_cast<int>( m_workspace->getZDimension()->getNBins() );

    const coord_t maxX = m_workspace->getXDimension()->getMaximum();
    const coord_t minX = m_workspace->getXDimension()->getMinimum();
    const coord_t maxY = m_workspace->getYDimension()->getMaximum();
    const coord_t minY = m_workspace->getYDimension()->getMinimum();
    const coord_t maxZ = m_workspace->getZDimension()->getMaximum();
    const coord_t minZ = m_workspace->getZDimension()->getMinimum();

    coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
    coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);
    coord_t incrementZ = (maxZ - minZ) / static_cast<coord_t>(nBinsZ);

    const int imageSize = (nBinsX ) * (nBinsY ) * (nBinsZ );
    vtkPoints *points = vtkPoints::New();
    points->Allocate(static_cast<int>(imageSize));

    //vtkFloatArray * signal = vtkFloatArray::New();
    //signal->Allocate(imageSize);
    //signal->SetName(m_scalarName.c_str());
    //signal->SetNumberOfComponents(1);
    
    vtkNew<vtkDoubleArray> signal;
    signal->SetName(m_scalarName.c_str());
    signal->SetNumberOfComponents(1);
    // (argument #3) tell VTK to not delete this array
    signal->SetArray(m_workspace->getSignalArray(),imageSize,1);
    assert(imageSize == signal->GetNumberOfTuples());

    vtkNew<vtkStructuredGrid> visualDataset;
    visualDataset->SetDimensions(nBinsX+1,nBinsY+1,nBinsZ+1);
    
    
    double signalScalar;
    const int nPointsX = nBinsX+1;
    const int nPointsY = nBinsY+1;
    const int nPointsZ = nBinsZ+1;

    CPUTimer tim;

    /* The idea of the next chunk of code is that you should only
     create the points that will be needed; so an array of pointNeeded
     is set so that all required vertices are marked, and created in a second step. */

    // Array with true where the voxel should be shown
    double progressFactor = 0.5/double(nBinsZ);
    double progressOffset = 0.5;

    
    
    vtkIdType index = 0;
    for (int z = 0; z < nBinsZ; z++)
    {
      //Report progress updates for the first 50%
      progressUpdate.eventRaised(double(z)*progressFactor);
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

          bool maskValue = (isSpecial( signalScalar ) || !m_thresholdRange->inRange(signalScalar));
          if (maskValue)
          {
            visualDataset->BlankCell(index);
          }
          index++;
        }
      }
    }

    std::cout << tim << " to check all the signal values." << std::endl;

    // Get the transformation that takes the points in the TRANSFORMED space back into the ORIGINAL (not-rotated) space.
    Mantid::API::CoordTransform const * transform = NULL;
    if (m_useTransform)
      transform = m_workspace->getTransformToOriginal();

    Mantid::coord_t in[3]; 
    Mantid::coord_t out[3];
            
    // Array with the point IDs (only set where needed)
    index = 0;
    progressFactor = 0.5/static_cast<double>(nPointsZ);

    for (int z = 0; z < nPointsZ; z++)
    {
      //Report progress updates for the last 50%
      progressUpdate.eventRaised(double(z)*progressFactor + progressOffset);
      in[2] = (minZ + (static_cast<coord_t>(z) * incrementZ)); //Calculate increment in z;
      for (int y = 0; y < nPointsY; y++)
      {
        in[1] = (minY + (static_cast<coord_t>(y) * incrementY)); //Calculate increment in y;
        for (int x = 0; x < nPointsX; x++)
        {
          // Create the point only when needed
          in[0] = (minX + (static_cast<coord_t>(x) * incrementX)); //Calculate increment in x;
          if (transform)
          {
            transform->apply(in, out);
            points->InsertNextPoint(out);
          }
          else
          {
            points->InsertNextPoint(in);
          }
          index++;
        }
      }
    }

    std::cout << tim << " to create the needed points." << std::endl;
    visualDataset->SetPoints(points);
    visualDataset->GetCellData()->SetScalars(signal.GetPointer());
    visualDataset->Register(NULL);
        
    std::cout << tim << " to create and add the hexadrons." << std::endl;

    points->Delete();
    visualDataset->Squeeze();
    return visualDataset.GetPointer();
  }


  /**
  Create the vtkStructuredGrid from the provided workspace
  @param progressUpdating: Reporting object to pass progress information up the stack.
  @return fully constructed vtkDataSet.
  */
  vtkDataSet* vtkMDHistoHexFactory::create(ProgressAction& progressUpdating) const
  {
    vtkDataSet* product = tryDelegatingCreation<MDHistoWorkspace, 3>(m_workspace, progressUpdating);
    if(product != NULL)
    {
      return product;
    }
    else
    {
      // Create in 3D mode
      return this->create3Dor4D(0, false, progressUpdating);
    }
  }


  vtkMDHistoHexFactory::~vtkMDHistoHexFactory()
  {
  }

}
}
