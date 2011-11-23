#ifndef SNAPTOGRIDDIALOG_H
#define SNAPTOGRIDDIALOG_H

#include <QtGui/QDialog>
#include "ui_SnapToGridDialog.h"

/** Dialog that prompts the user
 * to enter the X and Y snap-to
 * grid values.
 *
 * For use with the SliceViewer.
 */
class SnapToGridDialog : public QDialog
{
  Q_OBJECT

public:
  SnapToGridDialog(QWidget *parent = 0);
  ~SnapToGridDialog();

  void setSnap(double x, double y);
  double getSnapX();
  double getSnapY();

private:
  Ui::SnapToGridDialogClass ui;
  /// Was cancel pressed?
  bool m_cancelled;
};

#endif // SNAPTOGRIDDIALOG_H
