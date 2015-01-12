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
      ColorMapManager::ColorMapManager() : m_indexCounter(0)
      {
      }

      ColorMapManager::~ColorMapManager()
      {
      }

      int ColorMapManager::getDefaultColorMapIndex(bool viewSwitched)
      {
        QString defaultColorMap;

        // If the view has switched use the last color map index
        if (viewSwitched)
        {
          defaultColorMap = m_mdSettings.getLastSessionColorMap();
        }
        else 
        {
          // Check if the user wants a general MD color map
          if (m_mdSettings.getUsageGeneralMdColorMap())
          {
            // The name is sufficient for the VSI to find the color map
            defaultColorMap = m_mdSettings.getGeneralMdColorMapName(); 
          }
          else 
          {
            // Check if the user wants to use the last session
            if (m_mdSettings.getUsageLastSession())
            {
              defaultColorMap = m_mdSettings.getLastSessionColorMap();
            }
            else
            {
              defaultColorMap = m_mdSettings.getUserSettingColorMap();
            }
          }
        }

        // Set the default colormap
        int defaultColorMapIndex = 0;

        if (!defaultColorMap.isEmpty())
        {
          m_mdSettings.setLastSessionColorMap(defaultColorMap);
          defaultColorMapIndex = this->getColorMapIndex(defaultColorMap.toStdString());
        }

        return defaultColorMapIndex;
      }

      void ColorMapManager::readInColorMap(std::string name)
      {
        // Add the name to the colormap map and increment the index counter
        if (!name.empty())
        {
          m_nameToIndex.insert(std::pair<std::string, int>(name, m_indexCounter));
          m_indexToName.insert(std::pair<int,std::string>(m_indexCounter, name));
          m_indexCounter = m_indexCounter + 1;
        }
      }
      
      int ColorMapManager::getColorMapIndex(std::string colorMap)
      {
        if (m_nameToIndex.count(colorMap) == 1 )
        {
          return m_nameToIndex[colorMap];
        }
        else
        {
          return 0;
        }
      }

      bool ColorMapManager::isRecordedColorMap(std::string colorMap)
      {
        if (m_nameToIndex.count(colorMap) > 0)
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
        if (m_indexToName.count(index) > 0)
        {
          m_mdSettings.setLastSessionColorMap(QString::fromStdString(m_indexToName[index]));
        }
      }
    }
  }
}