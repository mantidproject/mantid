#ifndef COLORMAP_MANAGER_H_
#define COLORMAP_MANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidQtAPI/MdSettings.h"
#include <string>
#include <map>
#include "boost/scoped_ptr.hpp"


namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      /**
       *
        This class handles the colormaps which are loaded into the 

        @date 10/12/2014

        Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at: <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
      */

      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorMapManager
      {
        public:
          ColorMapManager();
          
          virtual ~ColorMapManager();
          
          /**
           * Get default color map
           * @param viewSwitched If the view has switched or not.
           * @returns index The index of the default color map in the list of color maps.
           */
          int getDefaultColorMapIndex(bool viewSwitched);

          /**
           * Read in and store the available color maps
           * @param xml The path to the colormap.
           */
          void readInColorMap(std::string xml);
      
          /**
           * Get index for colormap
           * @param colorMap The name of the color map.
           * @returns The index of the colormap in the Paraview store.
           */
          int getColorMapIndex(std::string colorMap);
        
          /**
           * Check if a color map already has been recorded.
           * @param colorMap The name of the color map.
           * @returns True if the name already exists
           */
          bool isRecordedColorMap(std::string colorMap);

          /**
           * Record the new active color map when the user selected a new one.
           * @param index The index of the color map in the color map list.
           */
          void setNewActiveColorMap(int index);

        private:
          int indexCounter;
          std::map<std::string, int> nameToIndex;
          std::map<int, std::string> indexToName;

          boost::scoped_ptr<MantidQt::API::MdSettings> mdSettings;
      };
    }
  }
}
#endif 