#include "MantidVatesAPI/vtkThresholdingLineFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"
#include "vtkLine.h"
#include <vector>
#include <boost/math/special_functions/fpclassify.hpp> 
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using Mantid::API::IMDWorkspace;
using Mantid::MDEvents::MDHistoWorkspace;
using Mantid::API::NullCoordTransform;

namespace Mantid
{

  namespace VATES
  {

    vtkThresholdingLineFactory::vtkThresholdingLineFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_scalarName(scalarName),
      m_thresholdRange(thresholdRange)
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
      this->m_thresholdRange = other.m_thresholdRange;
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
   this->m_thresholdRange = other.m_thresholdRange;
   this->m_workspace = other.m_workspace;
  }

    vtkDataSet* vtkThresholdingLineFactory::create() const
    {
      validate();
      //use the successor factory's creation method if this type cannot handle the dimensionality of the workspace.
      size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
      if((doesCheckDimensionality() && nonIntegratedSize != vtkDataSetFactory::OneDimensional))
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

        UnstructuredPoint unstructPoint;
        float signalScalar;
        const int nPointsX = nBinsX;
        Column column(nPointsX);

        NullCoordTransform transform;
        //Mantid::API::CoordTransform* transform = m_workspace->getTransformFromOriginal();
        Mantid::coord_t in[3]; 
        Mantid::coord_t out[3];

        //Loop through dimensions
        for (int i = 0; i < nPointsX; i++)
        {
          in[0] = minX + (i * incrementX); //Calculate increment in x;
          
          signalScalar = static_cast<float>(m_workspace->getSignalNormalizedAt(i));

            if (boost::math::isnan( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
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

            transform.apply(in, out);

            unstructPoint.pointId = points->InsertNextPoint(out);
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

    void vtkThresholdingLineFactory::initialize(Mantid::API::Workspace_sptr wspace_sptr)
    {
      m_workspace = boost::dynamic_pointer_cast<MDHistoWorkspace>(wspace_sptr);
      validate();
      // When the workspace can not be handled by this type, take action in the form of delegation.
      size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
      if((doesCheckDimensionality() && nonIntegratedSize != vtkDataSetFactory::OneDimensional))
      {
        if(this->hasSuccessor())
        {
          m_successor->setUseTransform(m_useTransform);
          m_successor->initialize(m_workspace);
          return;
        }
        else
        {
          throw std::runtime_error("There is no successor factory set for this vtkThresholdingLineFactory type");
        }
      }

      //Setup range values according to whatever strategy object has been injected.
      m_thresholdRange->setWorkspace(m_workspace);
      m_thresholdRange->calculate();
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
