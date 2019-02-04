// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDSAMPLELOGDIALOG_H_
#define MANTIDSAMPLELOGDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "SampleLogDialogBase.h"

// Qt
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTextEdit>

//----------------------------------
// Forward declarations
//----------------------------------
class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QRadioButton;
class MantidUI;
class ApplicationWindow;

/**
This class displays a list of log files for a selected workspace. It
allows the user to plot selected log files.

@author Martyn Gigg, Tessella Support Services plc
@date 05/11/2009
*/
class MantidSampleLogDialog : public SampleLogDialogBase {
  Q_OBJECT

public:
  /// Constructor
  MantidSampleLogDialog(const QString &wsname, MantidUI *mui,
                        Qt::WFlags flags = nullptr,
                        size_t experimentInfoIndex = 0);

  /// Destructor
  virtual ~MantidSampleLogDialog() override;

  /// Which type of filtering is selected
  Mantid::API::LogFilterGenerator::FilterType getFilterType() const override;

protected slots:

  /// Import a single item
  virtual void importItem(QTreeWidgetItem *item) override;

protected:
  /// Filter radio buttons
  QRadioButton *filterNone, *filterStatus, *filterPeriod, *filterStatusPeriod;

  /// Stats labels
  QLabel *statLabels[NUM_STATS]; // minLabel, maxLabel, meanLabel,
                                 // timeAverageLabel, medianLabel, stddevLabel,
                                 // durationLabel;

  /// A pointer to the MantidUI object
  MantidUI *m_mantidUI;
};

#endif // MANTIDSAMPLELOGDIALOG_H_
