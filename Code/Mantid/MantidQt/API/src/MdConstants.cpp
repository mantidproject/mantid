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
  }
}
