#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/SynchronisingGeometryPresenter.h"
#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/GeometryView.h"
#include <algorithm>
namespace Mantid
{
  namespace VATES
  {
    /// Comparitor to find DimensionPresenter shared pointers via a dimension id.
    struct FindId : public std::unary_function <DimPresenter_sptr, bool>
    {
      const std::string m_id;
      FindId(const std::string id) : m_id(id){ }

      bool operator ()(const DimPresenter_sptr obj) const
      {
        return m_id == obj->getModel()->getDimensionId();
      }
      FindId& operator=(const  FindId&);
    };

    /// Comparitor to find IMDDimension shared pointers via a dimension id.
    struct FindModelId : public std::unary_function <Mantid::Geometry::IMDDimension_sptr, bool>
    {
      const std::string m_id;
      FindModelId(const std::string id) : m_id(id){ }

      bool operator ()(const Mantid::Geometry::IMDDimension_sptr obj) const
      {
        return m_id == obj->getDimensionId();
      }
      FindModelId& operator=(const  FindModelId&);
    };

    /**
    Constructor
    */
    SynchronisingGeometryPresenter::SynchronisingGeometryPresenter(Mantid::Geometry::MDGeometryXMLParser& source) : 
      m_notIntegrated(source.getNonIntegratedDimensions()), 
      m_integrated(source.getIntegratedDimensions()),
      m_source(source)
    {

    }

    /**
    Destructor
    */
    SynchronisingGeometryPresenter::~SynchronisingGeometryPresenter()
    {
    }

    /**
    Handles dimension realignment. When a dimension presenter is handling a realignment, its is necessary for this to be synchronsied with other non-integrated dimensions.
    @parameter pDimensionPresenter : dimension presenter on which realignement has been requested.
    */
    void SynchronisingGeometryPresenter::dimensionRealigned(DimensionPresenter* pDimensionPresenter)
    {
      VecDimPresenter_sptr::iterator pos = find_if (m_dimPresenters.begin(), m_dimPresenters.end(), FindId( pDimensionPresenter->getDimensionId()));
      Mantid::Geometry::IMDDimension_sptr temp = (*pos)->getModel();
      (*pos)->acceptModel(pDimensionPresenter->getModel());
      pDimensionPresenter->acceptModel(temp);
      //Pass on via-view that clipping boxes should be disregarded.
      m_view->raiseNoClipping();
    }

    /**
    Ensure that for non-integrated dimensions, mappings are always occupied in the priority x before y before z before t.
    */
    void SynchronisingGeometryPresenter::shuffleMappedPresenters()
    {
      DimPresenter_sptr temp;
      if(hasYDim() && !hasXDim())
      {   
        temp = m_yDim;
        eraseMappedPresenter(m_yDim);
        m_xDim = temp;
      }
      if(hasZDim() && !hasYDim())
      {
        temp = m_zDim;
        eraseMappedPresenter(m_zDim);
        m_yDim = temp;
      }
      if(hasTDim() && !hasZDim())
      {
        temp = m_tDim;
        eraseMappedPresenter(m_tDim);
        m_zDim = temp;
      }
    }

    /**
    Ensure that for the collaped mapeed dimension, it's mapped placeholder is erased (marked as empty).
    @parameter expiredMappedDimension : mapped dimension presenter which has been collapsed, and may currently occupy a x, y, z, t mapping.
    */
    void SynchronisingGeometryPresenter::eraseMappedPresenter(DimPresenter_sptr expiredMappedDimension)
    {
      DimensionPresenter* pNull = NULL;
      DimPresenter_sptr nullDim(pNull);
      if(isXDimensionPresenter(expiredMappedDimension.get()))
      {
        m_xDim = nullDim;
      }
      else if(isYDimensionPresenter(expiredMappedDimension.get()))
      {
        m_yDim = nullDim;
      }
      else if(isZDimensionPresenter(expiredMappedDimension.get()))
      {
        m_zDim = nullDim;
      }
      else if(isTDimensionPresenter(expiredMappedDimension.get()))
      {
        m_tDim = nullDim;
      }
    }

    /**
    With the priority mapping of x before y, y before z, and z before t. Ensure that a candidate mapped dimension presenter is set to occupy a vacent mapping.
    @parameter candidateMappedDimension : Dimension presenter to which a mapping is requested.
    */
    void SynchronisingGeometryPresenter::insertMappedPresenter(DimPresenter_sptr candidateMappedDimension)
    {
      if(!hasXDim())
      {
        m_xDim = candidateMappedDimension;
      }
      else if(!hasYDim())
      {
        m_yDim = candidateMappedDimension;
      }
      else if(!hasZDim())
      {
        m_zDim = candidateMappedDimension;
      }
      else if(!hasTDim())
      {
        m_tDim = candidateMappedDimension;
      }
    }

