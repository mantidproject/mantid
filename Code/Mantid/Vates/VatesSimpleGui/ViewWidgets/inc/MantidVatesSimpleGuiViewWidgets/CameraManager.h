#ifndef CAMERAMANAGER_H_
#define CAMERAMANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesAPI/ViewFrustum.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{


/**
 *
  This class handles the camera of the view.

  @date 14/1/2015

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS CameraManager
{
public:
  CameraManager();

  ~CameraManager();
  
  Mantid::VATES::ViewFrustum getCurrentViewFrustum();

  void setCameraToPeak(double xpos, double ypos, double zpos, double peakRadius);
};

}
}
}

#endif 
