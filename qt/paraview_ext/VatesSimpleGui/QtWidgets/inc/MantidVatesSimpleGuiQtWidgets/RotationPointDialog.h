// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ROTATIONPOINTDIALOG_H
#define ROTATIONPOINTDIALOG_H

#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"
#include "ui_RotationPointDialog.h"

#include <QDialog>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
  This class handles providing the coordinates for a center of rotation.

  @date 14/11/2011
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS RotationPointDialog
    : public QDialog {
  Q_OBJECT

public:
  /// Default constructor.
  RotationPointDialog(QWidget *parent = nullptr);
  /// Default destructor.
  ~RotationPointDialog() override;

signals:
  /**
   * Signal to pass along the individual coordinate values.
   * @param x the x coordinate of the point
   * @param y the y coordinate of the point
   * @param z the z coordinate of the point
   */
  void sendCoordinates(double x, double y, double z);

protected slots:
  /// Gather the coordinates from the dialog line editors.
  void getCoordinates();

private:
  Ui::RotationPointDialog ui; ///< The dialog's UI form
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // ROTATIONPOINTDIALOG_H
