#ifndef VSGMAINWINDOW_H_
#define VSGMAINWINDOW_H_

#include <QMainWindow>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
class MdViewerWidget;
}
}
}

class QAction;
class QMenu;
/**
 *
  This class represents the main level program.

  @author Michael Reuter
  @date 24/05/2011

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
class VsgMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the main window
   */
  VsgMainWindow(QWidget *parent = 0);
  /// Default destructor.
  virtual ~VsgMainWindow();

private:
  /// Create the actions for the main program.
  void createActions();
  /// Create the menus for the main program.
  void createMenus();

  QAction *openAction; ///< Action for opening files
  QAction *exitAction; ///< Action for exiting the program
  QMenu *fileMenu; ///< File actions menu
  Mantid::Vates::SimpleGui::MdViewerWidget *mdViewer; ///< The VATES viz widget
};

#endif // VSGMAINWINDOW_H_
