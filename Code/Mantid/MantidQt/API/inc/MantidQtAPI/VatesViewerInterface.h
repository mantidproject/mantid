#ifndef MANTIDQT_API_VATESVIEWERINTERFACE_H_
#define MANTIDQT_API_VATESVIEWERINTERFACE_H_

#include "DllOption.h"

#include <QWidget>

#include <string>

class QString;

namespace MantidQt
{
namespace API
{

/**
 *
  This class is an interface for the central widget for handling VATES visualization
  operations. Its main use is for the plugin mode operation of the viewer.

  @author Michael Reuter
  @date 08/09/2011

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
class EXPORT_OPT_MANTIDQT_API VatesViewerInterface : public QWidget
{
  Q_OBJECT
public:
  /// Default constructor for plugin mode.
  VatesViewerInterface();
  /**
   * Constructor for standalone mode.
   * @param parent the parent for the widget
   */
  VatesViewerInterface(QWidget *parent);
  /// Default destructor.
  virtual ~VatesViewerInterface();
  /**
   * Function to create the source from the given workspace.
   * @param workspaceName the name of the workspace to visualize
   * @param workspaceType the type of workspace being visualized
   * @param instrumentName The Name of the instrument.
   */
  virtual void renderWorkspace(QString workspaceName, int workspaceType, std::string instrumentName);

  /**
   * Special function of correct widget invocation for plugin mode.
   */
  virtual void setupPluginMode();

  /// Enum to track the workspace type
  enum WorkspaceType { MDEW, PEAKS, MDHW };

signals:
  /// Interface is asking to be closed
  void requestClose();

public slots:
  /// Perform any clean up on main window shutdown
  virtual void shutdown();
};

}
}

#endif // MANTIDQT_API_VATESVIEWERINTERFACE_H_
