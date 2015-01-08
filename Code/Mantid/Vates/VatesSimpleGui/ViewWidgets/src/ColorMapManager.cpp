#include "MantidVatesSimpleGuiViewWidgets/ColorMapManager.h"
#include "MantidQtAPI/MdSettings.h"
#include "MantidKernel/ConfigService.h"
#include <map>
#include <string>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      ColorMapManager::ColorMapManager() : indexCounter(0), mdSettings(new MantidQt::API::MdSettings())
      {
      }

      ColorMapManager::~ColorMapManager()
      {
      }

      int ColorMapManager::getDefaultColorMapIndex(bool viewSwitched)
      {
        std::string defaultColorMap;

        // If the view has switched use the last color map index
        if (viewSwitched)
        {
          defaultColorMap = mdSettings->getLastSessionColorMap();
        }
        else 
        {
          // Check if the user wants a general MD color map
          if (mdSettings->getUsageGeneralMdColorMap())
          {
            // The name is sufficient for the VSI to find the color map
            defaultColorMap = mdSettings->getGeneralMdColorMapName(); 
          }
          else 
          {
            // Check if the user wants to use the last session
            if (mdSettings->getUsageLastSession())
            {
              defaultColorMap = mdSettings->getLastSessionColorMap();
            }
            else
            {
              defaultColorMap = mdSettings->getUserSettingColorMap();
            }
          }
        }

        // Set the default colormap
        int defaultColorMapIndex = 0;

        if (!defaultColorMap.empty())
        {
          mdSettings->setLastSessionColorMap(defaultColorMap);
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
          mdSettings->setLastSessionColorMap(indexToName[index]);
        }
      }
    }
  }
}