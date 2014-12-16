#include "MantidVatesSimpleGuiViewWidgets/ColorMapManager.h"
#include "MantidKernel/ConfigService.h"
#include <QSettings>
#include <map>
#include <string>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      ColorMapManager::ColorMapManager() : indexCounter(0)
      {
        // Check if this is the first time a color map will be loaded
        QSettings settings;

        settings.beginGroup("Mantid/Vsi");

        settings.setValue("intitialcolormap", "Cool to Warm");

        settings.endGroup();
      }
      
      ColorMapManager::~ColorMapManager()
      {
      }
      
      int ColorMapManager::getDefaultColorMapIndex()
      {
        // Read from QSettings
        QSettings settings;

        settings.beginGroup("Mantid/Vsi");

        std::string defaultColorMap;

        if (settings.value("firststartup", true).asBool())
        {
          defaultColorMap = settings.value("intitialcolormap", QString("")).toString().toStdString();

          settings.setValue("firststartup", false);
        }
        else
        {
          defaultColorMap = settings.value("colormap", QString("")).toString().toStdString();
        }

        settings.endGroup();

        // Set the default colormap
        int defaultColorMapIndex = 0;

        if (!defaultColorMap.empty())
        {
          defaultColorMapIndex = this->getColorMapIndex(defaultColorMap);
        }

        return defaultColorMapIndex;
      }

      void ColorMapManager::readInColorMap(std::string name)
      {
        // Add the name to the colormap map and increment the index counter
        if (!name.empty())
        {
          this->nameToIndex.insert(std::pair<std::string, int>(name, this->indexCounter));
          this->indexToName.insert(std::pair<int,std::string>(this->indexCounter, name));
          this->indexCounter = this->indexCounter + 1;
        }
      }
      
      int ColorMapManager::getColorMapIndex(std::string colorMap)
      {
        if (this->nameToIndex.count(colorMap) == 1 )
        {
          return nameToIndex[colorMap];
        }
        else
        {
          return 0;
        }
      }

      bool ColorMapManager::isRecordedColorMap(std::string colorMap)
      {
        if (this->nameToIndex.count(colorMap) > 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }

      void ColorMapManager::setNewActiveColorMap(int index)
      {
        // Persist the new value of the color map in the QSettings object.
        if (indexToName.count(index) > 0)
        {
          // Persist default color map
          QSettings settings;
          settings.beginGroup("Mantid/Vsi");
          settings.setValue("colormap", QString::fromStdString(indexToName[index]));
          settings.endGroup();
        }
      }
    }
  }
}