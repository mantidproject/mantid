#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/SynchronisingGeometryPresenter.h"
#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/GeometryView.h"
#include <algorithm>


using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::VecIMDDimension_sptr;
typedef Mantid::VATES::GeometryPresenter::MappingType MappingType;

namespace Mantid
{
  namespace VATES
  {

    /// Comparitor to find integrated dimensions.
     struct FindIntegrated : public std::unary_function <IMDDimension_sptr, bool>
    {
      bool operator ()(const IMDDimension_sptr obj) const
      {
        return obj->getIsIntegrated();
      }
    };

    /// Comparitor to find DimensionPresenter shared pointers via a dimension id.
    struct FindId : public std::unary_function <DimPresenter_sptr, bool>
    {
      const std::string m_id;
      FindId(const std::string &id) : m_id(id){ }

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
      FindModelId(const std::string &id) : m_id(id){ }

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
      X_AXIS("X-AXIS"),
      Y_AXIS("Y-AXIS"),
      Z_AXIS("Z-AXIS"),
      T_AXIS("T-AXIS"),
      m_dimensions(source.getAllDimensions()), 
      m_source(source),
      m_view(NULL),
      m_binDisplayMode(Simple)
    {

    }

    /**
    Destructor
    */
    SynchronisingGeometryPresenter::~SynchronisingGeometryPresenter()
    {
    }

