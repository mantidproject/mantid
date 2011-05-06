#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"
#include "vtkQuad.h"
#include "MantidGeometry/MDGeometry/Coordinate.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include <vector>
#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid
{

  namespace VATES
  {

    vtkThresholdingQuadFactory::vtkThresholdingQuadFactory(const std::string& scalarName, double minThreshold, double maxThreshold) : m_scalarName(scalarName),
      m_minThreshold(minThreshold), m_maxThreshold(maxThreshold)
    {
    }

    vtkDataSet* vtkThresholdingQuadFactory::create() const
    {
      validate();
      //use the successor factory's creation method if this type cannot handle the dimensionality of the workspace.
      if(m_workspace->getNumDims() != 2)
      {
        return m_successor->create();
      }
      else
      {
        const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
        const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );

        const double maxX = m_workspace-> getXDimension()->getMaximum();
        const double minX = m_workspace-> getXDimension()->getMinimum();
        const double maxY = m_workspace-> getYDimension()->getMaximum();
        const double minY = m_workspace-> getYDimension()->getMinimum();

        double incrementX = (maxX - minX) / (nBinsX-1);
        double incrementY = (maxY - minY) / (nBinsY-1);

        const int imageSize = (nBinsX ) * (nBinsY );
        vtkPoints *points = vtkPoints::New();
        points->Allocate(static_cast<int>(imageSize));

        vtkFloatArray * signal = vtkFloatArray::New();
        signal->Allocate(imageSize);
        signal->SetName(m_scalarName.c_str());
        signal->SetNumberOfComponents(1);

        //The following represent actual calculated positions.
        double posX, posY;

        UnstructuredPoint unstructPoint;
        double signalScalar;
        const int nPointsX = nBinsX;
        const int nPointsY = nBinsY;
        Plane plane(nPointsX);

        //Loop through dimensions
        for (int i = 0; i < nPointsX; i++)
        {
          posX = minX + (i * incrementX); //Calculate increment in x;
          Column column(nPointsY);
          for (int j = 0; j < nPointsY; j++)
          {
            posY = minY + (j * incrementY); //Calculate increment in y;

            signalScalar = m_workspace->getCell(i,j).getSignal();

            if (boost::math::isnan( signalScalar ) || (signalScalar <= m_minThreshold) || (signalScalar >= m_maxThreshold))
            {
              //Flagged so that topological and scalar data is not applied.
              unstructPoint.isSparse = true;
            }
            else
            {
              if ((i < (nBinsX -1)) && (j < (nBinsY - 1)))
              {
                signal->InsertNextValue(signalScalar);
              }
              unstructPoint.isSparse = false;
            }
            unstructPoint.pointId = points->InsertNextPoint(posX, posY, 0);
            column[j] = unstructPoint;

          }
          plane[i] = column;
        }

        points->Squeeze();
        signal->Squeeze();

        vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
        visualDataSet->Allocate(imageSize);
        visualDataSet->SetPoints(points);
        visualDataSet->GetCellData()->SetScalars(signal);

        for (int i = 0; i < nBinsX - 1; i++)
        {
          for (int j = 0; j < nBinsY -1; j++)
          {
            //Only create topologies for those cells which are not sparse.
            if (!plane[i][j].isSparse)
            {
              vtkQuad* quad = vtkQuad::New();
              quad->GetPointIds()->SetId(0, plane[i][j].pointId);
              quad->GetPointIds()->SetId(1, plane[i + 1][j].pointId);
              quad->GetPointIds()->SetId(2, plane[i + 1][j + 1].pointId); 
              quad->GetPointIds()->SetId(3, plane[i][j + 1].pointId);
              visualDataSet->InsertNextCell(VTK_QUAD, quad->GetPointIds());
            }

          }
        }

        points->Delete();
        signal->Delete();
        visualDataSet->Squeeze();
        return visualDataSet;
      }
    }

    vtkDataSet* vtkThresholdingQuadFactory::createMeshOnly() const
    {
      throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
    }

    vtkFloatArray* vtkThresholdingQuadFactory::createScalarArray() const
    {
      throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
    }

    void vtkThresholdingQuadFactory::initialize(Mantid::API::IMDWorkspace_sptr wspace_sptr)
    {
      m_workspace = wspace_sptr;
      validate();
      // When the workspace can not be handled by this type, take action in the form of delegation.
      if(m_workspace->getNumDims() != 2)
      {
        if(this->hasSuccessor())
        {
          m_successor->initialize(m_workspace);
          return;
        }
        else
        {
          throw std::runtime_error("There is no successor factory set for this vtkThresholdingQuadFactory type");
        }
      }
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