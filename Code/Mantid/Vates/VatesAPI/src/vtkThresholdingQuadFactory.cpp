#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkQuad.h"
#include "vtkSmartPointer.h" 
#include <boost/math/special_functions/fpclassify.hpp>
#include <vector>

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using Mantid::MDEvents::MDHistoWorkspace;

namespace Mantid
{

  namespace VATES
  {

    vtkThresholdingQuadFactory::vtkThresholdingQuadFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_scalarName(scalarName), m_thresholdRange(thresholdRange)
    {
    }

          /**
  Assigment operator
  @param other : vtkThresholdingQuadFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  vtkThresholdingQuadFactory& vtkThresholdingQuadFactory::operator=(const vtkThresholdingQuadFactory& other)
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
  vtkThresholdingQuadFactory::vtkThresholdingQuadFactory(const vtkThresholdingQuadFactory& other)
  {
   this->m_scalarName = other.m_scalarName;
   this->m_thresholdRange = other.m_thresholdRange;
   this->m_workspace = other.m_workspace;
  }

    vtkDataSet* vtkThresholdingQuadFactory::create() const
    {
      validate();
      //use the successor factory's creation method if this type cannot handle the dimensionality of the workspace.
      const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
      if((doesCheckDimensionality() && nonIntegratedSize != vtkDataSetFactory::TwoDimensional))
      {
        return m_successor->create();
      }
      else
      {
        CPUTimer tim;
        const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
        const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );

        const double maxX = m_workspace-> getXDimension()->getMaximum();
        const double minX = m_workspace-> getXDimension()->getMinimum();
        const double maxY = m_workspace-> getYDimension()->getMaximum();
        const double minY = m_workspace-> getYDimension()->getMinimum();

        double incrementX = (maxX - minX) / (nBinsX);
        double incrementY = (maxY - minY) / (nBinsY);

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



        size_t index = 0;
        for (int i = 0; i < nBinsX; i++)
        {
          for (int j = 0; j < nBinsY; j++)
          {
            index = j + nBinsY*i;
            signalScalar = static_cast<float>(m_workspace->getSignalNormalizedAt(i, j));
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
          in[0] = minX + (i * incrementX); //Calculate increment in x;
          for (int j = 0; j < nPointsY; j++)
          {
            // Create the point only when needed
            if (pointNeeded[index])
            {
              in[1] = minY + (j * incrementY); //Calculate increment in y;
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

        return visualDataSet;
      }
    }

    void vtkThresholdingQuadFactory::initialize(Mantid::API::Workspace_sptr wspace_sptr)
    {
      m_workspace = boost::dynamic_pointer_cast<MDHistoWorkspace>(wspace_sptr);
      validate();
      // When the workspace can not be handled by this type, take action in the form of delegation.
      const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
      if((doesCheckDimensionality() && nonIntegratedSize != vtkDataSetFactory::TwoDimensional))
      {
        if(this->hasSuccessor())
        {
          m_successor->setUseTransform(m_useTransform);
          m_successor->initialize(m_workspace);
          return;
        }
        else
        {
          throw std::runtime_error("There is no successor factory set for this vtkThresholdingQuadFactory type");
        }
      }

      //Setup range values according to whatever strategy object has been injected.
      m_thresholdRange->setWorkspace(m_workspace);
      m_thresholdRange->calculate();
    }

    void vtkThresholdingQuadFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("IMDWorkspace is null");
      }
    }

    vtkThresholdingQuadFactory::~vtkThresholdingQuadFactory()
    {

    }
  }
}
