#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"
#include "vtkLine.h"
#include <vector>
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/ReadLock.h"

using Mantid::API::IMDWorkspace;
using Mantid::DataObjects::MDHistoWorkspace;
using Mantid::API::NullCoordTransform;

namespace Mantid
{

  namespace VATES
  {

    vtkMDHistoLineFactory::vtkMDHistoLineFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_scalarName(scalarName),
      m_thresholdRange(thresholdRange)
    {
    }

    /**
    Assigment operator
    @param other : vtkMDHistoLineFactory to assign to this instance from.
    @return ref to assigned current instance.
    */
    vtkMDHistoLineFactory& vtkMDHistoLineFactory::operator=(const vtkMDHistoLineFactory& other)
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
    vtkMDHistoLineFactory::vtkMDHistoLineFactory(const vtkMDHistoLineFactory& other)
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
    vtkDataSet* vtkMDHistoLineFactory::create(ProgressAction& progressUpdating) const
    {
      vtkDataSet* product = tryDelegatingCreation<MDHistoWorkspace, 1>(m_workspace, progressUpdating);
      if(product != NULL)
      {
        return product;
      }
      else
      {
        Mantid::Kernel::ReadLock lock(*m_workspace);
        const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );

        const coord_t maxX = m_workspace-> getXDimension()->getMaximum();
        const coord_t minX = m_workspace-> getXDimension()->getMinimum();

        coord_t incrementX = (maxX - minX) / coord_t(nBinsX-1);

        const int imageSize = nBinsX;
        vtkPoints *points = vtkPoints::New();
        points->Allocate(static_cast<int>(imageSize));

        vtkFloatArray * signal = vtkFloatArray::New();
        signal->Allocate(imageSize);
        signal->SetName(m_scalarName.c_str());
        signal->SetNumberOfComponents(1);

        UnstructuredPoint unstructPoint;
        const int nPointsX = nBinsX;
        Column column(nPointsX);

        NullCoordTransform transform;
        //Mantid::API::CoordTransform* transform = m_workspace->getTransformFromOriginal();
        Mantid::coord_t in[3]; 
        Mantid::coord_t out[3];

        double progressFactor = 0.5/double(nBinsX);
        double progressOffset = 0.5;

        //Loop through dimensions
        for (int i = 0; i < nPointsX; i++)
        {
          progressUpdating.eventRaised(progressFactor * double(i));
          in[0] = minX + static_cast<coord_t>(i) * incrementX; //Calculate increment in x;

          float signalScalar = static_cast<float>(m_workspace->getSignalNormalizedAt(i));

          if (isSpecial( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
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
          progressUpdating.eventRaised((progressFactor * double(i)) + progressOffset);
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

    void vtkMDHistoLineFactory::initialize(Mantid::API::Workspace_sptr wspace_sptr)
    {
      m_workspace = this->doInitialize<MDHistoWorkspace, 1>(wspace_sptr);

      //Setup range values according to whatever strategy object has been injected.
      m_thresholdRange->setWorkspace(wspace_sptr);
      m_thresholdRange->calculate();
    }

    void vtkMDHistoLineFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("IMDWorkspace is null");
      }
    }

    vtkMDHistoLineFactory::~vtkMDHistoLineFactory()
    {

    }
  }
}
