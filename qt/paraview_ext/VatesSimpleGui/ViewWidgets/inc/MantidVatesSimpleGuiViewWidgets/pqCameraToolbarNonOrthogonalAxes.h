/*=========================================================================

   Program: ParaView
   Module:    pqCameraToolbar.h

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
 * Modified Camera toolbar to adjust view along nonorthogonal axes

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

#ifndef pqCameraToolbarNonOrthogonalAxes_h
#define pqCameraToolbarNonOrthogonalAxes_h

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "pqApplicationComponentsModule.h"
#include <QToolBar>

/**
* pqCameraToolbarNonOrthogonalAxes is the toolbar that has icons for resetting
* camera
* orientations as well as zoom-to-data and zoom-to-box.
*/
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS
    pqCameraToolbarNonOrthogonalAxes : public QToolBar {
  Q_OBJECT
  using Superclass = QToolBar;

public:
  pqCameraToolbarNonOrthogonalAxes(const QString &title,
                                   QWidget *parentObject = nullptr)
      : Superclass(title, parentObject) {
    this->constructor();
  }
  pqCameraToolbarNonOrthogonalAxes(QWidget *parentObject = nullptr)
      : Superclass(parentObject) {
    this->constructor();
  }
  pqCameraToolbarNonOrthogonalAxes(const pqCameraToolbarNonOrthogonalAxes &) =
      delete;
  pqCameraToolbarNonOrthogonalAxes &
  operator=(const pqCameraToolbarNonOrthogonalAxes &) = delete;
private slots:
  void updateEnabledState();

private:
  void constructor();
  QAction *ZoomToDataAction;
};

#endif
