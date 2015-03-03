#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/CoordTransform.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkLine.h>
#include <vtkCellData.h>
#include "MantidKernel/ReadLock.h"

using namespace Mantid::API;

namespace Mantid
{
  namespace VATES
  {
    /**
    Constructor
    @param thresholdRange : Thresholding range functor
    @param scalarName : Name to give to signal
    */
    vtkMDLineFactory::vtkMDLineFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_thresholdRange(thresholdRange), m_scalarName(scalarName)
    {
    }

    /// Destructor
    vtkMDLineFactory::~vtkMDLineFactory()
    {
    }

    /**
    Create the vtkStructuredGrid from the provided workspace
    @param progressUpdating: Reporting object to pass progress information up the stack.
    @return fully constructed vtkDataSet.
    */
    vtkDataSet* vtkMDLineFactory::create(ProgressAction& progressUpdating) const
    {
      vtkDataSet* product = tryDelegatingCreation<IMDEventWorkspace, 1>(m_workspace, progressUpdating);
      if(product != NULL)
      {
        return product;
      }
      else
      {
        IMDEventWorkspace_sptr imdws = doInitialize<IMDEventWorkspace, 1>(m_workspace);
        // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
        Mantid::Kernel::ReadLock lock(*imdws);

        const size_t nDims = imdws->getNumDims();
        size_t nNonIntegrated = imdws->getNonIntegratedDimensions().size();

        /*
        Write mask array with correct order for each internal dimension.
        */
        bool* masks = new bool[nDims];
        for(size_t i_dim = 0; i_dim < nDims; ++i_dim)
        {
          bool bIntegrated = imdws->getDimension(i_dim)->getIsIntegrated();
          masks[i_dim] = !bIntegrated; //TRUE for unmaksed, integrated dimensions are masked.
        }
        
        //Ensure destruction in any event.
        boost::scoped_ptr<IMDIterator> it(imdws->createIterator());

        // Create 2 points per box.
        vtkPoints *points = vtkPoints::New();
        points->SetNumberOfPoints(it->getDataSize() * 2);

        // One scalar per box
        vtkFloatArray * signals = vtkFloatArray::New();
        signals->Allocate(it->getDataSize());
        signals->SetName(m_scalarName.c_str());
        signals->SetNumberOfComponents(1);

        size_t nVertexes;
        
        vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
        visualDataSet->Allocate(it->getDataSize());

        vtkIdList * linePointList = vtkIdList::New();
        linePointList->SetNumberOfIds(2);

        Mantid::API::CoordTransform* transform = NULL;
        if (m_useTransform)
        {
          transform = imdws->getTransformToOriginal();
        }

        Mantid::coord_t out[1];
        bool* useBox = new bool[it->getDataSize()];

        double progressFactor = 0.5/double(it->getDataSize());
        double progressOffset = 0.5;

        size_t iBox = 0;
        do
        {
          progressUpdating.eventRaised(double(iBox)*progressFactor);

          Mantid::signal_t signal_normalized= it->getNormalizedSignal();
          if (!isSpecial( signal_normalized ) && m_thresholdRange->inRange(signal_normalized))
          {
            useBox[iBox] = true;
            signals->InsertNextValue(static_cast<float>(signal_normalized));

            coord_t* coords = it->getVertexesArray(nVertexes, nNonIntegrated, masks);
            delete [] coords;
            coords = it->getVertexesArray(nVertexes, nNonIntegrated, masks);

            //Iterate through all coordinates. Candidate for speed improvement.
            for(size_t v = 0; v < nVertexes; ++v)
            {
              coord_t * coord = coords + v*1;
              size_t id = iBox*2 + v;
              if(m_useTransform)
              {
                transform->apply(coord, out);
                points->SetPoint(id, out[0], 0, 0);
              }
              else
              {
                points->SetPoint(id, coord[0], 0, 0);
              }
            }
            // Free memory
            delete [] coords;
          } // valid number of vertexes returned
          else
          {
            useBox[iBox] = false;
          }
          ++iBox;
        } while (it->next());

        delete[] masks;
        for(size_t ii = 0; ii < it->getDataSize() ; ++ii)
        {
          progressUpdating.eventRaised((double(ii)*progressFactor) + progressOffset);

          if (useBox[ii] == true)
          {
            vtkIdType pointIds = ii * 2;

            linePointList->SetId(0, pointIds + 0); //xyx
            linePointList->SetId(1, pointIds + 1); //dxyz
            visualDataSet->InsertNextCell(VTK_LINE, linePointList);
          } // valid number of vertexes returned
        }

        delete[] useBox;

        signals->Squeeze();
        points->Squeeze();

        visualDataSet->SetPoints(points);
        visualDataSet->GetCellData()->SetScalars(signals);
        visualDataSet->Squeeze();

        signals->Delete();
        points->Delete();
        linePointList->Delete();

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

    /// Initalize with a target workspace.
    void vtkMDLineFactory::initialize(Mantid::API::Workspace_sptr ws)
    {
      m_workspace = doInitialize<IMDEventWorkspace, 1>(ws);
    }

    /// Get the name of the type.
    std::string vtkMDLineFactory::getFactoryTypeName() const
    {
      return "vtkMDLineFactory";
    }

    /// Template Method pattern to validate the factory before use.
    void vtkMDLineFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("vtkMDLineFactory has no workspace to run against");
      }
    }

  }
}
