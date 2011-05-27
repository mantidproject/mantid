#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
namespace Mantid
{
  namespace VATES
  {
    /**
    Constructor
    @parameter view : MVP view.
    @parameter geometryPresenter : parent presenter wrapping all dimensions.
    */
    DimensionPresenter::DimensionPresenter(DimensionView* view, GeometryPresenter * geometryPresenter) : m_view(view), m_geometryPresenter(geometryPresenter), m_lastIsIntegrated(false)
    {
    }

    /**
    Accept a model.
    @parameter model : The model to manage/contain.
    */
    void DimensionPresenter::acceptModel(Mantid::Geometry::IMDDimension_sptr model)
    {
      m_model = model;
      m_view->configure();
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
    Getter for read-only non-integrated dimensions.
    @return collection of non-integrated dimensions.
    */
    Mantid::Geometry::VecIMDDimension_sptr DimensionPresenter::getNonIntegratedDimensions() const
    {
      return m_geometryPresenter->getNonIntegratedDimensions();
    }

    /**
    Callable method from the view. Determines what to do after the view is updated in some manner.
    */
    void DimensionPresenter::updateModel()
    {
      validate();
      bool isIntegrated = m_view->getIsIntegrated();
      std::string id = m_view->getDimensionId();
      if(isIntegrated != m_lastIsIntegrated)
      {
        m_geometryPresenter->dimensionResized(this);
        m_lastIsIntegrated = isIntegrated;
      }
      else if(id != m_model->getDimensionId())
      {
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
        m_view->configure();
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

      return Mantid::Geometry::createDimension(m_model->toXMLString(), nbins, min, max);

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
    Gets the current dimension id from the view. 
    @return applied id
    */
    std::string DimensionPresenter::getDimensionId() const
    {
      return m_view->getDimensionId();
    }

    /**
    Getter for the label to use for this presenter.
    @return applied label.
    */
    std::string DimensionPresenter::getLabel() const
    {
      return m_geometryPresenter->getLabel(this);
    }

  }
}
