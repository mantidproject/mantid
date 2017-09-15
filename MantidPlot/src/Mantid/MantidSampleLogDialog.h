#ifndef MANTIDSAMPLELOGDIALOG_H_
#define MANTIDSAMPLELOGDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "SampleLogDialogBase.h"

// Qt
#include <QTextEdit>
#include <QLineEdit>
#include <QDoubleSpinBox>

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

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MantidSampleLogDialog : public SampleLogDialogBase {
  Q_OBJECT

public:
  /// Constructor
  MantidSampleLogDialog(const QString &wsname, MantidUI *mui,
                        Qt::WFlags flags = 0, size_t experimentInfoIndex = 0);

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
