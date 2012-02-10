#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/CoordTransform.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkQuad.h>
#include <vtkCellData.h>
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid::API;

namespace Mantid
{
  namespace VATES
  {
    /// Constructor
    vtkMDQuadFactory::vtkMDQuadFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_thresholdRange(thresholdRange), m_scalarName(scalarName)
    {
    }

    /// Destructor
    vtkMDQuadFactory::~vtkMDQuadFactory()
    {
    }

    /// Factory Method. Should also handle delegation to successors.
    vtkDataSet* vtkMDQuadFactory::create() const
    {
      validate();
      IMDEventWorkspace_sptr imdws = boost::dynamic_pointer_cast<IMDEventWorkspace>(m_workspace);
      if(!imdws || (doesCheckDimensionality() && imdws->getNonIntegratedDimensions().size() != TwoDimensional))
      {
        return m_successor->create();
      }

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

      //Exact number of boxes given. Possible improvement - get this info via IMDMethods instead of IMDEventWorkspace methods to keep this generic.
      const size_t maxSize = imdws->getBoxController()->getTotalNumMDBoxes();
     
      // Create 4 points per box.
      vtkPoints *points = vtkPoints::New();
      points->SetNumberOfPoints(maxSize * 4);

      // One scalar per box
      vtkFloatArray * signals = vtkFloatArray::New();
      signals->Allocate(maxSize);
      signals->SetName(m_scalarName.c_str());
      signals->SetNumberOfComponents(1);

      size_t nVertexes;
      //Ensure destruction in any event.
      boost::scoped_ptr<IMDIterator> it(imdws->createIterator());

      vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
      visualDataSet->Allocate(maxSize);

      vtkIdList * quadPointList = vtkIdList::New();
      quadPointList->SetNumberOfIds(4);

      Mantid::API::CoordTransform* transform = NULL;
      if (m_useTransform)
      {
        transform = imdws->getTransformToOriginal();
      }

      Mantid::coord_t out[2];
      bool* useBox = new bool[maxSize];

      for(size_t iBox = 0; iBox < maxSize; ++iBox)
      {
        Mantid::signal_t signal_normalized= it->getNormalizedSignal();
        if (!boost::math::isnan( signal_normalized ) && m_thresholdRange->inRange(signal_normalized))
        {
          useBox[iBox] = true;
          signals->InsertNextValue(static_cast<float>(signal_normalized));

          coord_t* coords = it->getVertexesArray(nVertexes, nNonIntegrated, masks);
          delete [] coords;
          coords = it->getVertexesArray(nVertexes, nNonIntegrated, masks);

          //Iterate through all coordinates. Candidate for speed improvement.
          for(size_t v = 0; v < nVertexes; ++v)
          {
            coord_t * coord = coords + v*2;
            size_t id = iBox*4 + v;
            if(m_useTransform)
            {
              transform->apply(coord, out);
              points->SetPoint(id, out[0], out[1], 0);
            }
            else
            {
              points->SetPoint(id, coord[0], coord[1], 0);
            }
          }
          // Free memory
          delete [] coords;
        } // valid number of vertexes returned
        else
        {
          useBox[iBox] = false;
        }
        it->next();
      }

      delete[] masks;
      for(size_t ii = 0; ii < maxSize ; ++ii)
      {

        if (useBox[ii] == true)
        {
          vtkIdType pointIds = ii * 4;

          quadPointList->SetId(0, pointIds + 0); //xyx
          quadPointList->SetId(1, pointIds + 1); //dxyz
          quadPointList->SetId(2, pointIds + 3); //dxdyz
          quadPointList->SetId(3, pointIds + 2); //xdyz
          visualDataSet->InsertNextCell(VTK_QUAD, quadPointList);
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
      quadPointList->Delete();

      return visualDataSet;
    }

    /// Initalize with a target workspace.
    void vtkMDQuadFactory::initialize(Mantid::API::Workspace_sptr ws)
    {
      m_workspace = ws;
      IMDEventWorkspace_sptr imdws = boost::dynamic_pointer_cast<IMDEventWorkspace>(m_workspace);
      if(!imdws || (doesCheckDimensionality() && imdws->getNonIntegratedDimensions().size() != TwoDimensional))
      {
        if(this->hasSuccessor())
        {
          m_successor->setUseTransform(m_useTransform);
          m_successor->initialize(ws);
        }
        else
        {
          throw std::runtime_error("vtkMDQuadFactory has no successor");
        }
      }
    }

    /// Get the name of the type.
    std::string vtkMDQuadFactory::getFactoryTypeName() const
    {
      return "vtkMDQuadFactory";
    }

    /// Template Method pattern to validate the factory before use.
    void vtkMDQuadFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("vtkMDQuadFactory has no workspace to run against");
      }
    }

  }
}