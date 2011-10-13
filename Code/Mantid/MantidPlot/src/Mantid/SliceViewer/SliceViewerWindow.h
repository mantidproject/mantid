#ifndef SLICEVIEWERWINDOW_H
#define SLICEVIEWERWINDOW_H

#include <QtGui/QMainWindow>
#include "../../MdiSubWindow.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/IMDWorkspace.h"
#include "SliceViewer.h"


/** A MDI sub window that contains only a
 * SliceViewer widget for a particular workpsace
 *
 * @author Janik Zikovsky
 * @date October 13, 2011
 */
class SliceViewerWindow : public MdiSubWindow, public MantidQt::API::WorkspaceObserver
{
    Q_OBJECT

public:
    SliceViewerWindow(const QString& wsName, ApplicationWindow *app , const QString& label = QString() , Qt::WFlags f=0);
    ~SliceViewerWindow();

signals:
  void needToClose();
  void needToUpdate();

protected slots:
  void closeWindow();
  void updateWorkspace();

protected:
  void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

  /// The SliceViewer
  SliceViewer * m_slicer;

  /// Workspace being looked at
  Mantid::API::IMDWorkspace_sptr m_ws;
  /// Name of the workspace being viewed
  std::string m_wsName;

};

#endif // SLICEVIEWERWINDOW_H
