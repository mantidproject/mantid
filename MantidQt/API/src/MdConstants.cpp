#include "MantidQtAPI/MdConstants.h"
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QColor>

namespace MantidQt
{
  namespace API
  {
    // Specifiers for ParaView filters
    const QString MdConstants::MantidParaViewSplatterPlot = "MantidParaViewSplatterPlot";
    const QString MdConstants::MantidParaViewSpecialCoordinates = "SpecialCoordinates";
    const QString MdConstants::MDPeaksFilter = "MDPeaksFilter";
    const QString MdConstants::MantidParaViewPeaksFilter = "MantidParaViewPeaksFilter";
    const QString MdConstants::PeakDimensions = "Peak Dimensions";
    const QString MdConstants::PeaksWorkspace = "PeaksWorkspace";
    const QString MdConstants::Delimiter = "Delimiter";
    const QString MdConstants::WorkspaceName = "WorkspaceName";
    const QString MdConstants::ProbePoint = "ProbePoint";
    const QString MdConstants::Threshold = "Threshold";

    MdConstants::MdConstants() : m_colorScaleStandardMax(0.1), m_logScaleDefaultValue(0.1)
    {
      initializeSettingsConstants();
      initializeViewConstants();
    }

    MdConstants::~MdConstants(){}

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

      m_autoNormalization = "Auto Select";
      m_volumeNormalization = "Volume";
      m_numberEventNormalization = "Number Events";
      m_noNormalization = "None";
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
    
    double MdConstants::getColorScaleStandardMax()
    {
      return m_colorScaleStandardMax;
    }

    double MdConstants::getLogScaleDefaultValue()
    {
      return m_logScaleDefaultValue;
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

    /**
     * Get the auto normalization 
     * @returns the auto normalization
     */
    QString MdConstants::getAutoNormalization() const {
      return m_autoNormalization;
    }

    /** 
     * Get the volume normalization
     * @returns the volume normalization 
     */
    QString MdConstants::getVolumeNormalization() const {
      return m_volumeNormalization;
    }

    /**
     * Get the number of events normalization
     * @returns the number of events normalization
     */
    QString MdConstants::getNumberEventNormalization() const {
      return m_numberEventNormalization;
    }

    /**
     * Get no normalization
     * @returns the no normalization
     */
    QString MdConstants::getNoNormalization() const {
      return m_noNormalization;
    }

     /**
     * Get a list of all normalization options.
     *@returns A list of all normalization options
     */
    QStringList MdConstants::getAllNormalizations() const
    {
      QStringList normalizations;

      normalizations.append(getAutoNormalization());
      normalizations.append(getVolumeNormalization());
      normalizations.append(getNumberEventNormalization());
      normalizations.append(getNoNormalization());
      return normalizations;
    }

  }
}