    /**
    Handles the change of a managed dimension presenter to be expanded (from collapsed). 
    @parameter pDimensionPresenter : dimension which is now expanded.
    */
    void SynchronisingGeometryPresenter::dimensionExpanded(DimensionPresenter* pDimensionPresenter)
    {
      Mantid::Geometry::VecIMDDimension_sptr::iterator it = std::find_if(m_notIntegrated.begin(), m_notIntegrated.end(), FindModelId(pDimensionPresenter->getDimensionId()));
      if(it == m_notIntegrated.end())
      {
        //TODO: Need to prevent m_nonIntegrated exceeding the number of mapped dimensions! Simple check here.
        m_notIntegrated.push_back(pDimensionPresenter->getAppliedModel()); 
        m_integrated.erase(std::remove_if(m_integrated.begin(), m_integrated.end(), FindModelId(pDimensionPresenter->getDimensionId())), m_integrated.end());
        VecDimPresenter_sptr::iterator location = std::find_if(m_dimPresenters.begin(), m_dimPresenters.end(), FindId(pDimensionPresenter->getDimensionId()));
        insertMappedPresenter((*location));
        shuffleMappedPresenters();
      }
    }

    /**
    Handles the change of a managed dimension presenter to be collapsed (from expanded). 
    @parameter pDimensionPresenter : dimension which is now collapsed.
    */
    void SynchronisingGeometryPresenter::dimensionCollapsed(DimensionPresenter* pDimensionPresenter)
    {
      Mantid::Geometry::VecIMDDimension_sptr::iterator it = std::find_if(m_integrated.begin(), m_integrated.end(), FindModelId(pDimensionPresenter->getDimensionId()));
      if(it == m_integrated.end())
      {
        m_integrated.push_back(pDimensionPresenter->getAppliedModel());
        m_notIntegrated.erase(std::remove_if(m_notIntegrated.begin(), m_notIntegrated.end(), FindModelId(pDimensionPresenter->getDimensionId())), m_notIntegrated.end());

        VecDimPresenter_sptr::iterator location = std::find_if(m_dimPresenters.begin(), m_dimPresenters.end(), FindId(pDimensionPresenter->getDimensionId()));

        eraseMappedPresenter((*location));
        shuffleMappedPresenters();
      }
    }

    /**
    Handles dimension resize request. Can either be collapsed or expanded. This is worked out internally.
    @parameter pDimensionPresenter : dimension which is now collapsed/expanded.
    */
    void SynchronisingGeometryPresenter::dimensionResized(DimensionPresenter* pDimensionPresenter)
    {
      bool nowIntegrated = pDimensionPresenter->getAppliedModel()->getNBins() == 1;
      if(nowIntegrated)
      {
        dimensionCollapsed(pDimensionPresenter);
      }
      else
      {
        dimensionExpanded(pDimensionPresenter);
      }

      //For non integrated dimension presenter. Lists of possible non-interated dimensions to switch to must be updated.
      for(unsigned int i = 0; i < m_dimPresenters.size(); i++)
      {
        m_dimPresenters[i]->updateIfNotIntegrated();
      }
      pDimensionPresenter->acceptAppliedModel(); 
      //Pass on via-view that clipping boxes should be disregarded.
      m_view->raiseNoClipping();
    }

    /**
    Getter for non-integrated dimensions.
    @return collection of non-integrated dimensions.
    */
    Mantid::Geometry::VecIMDDimension_sptr SynchronisingGeometryPresenter::getNonIntegratedDimensions() const
    {
      return m_notIntegrated;
    }

    /**
    Getter for the geometry xml.
    @return GeometryXML in string format.
    */
    std::string SynchronisingGeometryPresenter::getGeometryXML() const
    {
      //Get the selected alignment for the xdimension.
      using namespace Mantid::Geometry;
      MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;

      Mantid::Geometry::VecIMDDimension_sptr::const_iterator it = m_integrated.begin();
      for(;it != m_integrated.end(); ++it)
      {
        xmlBuilder.addOrdinaryDimension(*it);
      }

      if(hasXDim())
      {
        xmlBuilder.addXDimension(m_xDim->getAppliedModel());
      }
      if(hasYDim())
      {
        xmlBuilder.addYDimension(m_yDim->getAppliedModel());
      }
      if(hasZDim())
      {
        xmlBuilder.addZDimension(m_zDim->getAppliedModel());
      }
      if(hasTDim())
      {
        xmlBuilder.addTDimension(m_tDim->getAppliedModel());
      }
      return xmlBuilder.create().c_str();
    }

