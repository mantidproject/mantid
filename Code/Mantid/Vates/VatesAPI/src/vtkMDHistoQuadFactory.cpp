#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkQuad.h"
#include "vtkSmartPointer.h" 
#include <vector>
#include "MantidKernel/ReadLock.h"

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using Mantid::MDEvents::MDHistoWorkspace;

namespace Mantid
{

  namespace VATES
  {

    vtkMDHistoQuadFactory::vtkMDHistoQuadFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_scalarName(scalarName), m_thresholdRange(thresholdRange)
    {
    }

    /**
    Assigment operator
    @param other : vtkMDHistoQuadFactory to assign to this instance from.
    @return ref to assigned current instance.
    */
    vtkMDHistoQuadFactory& vtkMDHistoQuadFactory::operator=(const vtkMDHistoQuadFactory& other)
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
    vtkMDHistoQuadFactory::vtkMDHistoQuadFactory(const vtkMDHistoQuadFactory& other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_thresholdRange = other.m_thresholdRange;
      this->m_workspace = other.m_workspace;
    }

    /**
    Create the vtkStructuredGrid from the provided workspace
    @param progressUpdating: Reporting object to pass progress information up the stack.
    @return fully constructed vtkDataSet.
    */
    vtkDataSet* vtkMDHistoQuadFactory::create(ProgressAction& progressUpdating) const
    {
      vtkDataSet* product = tryDelegatingCreation<MDHistoWorkspace, 2>(m_workspace, progressUpdating);
      if(product != NULL)
      {
        return product;
      }
      else
      {
        Mantid::Kernel::ReadLock lock(*m_workspace);
        CPUTimer tim;
        const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
        const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );

        const coord_t maxX = m_workspace-> getXDimension()->getMaximum();
        const coord_t minX = m_workspace-> getXDimension()->getMinimum();
        const coord_t maxY = m_workspace-> getYDimension()->getMaximum();
        const coord_t minY = m_workspace-> getYDimension()->getMinimum();

        coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
        coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);

        const int imageSize = (nBinsX ) * (nBinsY );
        vtkPoints *points = vtkPoints::New();
        points->Allocate(static_cast<int>(imageSize));

        vtkFloatArray * signal = vtkFloatArray::New();
        signal->Allocate(imageSize);
        signal->SetName(m_scalarName.c_str());
        signal->SetNumberOfComponents(1);

        //The following represent actual calculated positions.

        float signalScalar;
        const int nPointsX = nBinsX+1;
        const int nPointsY = nBinsY+1;

        /* The idea of the next chunk of code is that you should only
        create the points that will be needed; so an array of pointNeeded
        is set so that all required vertices are marked, and created in a second step. */

        // Array of the points that should be created, set to false
        bool * pointNeeded = new bool[nPointsX*nPointsY];
        memset(pointNeeded, 0, nPointsX*nPointsY*sizeof(bool));
        // Array with true where the voxel should be shown
        bool * voxelShown = new bool[nBinsX*nBinsY];

        double progressFactor = 0.5/double(nBinsX);
        double progressOffset = 0.5;

        size_t index = 0;
        for (int i = 0; i < nBinsX; i++)
        {
          progressUpdating.eventRaised(progressFactor*double(i));

          for (int j = 0; j < nBinsY; j++)
          {
            index = j + nBinsY*i;
            signalScalar = static_cast<float>(m_workspace->getSignalNormalizedAt(i, j));
            if (isSpecial( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
            {
              // out of range
              voxelShown[index] = false;
            }
            else
            {
              // Valid data
              voxelShown[index] = true;
              signal->InsertNextValue(static_cast<float>(signalScalar));
              // Make sure all 4 neighboring points are set to true
              size_t pointIndex = i * nPointsY + j;
              pointNeeded[pointIndex] = true;  pointIndex++;
              pointNeeded[pointIndex] = true;  pointIndex += nPointsY-1;
              pointNeeded[pointIndex] = true;  pointIndex++;
              pointNeeded[pointIndex] = true;
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
        in[2] = 0;

        // Array with the point IDs (only set where needed)
        vtkIdType * pointIDs = new vtkIdType[nPointsX*nPointsY];
        index = 0;
        for (int i = 0; i < nPointsX; i++)
        {
          progressUpdating.eventRaised((progressFactor*double(i)) + progressOffset);
          in[0] = minX + (static_cast<coord_t>(i) * incrementX); //Calculate increment in x;
          for (int j = 0; j < nPointsY; j++)
          {
            // Create the point only when needed
            if (pointNeeded[index])
            {
              in[1] = minY + (static_cast<coord_t>(j) * incrementY); //Calculate increment in y;
              if (transform)
              {
                transform->apply(in, out);
                pointIDs[index] = points->InsertNextPoint(out);
              }
              else
                pointIDs[index] = points->InsertNextPoint(in);
            }
            index++;
          }
        }

        std::cout << tim << " to create the needed points." << std::endl;

        vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
        visualDataSet->Allocate(imageSize);
        visualDataSet->SetPoints(points);
        visualDataSet->GetCellData()->SetScalars(signal);

        // ------ Quad creation ----------------
        vtkQuad* quad = vtkQuad::New(); // Significant speed increase by creating ONE quad
        index = 0;
        for (int i = 0; i < nBinsX; i++)
        {
          for (int j = 0; j < nBinsY; j++)
          {
            if (voxelShown[index])
            {
              // The quad will be shown
              quad->GetPointIds()->SetId(0, pointIDs[(i)*nPointsY + j]);
              quad->GetPointIds()->SetId(1, pointIDs[(i+1)*nPointsY + j]);
              quad->GetPointIds()->SetId(2, pointIDs[(i+1)*nPointsY + j+1]);
              quad->GetPointIds()->SetId(3, pointIDs[(i)*nPointsY + j+1]);
              visualDataSet->InsertNextCell(VTK_QUAD, quad->GetPointIds());
            }
            index++;
          }
        }
        quad->Delete();

        std::cout << tim << " to create and add the quads." << std::endl;

        points->Delete();
        signal->Delete();
        visualDataSet->Squeeze();
        delete [] pointIDs;
        delete [] voxelShown;
        delete [] pointNeeded;

        // Hedge against empty data sets
        if (visualDataSet->GetNumberOfPoints() <= 0)
        {
          visualDataSet->Delete();
          vtkNullUnstructuredGrid nullGrid;
          visualDataSet = nullGrid.createNullData();
        }

        return visualDataSet;
      }
    }

    void vtkMDHistoQuadFactory::initialize(Mantid::API::Workspace_sptr wspace_sptr)
    {
      m_workspace = doInitialize<MDHistoWorkspace, 2>(wspace_sptr);

      //Setup range values according to whatever strategy object has been injected.
      m_thresholdRange->setWorkspace(wspace_sptr);
      m_thresholdRange->calculate();
    }

    void vtkMDHistoQuadFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("IMDWorkspace is null");
      }
    }

    /// Destructor
    vtkMDHistoQuadFactory::~vtkMDHistoQuadFactory()
    {

    }
  }
}
