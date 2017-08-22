#ifndef SNAPTOGRIDDIALOG_H
#define SNAPTOGRIDDIALOG_H

#include <QtGui/QDialog>
#include "ui_SnapToGridDialog.h"
#include "DllOption.h"

/** Dialog that prompts the user
 * to enter the X and Y snap-to
 * grid values.
 *
 * For use with the SliceViewer.
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER SnapToGridDialog : public QDialog {
  Q_OBJECT

public:
  SnapToGridDialog(QWidget *parent = 0);
  ~SnapToGridDialog() override;

  void setSnap(double x, double y);
  double getSnapX();
  double getSnapY();

private:
  Ui::SnapToGridDialogClass ui;
};

#endif // SNAPTOGRIDDIALOG_H
