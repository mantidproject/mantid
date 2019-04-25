// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BINDIALOG_H_
#define BINDIALOG_H_

#include <QCheckBox>
#include <QDialog>

// Qt forward declarations
class QLineEdit;
class QRadioButton;

namespace MantidQt {
namespace MantidWidgets {

/**
\class  BinDialog
\brief  class to display Bin selection dialog
\author Srikanth Nagella
\date   November 2008
\version 1.0

BinDialog class handles the Input Dialog for bin selection:
e.g. enter bin range from X1 to X2.
*/

class BinDialog : public QDialog {
  Q_OBJECT

public:
  explicit BinDialog(QWidget *parent = nullptr);
  ~BinDialog() override;
  void setIntegralMinMax(double /*minBin*/, double /*maxBin*/,
                         bool /*useEverything*/);
signals:
  /// This signal is sent when changing the bin range selected.
  /// Parameters are: min, max, and a bool set to true to mean "everything"
  void IntegralMinMax(double /*_t1*/, double /*_t2*/, bool /*_t3*/);

public slots:
  void btnOKClicked();
  void mEntireRange_toggled(bool on);

private:
  QLineEdit *mIntegralMinValue;
  QLineEdit *mIntegralMaxValue;
  QCheckBox *mEntireRange;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*BINDIALOG_H_*/
