// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_VATESVIEWERINTERFACE_H_
#define MANTIDQT_API_VATESVIEWERINTERFACE_H_

#include "DllOption.h"
#include "IProjectSerialisable.h"

#include <QWidget>
#include <string>

class QString;

namespace MantidQt {
namespace API {

/**
 *
  This class is an interface for the central widget for handling VATES
 visualization
  operations. Its main use is for the plugin mode operation of the viewer.

  @author Michael Reuter
  @date 08/09/2011
 */
class EXPORT_OPT_MANTIDQT_COMMON VatesViewerInterface
    : public QWidget,
      public IProjectSerialisable {
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
  ~VatesViewerInterface() override;

  /**
   * Function to create the source from the given workspace.
   * @param workspaceName the name of the workspace to visualize
   * @param workspaceType the type of workspace being visualized
   * @param instrumentName The Name of the instrument.
   */
  virtual void renderWorkspace(QString workspaceName, int workspaceType,
                               std::string instrumentName);
  /**
   * Special function of correct widget invocation for plugin mode.
   */
  virtual void setupPluginMode(int WsType, const std::string &instrumentName);

  /// Static method to create a handle to new window instance
  static IProjectSerialisable *loadFromProject(const std::string &lines,
                                               ApplicationWindow *app,
                                               const int fileVersion);
  /// Load the VATES gui from a Mantid project string
  virtual void loadFromProject(const std::string &lines) = 0;

  /// Enum to track the workspace type
  enum WorkspaceType { MDEW, PEAKS, MDHW };

signals:
  /// Interface is asking to be closed
  void requestClose();

public slots:
  /// Perform any clean up on main window shutdown
  virtual void shutdown();
};
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_VATESVIEWERINTERFACE_H_
