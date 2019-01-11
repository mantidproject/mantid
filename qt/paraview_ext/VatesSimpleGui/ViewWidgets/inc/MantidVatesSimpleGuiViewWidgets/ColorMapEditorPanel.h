// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef COLORMAPEDITORPANEL_H
#define COLORMAPEDITORPANEL_H

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "ui_ColorMapEditorPanel.h"
#include <QDialog>
#include <QWidget>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
  This class handles the color map editor.

  @date 11/05/2015
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorMapEditorPanel
    : public QDialog {
  Q_OBJECT
public:
  /// Default constructor.
  ColorMapEditorPanel(QWidget *parent = nullptr);
  /// Default destructor.
  ~ColorMapEditorPanel() override;
  /// Connect the panel to ParaView
  void setUpPanel();
  /// Filter events to check for show events.
  bool eventFilter(QObject *obj, QEvent *ev) override;

signals:
  void showPopUpWindow();
  void hidePopUpWindow();

public slots:
  // Show the window pop up
  void onShowPopUpWindow();
  // Hide the pop up window
  void onHidePopUpWindow();

private:
  Ui::ColorMapEditorPanel ui; ///< The dialog's UI form
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // COLORMAPEDITORPANEL_H
