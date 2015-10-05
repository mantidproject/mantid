#ifndef MANTIDMATRIXDIALOG_H
#define MANTIDMATRIXDIALOG_H

#include <QDialog>
#include <QPointer>

class MantidMatrix;
class QPushButton;
class QSpinBox;
class QComboBox;
class QLineEdit;

//! Matrix properties dialog
class MantidMatrixDialog : public QDialog
{
  Q_OBJECT

public:
  //! Constructor
  /**
  * @param parent :: parent widget
  * @param fl :: window flags
  */
  MantidMatrixDialog( QWidget* parent = 0, Qt::WFlags fl = 0 );
  void setMatrix(MantidMatrix *m);

  private slots:
    //! Accept changes and quit
    void accept();
    //! Apply changes
    void apply();

private:
  MantidMatrix* d_matrix;

  QPushButton* buttonOk;
  QPushButton* buttonCancel;
  QSpinBox* boxColWidth, *boxPrecision;
  QComboBox *boxFormat;
  QLineEdit *editRangeMin, *editRangeMax;
};

#endif // MANTIDMATRIXDIALOG_H
