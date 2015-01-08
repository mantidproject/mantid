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
      generalMdColorMap = "ColdFire";

      // Background color
      defaultBackgroundColor = QColor(84,89,109);

      // Populate the optional color maps
      vsiColorMaps.append("Cool to Warm");
      vsiColorMaps.append("Blue to Red Rainbow");
      vsiColorMaps.append("Red to Blue Rainbow");
      vsiColorMaps.append("Grayscale");
      vsiColorMaps.append("X Ray");
      vsiColorMaps.append("Blue to Yellow");
    }

    void MdConstants::initializeViewConstants()
    {
      techniqueDependence = "Technique-Dependent";
      standardView = "Standard";
      multiSliceView = "Multi Slice";
      threeSliceView = "Three Slice";
      splatterPlotView = "Splatter Plot";
    }

    /**
     * Gets the general MD color map.
     *@returns The general MD color map.
     */
    QString MdConstants::getGeneralMdColorMap() const
    {
      return generalMdColorMap;
    }

    /**
     * Gets the label for the background color.
     *@returns The label for the background color.
     */
    QColor MdConstants::getDefaultBackgroundColor() const
    {
      return defaultBackgroundColor;
    }

    /**
     * Gets a list of VSI color maps.
     *@returns The list of VSI color maps.
     */
    QStringList MdConstants::getVsiColorMaps() const
    {
      return vsiColorMaps;
    }

    /**
     * Get the standard view.
     *@returns The standard view in the VSI.
     */
    QString MdConstants::getStandardView() const
    {
      return standardView;
    }

     /**
     * Get the multi slice view.
     *@returns The multi slice view in the VSI.
     */
    QString MdConstants::getMultiSliceView() const
    {
      return multiSliceView;
    }

     /**
     * Get the three slice view.
     *@returns The three slice view in the VSI.
     */
    QString MdConstants::getThreeSliceView() const
    {
      return threeSliceView;
    }

     /**
     * Get the splatter plot view.
     *@returns The splatter plot view in the VSI.
     */
    QString MdConstants::getSplatterPlotView() const
    {
      return splatterPlotView;
    }

     /**
     * Get the technique dependence.
     *@returns The technique dependence.
     */
    QString MdConstants::getTechniqueDependence() const
    {
      return techniqueDependence;
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
