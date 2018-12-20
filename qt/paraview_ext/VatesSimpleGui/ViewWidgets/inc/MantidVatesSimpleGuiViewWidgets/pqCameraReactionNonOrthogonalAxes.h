/*=========================================================================

   Program: ParaView
   Module:    pqCameraReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

/**
 * Modified Camera reaction to adjust view along nonorthogonal axes

 @date 19/04/2017

 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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

#ifndef pqCameraReactionNonOrthogonalAxes_h
#define pqCameraReactionNonOrthogonalAxes_h

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "pqReaction.h"

/**
 * @ingroup Reactions
 * pqCameraReaction has the logic to handle common operations associated with
 * the camera such as reset view along X axis etc.
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS
    pqCameraReactionNonOrthogonalAxes : public pqReaction {
  Q_OBJECT
  using Superclass = pqReaction;

public:
  enum Mode {
    RESET_CAMERA,
    RESET_POSITIVE_U,
    RESET_POSITIVE_V,
    RESET_POSITIVE_W,
    RESET_NEGATIVE_U,
    RESET_NEGATIVE_V,
    RESET_NEGATIVE_W,
    ZOOM_TO_DATA,
    ROTATE_CAMERA_CW,
    ROTATE_CAMERA_CCW
  };

  pqCameraReactionNonOrthogonalAxes(QAction *parent, Mode mode);
  pqCameraReactionNonOrthogonalAxes(const pqCameraReactionNonOrthogonalAxes &) =
      delete;
  pqCameraReactionNonOrthogonalAxes &
  operator=(const pqCameraReactionNonOrthogonalAxes &) = delete;
  static void resetCamera();
  static void resetPositiveU();
  static void resetPositiveV();
  static void resetPositiveW();
  static void resetNegativeU();
  static void resetNegativeV();
  static void resetNegativeW();
  static void resetDirection(double sign, std::array<int, 2> axes);
  static void zoomToData();
  static void rotateCamera(double angle);

public slots:
  /**
   * Updates the enabled state. Applications need not explicitly call
   * this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Mode ReactionMode;
};

#endif
