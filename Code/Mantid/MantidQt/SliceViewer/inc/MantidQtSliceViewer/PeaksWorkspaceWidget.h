#ifndef PEAKSWORKSPACEWIDGET_H
#define PEAKSWORKSPACEWIDGET_H

#include <QtGui/QWidget>
#include "DllOption.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "ui_PeaksWorkspaceWidget.h"

namespace MantidQt
{
namespace SliceViewer
{

  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksWorkspaceWidget : public QWidget
  {
    Q_OBJECT
  public:
    PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws, QWidget *parent = 0);
    ~PeaksWorkspaceWidget();
  private:
    /// Populate the widget with model data.
    void populate();
    /// Auto-generated UI controls.
    Ui::PeaksWorkspaceWidget ui;
    /// Peaks workspace to view.
    Mantid::API::IPeaksWorkspace_const_sptr m_ws;
  private slots:
      void expandChanged(bool);
  };

} //namespace
}
#endif // PEAKSWORKSPACEWIDGET_H
