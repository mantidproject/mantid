// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LABELTOOLLOGVALUESDIALOG_H_
#define LABELTOOLLOGVALUESDIALOG_H_

//----------------------------------
// Includes
//----------------------------------

#include "MantidSampleLogDialog.h"

//----------------------------------
// Forward declarations
//----------------------------------

class ApplicationWindow;
class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QRadioButton;
class QLabel;
class QLineEdit;
class QSpinBox;

/**
This class display a Sample Log Dialog for the Label Tool,
it is used to select a Sample Log and import it on the plot
as a label.

@author Dimitar Tasev, Mantid Development Team, STFC
@date 14/07/2016
*/
class LabelToolLogValuesDialog : public SampleLogDialogBase {
  Q_OBJECT
public:
  /// Constructor
  LabelToolLogValuesDialog(const QString &wsname, QWidget *parentContainer,
                           Qt::WFlags flags = nullptr,
                           size_t experimentInfoIndex = 0);

  virtual ~LabelToolLogValuesDialog() override;

private slots:

  /// Override providing custom functionality to the import function
  virtual void importItem(QTreeWidgetItem *item) override;

private:
  /// Tracks which statistic of the log is selected
  QRadioButton *statRadioChoice[NUM_STATS];
};

#endif // LABELTOOLLOGVALUESDIALOG_H_