    void SynchronisingGeometryPresenter::swap(const GeometryPresenter::MappingType::key_type& keyA, const GeometryPresenter::MappingType::key_type& keyB)
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
    @param pDimensionPresenter : dimension presenter on which realignement has been requested.
    */
    void SynchronisingGeometryPresenter::dimensionRealigned(DimensionPresenter* pDimensionPresenter)
    {
      swap(pDimensionPresenter->getMapping(), pDimensionPresenter->getVisDimensionName());
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
    @param expiredMappedDimension : mapped dimension presenter which has been collapsed, and may currently occupy a x, y, z, t mapping.
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
    @param candidateMappedDimension : Dimension presenter to which a mapping is requested.
    */
    void SynchronisingGeometryPresenter::insertMappedPresenter(DimPresenter_sptr candidateMappedDimension)
    {
      /*
      Check to see whether there is already a mapping for this presenter. If there is, don't create another one!
      */
      bool bAlreadyMapped = false;
      for (MappingType::iterator it = m_mapping.begin(); it != m_mapping.end(); ++it)
      {
        if (it->second == candidateMappedDimension)
        {
          bAlreadyMapped = true;
          break;
        }
      }
      if(!bAlreadyMapped)
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
    }

    /**
    Handles the change of a managed dimension presenter to be expanded (from collapsed). 
    @param pDimensionPresenter : dimension which is now expanded.
    */
    void SynchronisingGeometryPresenter::dimensionExpanded(DimensionPresenter* pDimensionPresenter)
    {
        //Replace the old dimension with the new/modified one.
        std::replace_if(m_dimensions.begin(), m_dimensions.end(), FindModelId(pDimensionPresenter->getAppliedModel()->getDimensionId()), pDimensionPresenter->getAppliedModel());
        //Insert an axis-mapping for this expanded dimension.
        VecDimPresenter_sptr::iterator location = std::find_if(m_dimPresenters.begin(), m_dimPresenters.end(), FindId(pDimensionPresenter->getAppliedModel()->getDimensionId()));
        if (location != m_dimPresenters.end())
        {
          insertMappedPresenter((*location));
        }
        shuffleMappedPresenters();
      
    }

    /**
    Handles the change of a managed dimension presenter to be collapsed (from expanded). 
    @param pDimensionPresenter : dimension which is now collapsed.
    */
    void SynchronisingGeometryPresenter::dimensionCollapsed(DimensionPresenter* pDimensionPresenter)
    { 
      //Effectively end the transaction if it will result in zero non-integrated dimensions
      if(1 == getNonIntegratedDimensions().size())
      {
        throw std::invalid_argument("Cannot have all dimensions integrated!");
      }
      //Replace the old dimension with the new/modified one.
      std::replace_if(m_dimensions.begin(), m_dimensions.end(), FindModelId(pDimensionPresenter->getAppliedModel()->getDimensionId()), pDimensionPresenter->getAppliedModel());
      //DONOT ERRASE THE MAPPING. 
      shuffleMappedPresenters();

    }

    /**
    Handles dimension resize request. Can either be collapsed or expanded. This is worked out internally.
    @param pDimensionPresenter : dimension which is now collapsed/expanded.
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
    }

    /**
    Getter for non-integrated dimensions.
    @return collection of non-integrated dimensions.
    */
    Mantid::Geometry::VecIMDDimension_sptr SynchronisingGeometryPresenter::getNonIntegratedDimensions() const
    {
      VecIMDDimension_sptr matches;
      VecIMDDimension_sptr::iterator i = m_dimensions.begin();
      FindIntegrated findIntegrated;
      std::unary_negate<FindIntegrated> findNotIntegrated(findIntegrated);
      while(true) 
      {
        i = std::find_if(i, m_dimensions.end(), findNotIntegrated);
        if (i == m_dimensions.end())
          break;
        matches.push_back(*i);
        ++i;
      }
      return matches;
    }

    /**
    Getter for integrated dimensions.
    @return collection of non-integrated dimensions.
    */
    Mantid::Geometry::VecIMDDimension_sptr SynchronisingGeometryPresenter::getIntegratedDimensions() const
    {
      VecIMDDimension_sptr matches;
      VecIMDDimension_sptr::iterator i = m_dimensions.begin();
      FindIntegrated findIntegrated;
      while(true) 
      {
        i = std::find_if(i, m_dimensions.end(), findIntegrated);
        if (i == m_dimensions.end())
          break;
        matches.push_back(*i);
        ++i;
      }
      return matches;
    }

    /**
    Getter for the geometry xml.
    @return GeometryXML in string format.
    */
    std::string SynchronisingGeometryPresenter::getGeometryXML() const
    {
      //Get the selected alignment for the xdimension.
      using namespace Mantid::Geometry;
      MDGeometryBuilderXML<NoDimensionPolicy> xmlBuilder;

      VecIMDDimension_sptr vecIntegrated = getIntegratedDimensions();
      VecDimPresenter_sptr::const_iterator its = m_dimPresenters.begin();
      for(;its != m_dimPresenters.end(); ++its)
      {
        if((*its)->getAppliedModel()->getIsIntegrated())
        {
          xmlBuilder.addOrdinaryDimension((*its)->getAppliedModel());
        }
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

    @param view : the GeoemtryView to direct.
    */
    void SynchronisingGeometryPresenter::acceptView(GeometryView* view)
    {
      m_view = view;
      m_binDisplayMode = m_view->getBinDisplayMode();
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
      for(size_t i = 0; i < m_dimPresenters.size(); i++)
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
    Setter to indicate changes to the display mode.
    */
    void SynchronisingGeometryPresenter::setDimensionModeChanged()
    {
      //Get the actual requested display mode.
      BinDisplay temp = m_view->getBinDisplayMode();
      if(temp != m_binDisplayMode)
      {
        m_binDisplayMode = temp;
        VecDimPresenter_sptr::iterator it = m_dimPresenters.begin();
        while(it != m_dimPresenters.end())
        {
          //Delegate the work of applying the changes to each DimensionPresenter.
          (*it)->setViewMode(m_binDisplayMode);
          ++it;
        }
      }
    }

    /**
    Determine if dimension presenter is mapped to x axis.
    @param dimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isXDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(X_AXIS);
    }

    /**
    Determine if dimension presenter is mapped to y axis.
    @param dimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isYDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(Y_AXIS);
    }

    /**
    Determine if dimension presenter is mapped to z axis.
    @param dimensionPresenter : The dimension presenter to which the comparison should be made.
    @return true if dimesion presenter matches exising mapping.
    */
    bool SynchronisingGeometryPresenter::isZDimensionPresenter(DimPresenter_sptr dimensionPresenter) const
    {
      return dimensionPresenter == m_mapping.at(Z_AXIS);
    }

    /**
    Determine if dimension presenter is mapped to t axis.
    @param dimensionPresenter : The dimension presenter to which the comparison should be made.
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
