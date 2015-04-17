#ifndef BACKGROUNDRGB_PROVIDER_H_
#define BACKGROUNDRGB_PROVIDER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidQtAPI/MdSettings.h"
#include <vector>
#include <map>
#include <string>

#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqRenderView.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

class vtkObject;

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
          
          ~BackgroundRgbProvider();

          /**
           * Set the Rgb values for the color of the view's background.
           * @param viewSwitched Is this the initial loading or were the views switched?
           * @param view The view which has its background color set.
           */
          void setBackgroundColor(pqRenderView* view, bool viewSwitched);

          /**
           * Listen to a change in the background color
           *@param view The view which we want to listen to.
           */
          void observe(pqRenderView* view);

          /**
           * Update the last session background color.
           */
          void update();

       private:
          /**
          * Get the Rgb values for the color of the view's background from the user setting.
          * @param viewSwitched Is this the initial loading or were the views switched?
          * @returns A vector with the RGB values
          */
          std::vector<double> getRgbFromSetting(bool viewSwitched);

          /**
           * Get the Rgb values for the color of the view's background
           * @param viewSwitched Is this the initial loading or were the views switched?
           * @returns A vector with the RGB values
           */
          std::vector<double> getRgb(bool viewSwitched);

          /**
           * Callback function for background color changing events
           *@param caller Calling object.
           *@param vtkNotUsed Not used.
           */
          static void backgroundColorChangeCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId), void* vtkNotUsed(clientData), void* vtkNotUsed(callData));

          static QColor currentBackgroundColor;

          MantidQt::API::MdSettings m_mdSettings;
      };
    }
  }
}
#endif 
