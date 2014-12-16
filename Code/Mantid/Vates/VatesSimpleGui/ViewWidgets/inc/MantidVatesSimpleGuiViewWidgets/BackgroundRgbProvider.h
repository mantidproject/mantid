#ifndef BACKGROUNDRGB_PROVIDER_H_
#define BACKGROUNDRGB_PROVIDER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include <vector>
#include <map>
#include <string>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      /**
       *
        This class gets the default color values for the background of the view.

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
      
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS BackgroundRgbProvider
      {
        public:
          BackgroundRgbProvider();
          
          virtual ~BackgroundRgbProvider();
          
          /**
           * Get the Rgb values for the color of the view's background
           * @returns A vector with the RGB values
           */
          std::vector<double> getRgb();
        
       private:
          /**
          * Get the Rgb values for the color of the view's background from the user setting
          * @returns A vector with the RGB values
          */
          std::vector<double> getRbgFromPropertiesFile();

          /**
           * Extract the rgb vector from the numeric user setting
           * @param background A vector with three color settings
           * @returns A vector with the RGB values or a default RGB vector
           */
          std::vector<double> getFromNumericSetting(std::vector<std::string> background);

          /**
           * Extract the rgb vector from the name setting
           * @param background A string with a color setting
           * @returns A vector with the RGB values or a default RGB vector
           */
          std::vector<double> getFromNameSetting(std::string background);

          /**
           * Extract all comma separated elements from the background setting string.
           * @param background A string with a color setting.
           * @returns A vector with the color entries.
           */
          std::vector<std::string> getCommaSeparatedEntries(std::string background);

          /**
           * Check if the numeric entry is acceptable, i.e. if it is between 0 and 255
           * @param entry The color value.
           * @returns True if the value is in the acceptable range.
           */
          bool BackgroundRgbProvider::isValidNumericEntry(double entry);

          /**
           * Check if the entry is a numeric entry at all
           * @param entry entry The color value.
           * @returns True if the value is a numeric value, otherwise false.
           */
          bool isNumeric(std::string entry);

          /// The separator string
          std::string separator;

          /// The default background color
          std::vector<double> defaultBackground;

          /// Background map which associates a name with an RGB vector
          std::map<std::string, std::vector<double>> backgroundMap;

      };
    }
  }
}
#endif 