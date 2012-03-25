#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"

namespace Mantid
{
  namespace VATES
  {
    /**
    Constructor
    @param view : MVP view.
    @param geometryPresenter : parent presenter wrapping all dimensions.
    */
    DimensionPresenter::DimensionPresenter(DimensionView* view, GeometryPresenter * geometryPresenter) : m_geometryPresenter(geometryPresenter), m_view(view), m_lastIsIntegrated(false)
    {
    }

    /**
    Accept a model. In this schenario the model overrules any settings on the view.
    @param model : The model to manage/contain.
    */
    void DimensionPresenter::acceptModelStrongly(Mantid::Geometry::IMDDimension_sptr model)
    {
      m_model = model;
      m_view->configureStrongly();
      commonSetup();
    }

    /**
    Accept a model. In this schenario the model does not overrule settings on the view relating to nbins/max/mins.
    @param model : The model to manage/contain.
    */
    void DimensionPresenter::acceptModelWeakly(Mantid::Geometry::IMDDimension_sptr model)
    {
      m_model = model;
      m_view->configureWeakly();
      commonSetup();
    }

    void DimensionPresenter::commonSetup()
    {
      if(m_model->getIsIntegrated())
      {
        m_view->showAsIntegrated();
        m_lastIsIntegrated = true;
      }
      else
      {
        m_view->showAsNotIntegrated(m_geometryPresenter->getNonIntegratedDimensions());
        m_lastIsIntegrated = false;
      }
    }

    /**
    Accept the applied model. i.e. Model -> AppliedModel
    */
    void DimensionPresenter::acceptAppliedModel()
    {
      m_model = this->getAppliedModel();
    }

    /**
    Callable method from the view. Determines what to do after the view is updated in some manner.
    */
    void DimensionPresenter::updateModel()
    {
      validate();
      bool isIntegrated = m_view->getIsIntegrated();
      std::string mapping = m_view->getVisDimensionName();
      if(isIntegrated != m_lastIsIntegrated)
      {
        //Dimension must have been collapsed/integrated.
        m_geometryPresenter->dimensionResized(this);
        m_lastIsIntegrated = isIntegrated;
      }
      else if(mapping != this->m_mapping)
      {
        //Dimensions must have been swapped.
        m_geometryPresenter->dimensionRealigned(this);
      }
      if(isIntegrated)
      {
        m_view->showAsIntegrated();
      }
      else
      {
        m_view->showAsNotIntegrated(m_geometryPresenter->getNonIntegratedDimensions());
      }
      m_geometryPresenter->setModified();
    }

    /**
    Update the view only if it is displaying for non-integrated.
    */
    void DimensionPresenter::updateIfNotIntegrated()
    {
      if(!m_view->getIsIntegrated())
      {
        m_view->configureWeakly();
        m_view->showAsNotIntegrated(m_geometryPresenter->getNonIntegratedDimensions());
        m_lastIsIntegrated = false;
      }
    }

    /**
    Getter for the applied model. This is the base model + any changes taken from the view.
    @return applied model.
    */
    Mantid::Geometry::IMDDimension_sptr DimensionPresenter::getAppliedModel() const
    {
      validate();
      bool isIntegrated = m_view->getIsIntegrated();
      unsigned int nbins;
      if(!isIntegrated) //TODO. Needs cleaning up!
      {
        if(m_view->getNBins() > 1)
        {
          nbins = m_view->getNBins();
        }
        else
        {
          nbins = 10;
        }
      }
      else
      {
        nbins = 1;
      }

      double min = m_view->getMinimum();
      double max = m_view->getMaximum();
      try
      {
       return Mantid::Geometry::createDimension(m_model->toXMLString(), nbins, min, max);
      }
      catch(std::invalid_argument&)
      {
        m_view->configureStrongly();
        m_view->displayError("Check the ranges just entered. Must have min < max.");
        return m_model;
      }
    }

    /**
    Getter for the MVP model.
    @return read-only model to visualise.
    */
    Mantid::Geometry::IMDDimension_sptr DimensionPresenter::getModel() const
    {
      validate();
      return m_model;
    }

    /// Destructor
    DimensionPresenter::~DimensionPresenter()
    {
    }

    /**
    Validate the usage of the presenter.
    */
    void DimensionPresenter::validate() const
    {
      if(m_model.get() == NULL)
      {
         throw std::runtime_error("Trying to use DimensionPresenter without calling ::acceptModel first");
      }
    }

    /**
    Getter for the label to use for this presenter.
    @return applied label.
    */
    std::string DimensionPresenter::getLabel() const
    {
      return this->m_model->getDimensionId();
    }

    /**
    Getter for the name of the visualisation dimension.
    @return name of the visualisation dimension this presenter is using (if any)
    */
    std::string DimensionPresenter::getVisDimensionName() const
    {
      return m_view->getVisDimensionName();
    }

    /**
    Pass through method. Gets all mapping-presenter pairs.
    @return mapping pairs.
    */
    GeometryPresenter::MappingType DimensionPresenter::getMappings() const
    {
      return m_geometryPresenter->getMappings();
    }

    /**
    Setter for the mapping to use.
    @param mapping to use.
    */
    void DimensionPresenter::setMapping(std::string mapping)
    {
      this->m_mapping = mapping;
    }

    /**
    Getter for the mapping to use.
    @return mapping used.
    */
    std::string DimensionPresenter::getMapping() const
    {
      return m_mapping;
    }

    /**
    Setter for the bin display mode.
    @mode : Bin display mode.
    */
    void DimensionPresenter::setViewMode(BinDisplay mode)
    {
      this->m_view->setViewMode(mode);
    }

  }
}
