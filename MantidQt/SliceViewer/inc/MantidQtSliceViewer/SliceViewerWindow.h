#ifndef SLICEVIEWERWINDOW_H
#define SLICEVIEWERWINDOW_H

#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtAPI/IProjectSerialisable.h"
#include "MantidQtSliceViewer/LineViewer.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "MantidQtSliceViewer/PeaksViewer.h"
#include <QtGui/QMainWindow>
#include <QtGui/QSplitter>
#include <QtGui/qdialog.h>
#include <qmainwindow.h>
#include <QShowEvent>
#include "DllOption.h"
#include "MantidKernel/VMD.h"

namespace MantidQt {
namespace SliceViewer {

/** A window that contains a SliceViewer and a LineViewer widget,
 * linked together.
 *
 * @author Janik Zikovsky
 * @date October 13, 2011
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER SliceViewerWindow
    : public QMainWindow,
      public MantidQt::API::WorkspaceObserver,
      public MantidQt::API::IProjectSerialisable {
  Q_OBJECT

public:
  SliceViewerWindow(const QString &wsName, const QString &label = QString(),
                    Qt::WFlags f = 0);
  ~SliceViewerWindow() override;
  MantidQt::SliceViewer::SliceViewer *getSlicer();
  MantidQt::SliceViewer::LineViewer *getLiner();
  const QString &getLabel() const;
  /// Load the state of the slice viewer from a Mantid project file
  static API::IProjectSerialisable *loadFromProject(const std::string &lines,
                                                    ApplicationWindow *app,
                                                    const int fileVersion);
  /// Save the state of the slice viewer to a Mantid project file
  virtual std::string saveToProject(ApplicationWindow *app) override;
  /// Get the name of the window
  std::string getWindowName() override;
  /// Get the workspaces associated with this window
  std::vector<std::string> getWorkspaceNames() override;
  /// Get the window type as a string
  std::string getWindowType() override;

private:
  void setLineViewerValues(QPointF start2D, QPointF end2D, double width);
  void initMenus();

signals:
  void needToClose();
  void needToUpdate();

protected slots:
  void closeWindow();
  void updateWorkspace();
  void slicerWorkspaceChanged();
  void changedSlicePoint(Mantid::Kernel::VMD);
  void lineChanging(QPointF start, QPointF end, double width);
  void lineChanged(QPointF start, QPointF end, double width);
  void changeStartOrEnd(Mantid::Kernel::VMD, Mantid::Kernel::VMD);
  void changePlanarWidth(double);
  void resizeWindow();
  void lineViewer_changedFixedBinWidth(bool fixed, double binWidth);
  void showLineViewer(bool);
  void showPeaksViewer(bool);

protected:
  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void resizeEvent(QResizeEvent *event) override;

  void renameHandle(const std::string &oldName,
                    const std::string &newName) override;

  /// The SliceViewer
  MantidQt::SliceViewer::SliceViewer *m_slicer;

  /// The LineViewer
  MantidQt::SliceViewer::LineViewer *m_liner;

  /// The PeaksViewer
  MantidQt::SliceViewer::PeaksViewer *m_peaksViewer;

  /// Horizontal splitter between slice viewer and LineViewer
  QSplitter *m_splitter;

  /// Workspace being looked at
  Mantid::API::IMDWorkspace_sptr m_ws;
  /// Name of the workspace being viewed
  std::string m_wsName;

  /// Additional label for identifying the window.
  QString m_label;

  /// Width of the LineViewer last time it was open
  int m_lastLinerWidth;

  /// Width of the PeaksViewer last time it was open
  int m_lastPeaksViewerWidth;

  /// Window width
  int m_desiredWidth;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif // SLICEVIEWERWINDOW_H
