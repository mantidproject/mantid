#ifndef SLICEVIEWERWINDOW_H
#define SLICEVIEWERWINDOW_H

#include "../../MdiSubWindow.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtSliceViewer/LineViewer.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include <QtGui/QMainWindow>
#include <QtGui/QSplitter>


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

private:
  void setLineViewerValues(QPointF start2D, QPointF end2D, double width);

signals:
  void needToClose();
  void needToUpdate();

protected slots:
  void closeWindow();
  void updateWorkspace();
  void lineChanging(QPointF start, QPointF end, double width);
  void lineChanged(QPointF start, QPointF end, double width);

protected:
  void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

  /// The SliceViewer
  MantidQt::SliceViewer::SliceViewer * m_slicer;

  /// The LineViewer
  MantidQt::SliceViewer::LineViewer * m_liner;

  /// Horizontal splitter between slice viewer and LineViewer
  QSplitter * m_splitter;

  /// Workspace being looked at
  Mantid::API::IMDWorkspace_sptr m_ws;
  /// Name of the workspace being viewed
  std::string m_wsName;

};

#endif // SLICEVIEWERWINDOW_H
