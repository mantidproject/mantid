#ifndef SNAPTOGRIDDIALOG_H
#define SNAPTOGRIDDIALOG_H

#include "DllOption.h"
#include "ui_SnapToGridDialog.h"
#include <QDialog>

/** Dialog that prompts the user
 * to enter the X and Y snap-to
 * grid values.
 *
 * For use with the SliceViewer.
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER SnapToGridDialog : public QDialog {
  Q_OBJECT

public:
  SnapToGridDialog(QWidget *parent = nullptr);
  ~SnapToGridDialog() override;

  void setSnap(double x, double y);
  double getSnapX();
  double getSnapY();

private:
  Ui::SnapToGridDialogClass ui;
};

#endif // SNAPTOGRIDDIALOG_H
