#include "MantidVatesSimpleGuiViewWidgets/ColorMapManager.h"
#include "MantidKernel/Logger.h"
#include <map>
#include <string>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      namespace
      {
        /// static logger
        Mantid::Kernel::Logger g_log("ColorMapManager");
      }

      ColorMapManager::ColorMapManager() :indexCounter(0)
      {
      }
      
      ColorMapManager::~ColorMapManager()
      {
      }
      
      void ColorMapManager::readInColorMap(std::string name)
      {
        // Add the name to the colormap map and increment the index counter
        if (!name.empty())
        {
          this->colorMapInfo.insert(std::pair<std::string, int>(name, this->indexCounter));
        
          this->indexCounter = this->indexCounter + 1;
        }
      }
      
      int ColorMapManager::getColorMapIndex(std::string colorMap)
      {
        if (this->colorMapInfo.count(colorMap) == 1 )
        {
          return colorMapInfo[colorMap];
        }
        else
        {
          // requested color map was not found
          g_log.warning() <<"Warning: Requested color map could not be found. Setting default color map. \n";
          return 0;
        }
      }

      bool ColorMapManager::isRecordedColorMap(std::string colorMap)
      {
        if (this->colorMapInfo.count(colorMap) > 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    }
  }
}