    /**
    SynchronisingGeometryPresenter are constructed without first knowing the view they manage. They must be dispatched with the
    view instance they both belong to (GeometryViews own GeometryPresenters) and can direct (GeometryPresenters direct GeometryViews MVP).

    i) Uses factory provided by GeometryView to generate DimensionViews.
    ii) Creates a DimensionPresenter for each of those views and binds the pair together. (although DimensionPresenters are owned by this and DimensionViews are owned by GeometryViews)
    iv) Replicates the mappings on the original source input. These are read/writable at a later point.

    @parameter view : the GeoemtryView to direct.
    */
    void SynchronisingGeometryPresenter::acceptView(GeometryView* view)
    {
      m_view = view;
      const DimensionViewFactory& factory = m_view->getDimensionViewFactory();
      Mantid::Geometry::VecIMDDimension_sptr vecAllDimensions = m_source.getAllDimensions();
      for(size_t i =0; i < vecAllDimensions.size(); i++)
      {
        DimensionView* dimView = factory.create();
        DimPresenter_sptr dimPresenter(new DimensionPresenter(dimView, this));

        Mantid::Geometry::IMDDimension_sptr model = vecAllDimensions[i];

        if(m_source.isXDimension(model))
        {
          m_xDim = dimPresenter;
        }
        else if(m_source.isYDimension(model))
        {
          m_yDim = dimPresenter;
        }
        else if(m_source.isZDimension(model))
        {
          m_zDim = dimPresenter;
        }
        else if(m_source.isTDimension(model))
        {
          m_tDim = dimPresenter;
        }

        dimView->accept(dimPresenter.get());
        dimPresenter->acceptModel(model);
        m_view->addDimensionView(dimView);
        m_dimPresenters.push_back(dimPresenter);
      }
    }

    /**
    Determine whether x dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasXDim() const
    {
      return m_xDim.get() != NULL;
    }

    /**
    Determine whether y dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasYDim() const
    {
      return m_yDim.get() != NULL;
    }

    /**
    Determine whether z dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasZDim() const
    {
      return m_zDim.get() != NULL;
    }

    /**
    Determine whether t dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasTDim() const
    {
      return m_tDim.get() != NULL;
    }

    /**
    Pass though method indicating to the view that modifications have occured.
    */
    void SynchronisingGeometryPresenter::setModified()
    {
      m_view->raiseModified();
    }

    /**
    Determine if dimension presenter is mapped to x axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isXDimensionPresenter(DimensionPresenter const * const pDimensionPresenter) const
    {
      return pDimensionPresenter == m_xDim.get();
    }

    /**
    Determine if dimension presenter is mapped to y axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isYDimensionPresenter(DimensionPresenter const * const pDimensionPresenter) const
    {
      return pDimensionPresenter == m_yDim.get();
    }

    /**
    Determine if dimension presenter is mapped to z axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isZDimensionPresenter(DimensionPresenter const * const pDimensionPresenter) const
    {
      return pDimensionPresenter == m_zDim.get();
    }

    /**
    Determine if dimension presenter is mapped to t axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isTDimensionPresenter(DimensionPresenter const * const pDimensionPresenter) const
    {
      return pDimensionPresenter == m_tDim.get();
    }

    /**
    Lookup/Getter providing a label for dimension presenters. Used for display purposes.
    */
    std::string SynchronisingGeometryPresenter::getLabel(DimensionPresenter const * const pDimensionPresenter) const
    {
      std::string result = "";
      if(hasXDim())
      {
        if(isXDimensionPresenter(pDimensionPresenter))
        {
          result = "X Axis";
        }
      }
      if(hasYDim())
      {
        if(isYDimensionPresenter(pDimensionPresenter))
        {
          result = "Y Axis";
        }
      }
      if(hasXDim())
      {
        if(isZDimensionPresenter(pDimensionPresenter))
        {
          result = "Z Axis";
        }
      }
      if(hasTDim())
      {
        if(isTDimensionPresenter(pDimensionPresenter))
        {
          result = "T Axis";
        }
      }
      return result;
    }

  }
}
