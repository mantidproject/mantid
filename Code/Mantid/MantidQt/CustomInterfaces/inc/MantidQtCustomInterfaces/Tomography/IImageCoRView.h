#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGECORVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGECORVIEW_H_

#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Widget to handle the selection of the center of rotation, region of
interest, region for normalization, etc. from an image or stack of
images. This is the abstract base class / interface for the view of
this widget (in the sense of the MVP pattern).  The name ImageCoR
refers to the Center-of-Rotation, which is the most basic parameter
that users can select via this widget. This class is Qt-free. Qt
specific functionality and dependencies are added in a class derived
from this.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class IImageCoRView {

public:
  IImageCoRView(){};
  virtual ~IImageCoRView(){};

  /**
   * Sets the user selection. This should guarantee that all widgets
   * are updated (including spin boxes, image, slider through the
   * image stack, etc.
   *
   * @param tools identifiers of the tools that can or could be run.
   * Order matters
   *
   */
  virtual void initParams(ImageStackPreParams &params) = 0;

  /**
   * Provides the current user selection.
   *
   * @return parameters as set/edited by the user.
   */
  virtual ImageStackPreParams userSelection() const = 0;

  /**
   *
   */
  virtual void showImgOrStack() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGECORVIEW_H_
