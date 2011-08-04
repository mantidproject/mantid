#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/SynchronisingGeometryPresenter.h"
#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/GeometryView.h"
#include <algorithm>


using Mantid::Geometry::IMDDimension_sptr;
typedef Mantid::VATES::GeometryPresenter::MappingType MappingType;

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
    struct FindModelId : public std::unary_function <IMDDimension_sptr, bool>
    {
      const std::string m_id;
      FindModelId(const std::string id) : m_id(id){ }

      bool operator ()(const IMDDimension_sptr obj) const
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
      m_source(source),
      X_AXIS("X-AXIS"),
      Y_AXIS("Y-AXIS"),
      Z_AXIS("Z-AXIS"),
      T_AXIS("T-AXIS")
    {

    }

    /**
    Destructor
    */
    SynchronisingGeometryPresenter::~SynchronisingGeometryPresenter()
    {
    }

    void SynchronisingGeometryPresenter::swap(const MappingType::key_type& keyA, const MappingType::key_type& keyB)
    {
      DimPresenter_sptr temp = m_mapping[keyA];
      
      //Swap items in mapping list.
      m_mapping[keyA] = m_mapping[keyB];
      m_mapping[keyB] = temp;

      //Set mapping name of presenter and then force view to update.
      if(NULL != m_mapping[keyA])
      {
        m_mapping[keyA]->setMapping(keyA);
        m_mapping[keyA]->acceptModelWeakly(m_mapping[keyA]->getModel());
      }
      //Set mapping name of presenter and then force view to update.
      if(NULL != m_mapping[keyB])
      {
        m_mapping[keyB]->setMapping(keyB);
        m_mapping[keyB]->acceptModelWeakly(m_mapping[keyB]->getModel());
      }
    }

    /**
    Handles dimension realignment. When a dimension presenter is handling a realignment, its is necessary for this to be synchronsied with other non-integrated dimensions.
    @parameter pDimensionPresenter : dimension presenter on which realignement has been requested.
    */
    void SynchronisingGeometryPresenter::dimensionRealigned(DimensionPresenter* pDimensionPresenter)
    {
      swap(pDimensionPresenter->getMapping(), pDimensionPresenter->getVisDimensionName());

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
        swap(X_AXIS, Y_AXIS);
        eraseMappedPresenter(m_mapping[Y_AXIS]);
      }
      if(hasZDim() && !hasYDim())
      {
        swap(Y_AXIS, Z_AXIS);
        eraseMappedPresenter(m_mapping[Z_AXIS]);
      }
      if(hasTDim() && !hasZDim())
      {
        swap(T_AXIS, Z_AXIS);
        eraseMappedPresenter(m_mapping[T_AXIS]);
      }
    }

    /**
    Ensure that for the collaped mapeed dimension, it's mapped placeholder is erased (marked as empty).
    @parameter expiredMappedDimension : mapped dimension presenter which has been collapsed, and may currently occupy a x, y, z, t mapping.
    */
    void SynchronisingGeometryPresenter::eraseMappedPresenter(DimPresenter_sptr expiredMappedDimension)
    {
      if(NULL != expiredMappedDimension)
      {
        m_mapping.erase(expiredMappedDimension->getMapping());
      }
    }

    /**
    With the priority mapping of x before y, y before z, and z before t. Ensure that a candidate mapped dimension presenter is set to occupy a vacent mapping.
    @parameter candidateMappedDimension : Dimension presenter to which a mapping is requested.
    */
    void SynchronisingGeometryPresenter::insertMappedPresenter(DimPresenter_sptr candidateMappedDimension)
    {
      if(!this->hasXDim())
      {
        m_mapping.insert(std::make_pair(X_AXIS, candidateMappedDimension));
        candidateMappedDimension->setMapping(X_AXIS);
      }
      else if(!this->hasYDim())
      {
        m_mapping.insert(std::make_pair(Y_AXIS, candidateMappedDimension));
        candidateMappedDimension->setMapping(Y_AXIS);
      }
      else if(!hasZDim())
      {
        m_mapping.insert(std::make_pair(Z_AXIS, candidateMappedDimension));
        candidateMappedDimension->setMapping(Z_AXIS);
      }
      else if(!hasTDim())
      {
        m_mapping.insert(std::make_pair(T_AXIS, candidateMappedDimension));
        candidateMappedDimension->setMapping(T_AXIS);
      }
    }

    /**
    Handles the change of a managed dimension presenter to be expanded (from collapsed). 
    @parameter pDimensionPresenter : dimension which is now expanded.
    */
    void SynchronisingGeometryPresenter::dimensionExpanded(DimensionPresenter* pDimensionPresenter)
    {
      Mantid::Geometry::VecIMDDimension_sptr::iterator it = std::find_if(m_notIntegrated.begin(), m_notIntegrated.end(), FindModelId(pDimensionPresenter->getModel()->getDimensionId()));
      if(it == m_notIntegrated.end())
      {
        //TODO: Need to prevent m_nonIntegrated exceeding the number of mapped dimensions! Simple check here.
        m_notIntegrated.push_back(pDimensionPresenter->getAppliedModel()); 
        m_integrated.erase(std::remove_if(m_integrated.begin(), m_integrated.end(), FindModelId(pDimensionPresenter->getModel()->getDimensionId())), m_integrated.end());
        VecDimPresenter_sptr::iterator location = std::find_if(m_dimPresenters.begin(), m_dimPresenters.end(), FindId(pDimensionPresenter->getModel()->getDimensionId()));
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
      //Effectively end the transactino if it will result in zero non-integrated dimensions
      if(1 == m_notIntegrated.size())
      {
        throw std::invalid_argument("Cannot have all dimensions integrated!");
      }

      Mantid::Geometry::VecIMDDimension_sptr::iterator it = std::find_if(m_integrated.begin(), m_integrated.end(), FindModelId(pDimensionPresenter->getModel()->getDimensionId()));
      if(it == m_integrated.end())
      {
        m_integrated.push_back(pDimensionPresenter->getAppliedModel());
        m_notIntegrated.erase(std::remove_if(m_notIntegrated.begin(), m_notIntegrated.end(), FindModelId(pDimensionPresenter->getModel()->getDimensionId())), m_notIntegrated.end());

        VecDimPresenter_sptr::iterator location = std::find_if(m_dimPresenters.begin(), m_dimPresenters.end(), FindId(pDimensionPresenter->getModel()->getDimensionId()));

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
        xmlBuilder.addXDimension(m_mapping.at(X_AXIS)->getAppliedModel());
      }
      if(hasYDim())
      {
        xmlBuilder.addYDimension(m_mapping.at(Y_AXIS)->getAppliedModel());
      }
      if(hasZDim())
      {
        xmlBuilder.addZDimension(m_mapping.at(Z_AXIS)->getAppliedModel());
      }
      if(hasTDim())
      {
        xmlBuilder.addTDimension(m_mapping.at(T_AXIS)->getAppliedModel());
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
          dimPresenter->setMapping(X_AXIS);
          m_mapping.insert(std::make_pair(X_AXIS, dimPresenter));
        }
        else if(m_source.isYDimension(model))
        {
          dimPresenter->setMapping(Y_AXIS);
          m_mapping.insert(std::make_pair(Y_AXIS, dimPresenter));
        }
        else if(m_source.isZDimension(model))
        {
          dimPresenter->setMapping(Z_AXIS);
          m_mapping.insert(std::make_pair(Z_AXIS, dimPresenter));
        }
        else if(m_source.isTDimension(model))
        {
          dimPresenter->setMapping(T_AXIS);
          m_mapping.insert(std::make_pair(T_AXIS, dimPresenter));
        }

        // Dimension View must have reference to Dimension Presenter.
        dimView->accept(dimPresenter.get());
        // Geometry View owns the Dimension View.
        m_view->addDimensionView(dimView);
        // Presenters are mainatined internally.
        m_dimPresenters.push_back(dimPresenter);
      }
      for(int i = 0; i < m_dimPresenters.size(); i++)
        {
          //Now that all presenters have views, models can be provided to complete the M-V-P chain.
          m_dimPresenters[i]->acceptModelStrongly(m_source.getAllDimensions()[i]);
        }
    }

    /**
    Determine whether x dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasXDim() const
    {
      return m_mapping.find(X_AXIS) != m_mapping.end() && NULL != m_mapping.find(X_AXIS)->second;
    }

    /**
    Determine whether y dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasYDim() const
    {
      return m_mapping.find(Y_AXIS) != m_mapping.end() && NULL != m_mapping.find(Y_AXIS)->second;
    }

    /**
    Determine whether z dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasZDim() const
    {
      return m_mapping.find(Z_AXIS) != m_mapping.end() && NULL != m_mapping.find(Z_AXIS)->second;
    }

    /**
    Determine whether t dimension mapping is available.
    @return true if target mapping is available.
    */
    bool SynchronisingGeometryPresenter::hasTDim() const
    {
      return m_mapping.find(T_AXIS) != m_mapping.end()  && NULL != m_mapping.find(T_AXIS)->second;
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
    bool SynchronisingGeometryPresenter::isXDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(X_AXIS);
    }

    /**
    Determine if dimension presenter is mapped to y axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isYDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(Y_AXIS);
    }

    /**
    Determine if dimension presenter is mapped to z axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isZDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(Z_AXIS);
    }

    /**
    Determine if dimension presenter is mapped to t axis.
    @parameter pDimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isTDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(T_AXIS);
    }

    /**
    Get mappings of vis diension names to dimension presenters.
    @return mapping.
    */
    MappingType SynchronisingGeometryPresenter::getMappings() const
    {
      return m_mapping;
    }

  }
}
