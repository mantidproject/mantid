// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
class MantidMatrixDialog : public QDialog {
  Q_OBJECT

public:
  //! Constructor
  /**
   * @param parent :: parent widget
   * @param fl :: window flags
   */
  MantidMatrixDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);
  void setMatrix(MantidMatrix *m);

private slots:
  //! Accept changes and quit
  void accept() override;
  //! Apply changes
  void apply();

private:
  MantidMatrix *d_matrix;

  QPushButton *buttonOk;
  QPushButton *buttonCancel;
  QSpinBox *boxColWidth, *boxPrecision;
  QComboBox *boxFormat;
  QLineEdit *editRangeMin, *editRangeMax;
};

#endif // MANTIDMATRIXDIALOG_H
