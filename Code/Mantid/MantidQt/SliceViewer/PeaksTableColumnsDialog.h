#ifndef PEAKSTABLECOLUMNSDIALOG_H
#define PEAKSTABLECOLUMNSDIALOG_H

#include <QDialog>

namespace Ui {
class PeaksTableColumnsDialog;
}

class PeaksTableColumnsDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit PeaksTableColumnsDialog(QWidget *parent = 0);
  ~PeaksTableColumnsDialog();
  
private:
  Ui::PeaksTableColumnsDialog *ui;
};

#endif // PEAKSTABLECOLUMNSDIALOG_H
