// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SPECTRUM_VIEW_H
#define SPECTRUM_VIEW_H

#include <QList>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <vector>

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/GraphDisplay.h"
#include "MantidQtWidgets/SpectrumViewer/MatrixWSDataSource.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"

/**
    @class SpectrumView

    This is the QMainWindow for the SpectrumView data viewer.  Data is
    displayed in an SpectrumView, by constructing the SpectrumView object and
    specifying a particular data source.

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace Ui {
class SpectrumViewer; // Forward declaration of UI file
}

namespace MantidQt {
namespace SpectrumView {

// Forward declarations
class EModeHandler;
class RangeHandler;
class SliderHandler;
class SpectrumDisplay;
class SVConnections;
class MatrixWSDataSource;

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SpectrumView
    : public QMainWindow,
      public MantidQt::API::WorkspaceObserver,
      public MantidQt::API::IProjectSerialisable {
  Q_OBJECT

public:
  /// Construct a SpectrumView to display data from the specified data source
  SpectrumView(QWidget *parent = nullptr);

  ~SpectrumView() override;
  void renderWorkspace(Mantid::API::MatrixWorkspace_const_sptr wksp);
  void renderWorkspace(const QString &wsName);
  QList<boost::shared_ptr<SpectrumDisplay>> getSpectrumDisplays() const {
    return m_spectrumDisplay;
  }

  void selectData(int spectrumNumber, double dataVal);
  bool isTrackingOn() const;
  /// Load the state of the spectrum viewer from a Mantid project file
  static API::IProjectSerialisable *loadFromProject(const std::string &lines,
                                                    ApplicationWindow *app,
                                                    const int fileVersion);
  /// Save the state of the spectrum viewer to a Mantid project file
  virtual std::string saveToProject(ApplicationWindow *app) override;
  /// Get the name of the window
  std::string getWindowName() override;
  /// Get the workspaces associated with this window
  std::vector<std::string> getWorkspaceNames() override;
  /// Get the window type as a string
  std::string getWindowType() override;

signals:
  void spectrumDisplayChanged(SpectrumDisplay * /*_t1*/);

protected slots:
  void closeWindow();
  void changeSpectrumDisplay(int tab);
  void respondToTabCloseReqest(int tab);
  void changeTracking(bool on);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;

  void dragMoveEvent(QDragMoveEvent *de) override;
  void dragEnterEvent(QDragEnterEvent *de) override;
  void dropEvent(QDropEvent *de) override;

private:
  void updateHandlers();
  void loadSettings();
  void saveSettings() const;
  bool replaceExistingWorkspace(
      const std::string &wsName,
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws);

  std::vector<MatrixWSDataSource_sptr> m_dataSource;
  QList<boost::shared_ptr<SpectrumDisplay>> m_spectrumDisplay;
  boost::shared_ptr<GraphDisplay> m_hGraph;
  boost::shared_ptr<GraphDisplay> m_vGraph;
  boost::shared_ptr<SVConnections> m_svConnections;

  Ui::SpectrumViewer *m_ui;
  SliderHandler *m_sliderHandler;
  RangeHandler *m_rangeHandler;
  EModeHandler *m_emodeHandler;

signals:
  void needToClose();
  void needToUpdate();
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // SPECTRUM_VIEW_H
