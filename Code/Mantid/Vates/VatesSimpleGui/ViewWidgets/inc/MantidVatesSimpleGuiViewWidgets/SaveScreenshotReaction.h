#ifndef __SAVESCREENSHOTREACTION_H
#define __SAVESCREENSHOTREACTION_H

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <pqReaction.h>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS SaveScreenshotReaction : public pqReaction
{
  /**
   *
   This class is a copy of the ParaView pqSaveScreenshotReaction class and is
   modified in order to be used with our "custom" views.

   @date 14/06/2013

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

  Q_OBJECT
  typedef pqReaction Superclass;
public:
  /// Constructor. Parent cannot be NULL.
  SaveScreenshotReaction(QAction* parent);

  /// Saves the screenshot.
  /// Note that this method is static. Applications can simply use this without
  /// having to create a reaction instance.
  static void saveScreenshot();
  static void saveScreenshot(const QString& filename,
                             const QSize& size,
                             int quality);

public slots:
  /// Updates the enabled state. Applications need not explicitly call this.
  void updateEnableState();

protected:
  /// Called when the action is triggered.
  virtual void onTriggered()
  { SaveScreenshotReaction::saveScreenshot(); }

private:
  Q_DISABLE_COPY(SaveScreenshotReaction)
};

}
}
}

#endif // __SAVESCREENSHOTREACTION_H


