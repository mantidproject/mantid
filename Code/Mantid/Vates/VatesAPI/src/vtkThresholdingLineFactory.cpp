#include "MantidVatesAPI/vtkThresholdingLineFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"
#include "vtkLine.h"
#include "MantidGeometry/MDGeometry/Coordinate.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include <vector>
#include <boost/math/special_functions/fpclassify.hpp> 

namespace Mantid
{

  namespace VATES
  {

    vtkThresholdingLineFactory::vtkThresholdingLineFactory(const std::string& scalarName, double minThreshold, double maxThreshold) : m_scalarName(scalarName),
      m_minThreshold(minThreshold), m_maxThreshold(maxThreshold)
    {
    }

      /**
  Assigment operator
  @param other : vtkThresholdingLineFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  vtkThresholdingLineFactory& vtkThresholdingLineFactory::operator=(const vtkThresholdingLineFactory& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_minThreshold = other.m_minThreshold;
      this->m_maxThreshold = other.m_maxThreshold;
      this->m_workspace = other.m_workspace;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  vtkThresholdingLineFactory::vtkThresholdingLineFactory(const vtkThresholdingLineFactory& other)
  {
   this->m_scalarName = other.m_scalarName;
   this->m_minThreshold = other.m_minThreshold;
   this->m_maxThreshold = other.m_maxThreshold;
   this->m_workspace = other.m_workspace;
  }

    vtkDataSet* vtkThresholdingLineFactory::create() const
    {
      validate();
      //use the successor factory's creation method if this type cannot handle the dimensionality of the workspace.
      if(m_workspace->getNumDims() != 1)
      {
        return m_successor->create();
      }
      else
      {
        const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );

        const double maxX = m_workspace-> getXDimension()->getMaximum();
        const double minX = m_workspace-> getXDimension()->getMinimum();

        double incrementX = (maxX - minX) / (nBinsX-1);

        const int imageSize = nBinsX;
        vtkPoints *points = vtkPoints::New();
        points->Allocate(static_cast<int>(imageSize));

        vtkFloatArray * signal = vtkFloatArray::New();
        signal->Allocate(imageSize);
        signal->SetName(m_scalarName.c_str());
        signal->SetNumberOfComponents(1);

        //The following represent actual calculated positions.
        double posX;

        UnstructuredPoint unstructPoint;
        double signalScalar;
        const int nPointsX = nBinsX;
        Column column(nPointsX);

        //Loop through dimensions
        for (int i = 0; i < nPointsX; i++)
        {
          posX = minX + (i * incrementX); //Calculate increment in x;
          

            signalScalar = m_workspace->getCell(i).getSignal();

            if (boost::math::isnan( signalScalar ) || (signalScalar <= m_minThreshold) || (signalScalar >= m_maxThreshold))
            {
              //Flagged so that topological and scalar data is not applied.
              unstructPoint.isSparse = true;
            }
            else
            {
              if (i < (nBinsX -1))
              {
                signal->InsertNextValue(static_cast<float>(signalScalar));
              }
              unstructPoint.isSparse = false;
            }
            unstructPoint.pointId = points->InsertNextPoint(posX, 0, 0);
            column[i] = unstructPoint;

          }
        
        points->Squeeze();
        signal->Squeeze();

        vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
        visualDataSet->Allocate(imageSize);
        visualDataSet->SetPoints(points);
        visualDataSet->GetCellData()->SetScalars(signal);

        for (int i = 0; i < nBinsX - 1; i++)
        {
            //Only create topologies for those cells which are not sparse.
            if (!column[i].isSparse)
            {
              vtkLine* line = vtkLine::New();
              line->GetPointIds()->SetId(0, column[i].pointId);
              line->GetPointIds()->SetId(1, column[i + 1].pointId);
              visualDataSet->InsertNextCell(VTK_LINE, line->GetPointIds());
            }
        }

        points->Delete();
        signal->Delete();
        visualDataSet->Squeeze();
        return visualDataSet;
      }
    }

    vtkDataSet* vtkThresholdingLineFactory::createMeshOnly() const
    {
      throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
    }

    vtkFloatArray* vtkThresholdingLineFactory::createScalarArray() const
    {
      throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
    }

    void vtkThresholdingLineFactory::initialize(Mantid::API::IMDWorkspace_sptr wspace_sptr)
    {
      m_workspace = wspace_sptr;
      validate();
      // When the workspace can not be handled by this type, take action in the form of delegation.
      if(m_workspace->getNumDims() != 1)
      {
        if(this->hasSuccessor())
        {
          m_successor->initialize(m_workspace);
          return;
        }
        else
        {
          throw std::runtime_error("There is no successor factory set for this vtkThresholdingLineFactory type");
        }
      }
    }

    void vtkThresholdingLineFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("IMDWorkspace is null");
      }
    }

    vtkThresholdingLineFactory::~vtkThresholdingLineFactory()
    {

    }
  }
}
