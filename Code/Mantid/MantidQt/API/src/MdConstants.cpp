#include "MantidQtAPI/MdConstants.h"
#include <QSettings>
#include <QString>
#include <QStringList>
#include <Qcolor>

namespace MantidQt
{
  namespace API
  {
    MdConstants::MdConstants()
    {
      initializeSettingsConstants();
      initializeViewConstants();
    };

    MdConstants::~MdConstants(){};

    void MdConstants::initializeSettingsConstants()
    {
      // General MD Color Map
      m_generalMdColorMap = "ColdFire";

      // Background color
      m_defaultBackgroundColor = QColor(84,89,109);

      // Populate the optional color maps
      m_vsiColorMaps.append("Cool to Warm");
      m_vsiColorMaps.append("Blue to Red Rainbow");
      m_vsiColorMaps.append("Red to Blue Rainbow");
      m_vsiColorMaps.append("Grayscale");
      m_vsiColorMaps.append("X Ray");
      m_vsiColorMaps.append("Blue to Yellow");
    }

    void MdConstants::initializeViewConstants()
    {
      m_techniqueDependence = "Technique-Dependent";
      m_standardView = "Standard";
      m_multiSliceView = "Multi Slice";
      m_threeSliceView = "Three Slice";
      m_splatterPlotView = "Splatter Plot";
    }

    /**
     * Gets the general MD color map.
     *@returns The general MD color map.
     */
    QString MdConstants::getGeneralMdColorMap() const
    {
      return m_generalMdColorMap;
    }

    /**
     * Gets the label for the background color.
     *@returns The label for the background color.
     */
    QColor MdConstants::getDefaultBackgroundColor() const
    {
      return m_defaultBackgroundColor;
    }

    /**
     * Gets a list of VSI color maps.
     *@returns The list of VSI color maps.
     */
    QStringList MdConstants::getVsiColorMaps() const
    {
      return m_vsiColorMaps;
    }

    /**
     * Get the standard view.
     *@returns The standard view in the VSI.
     */
    QString MdConstants::getStandardView() const
    {
      return m_standardView;
    }

     /**
     * Get the multi slice view.
     *@returns The multi slice view in the VSI.
     */
    QString MdConstants::getMultiSliceView() const
    {
      return m_multiSliceView;
    }

     /**
     * Get the three slice view.
     *@returns The three slice view in the VSI.
     */
    QString MdConstants::getThreeSliceView() const
    {
      return m_threeSliceView;
    }

     /**
     * Get the splatter plot view.
     *@returns The splatter plot view in the VSI.
     */
    QString MdConstants::getSplatterPlotView() const
    {
      return m_splatterPlotView;
    }

     /**
     * Get the technique dependence.
     *@returns The technique dependence.
     */
    QString MdConstants::getTechniqueDependence() const
    {
      return m_techniqueDependence;
    }

     /**
     * Get a list of all initial views.
     *@returns A list of all viewss, including a technique-dependent view
     */
    QStringList MdConstants::getAllInitialViews() const
    {
      QStringList views;

      views.append(getTechniqueDependence());
      views.append(getStandardView());
      views.append(getMultiSliceView());
      views.append(getThreeSliceView());
      views.append(getSplatterPlotView());

      return views;
    }
  }
}
