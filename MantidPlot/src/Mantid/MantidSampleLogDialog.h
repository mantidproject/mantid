#ifndef MANTIDSAMPLELOGDIALOG_H_
#define MANTIDSAMPLELOGDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include <QDialog>
#include <QPoint>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include "MantidAPI/ExperimentInfo.h"
#include <QDoubleSpinBox>

//----------------------------------
// Forward declarations
//----------------------------------
class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QRadioButton;
class MantidUI;

/** 
This class displays a list of log files for a selected workspace. It
allows the user to plot selected log files.

@author Martyn Gigg, Tessella Support Services plc
@date 05/11/2009

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MantidSampleLogDialog : public QDialog
{
  Q_OBJECT

public:
  ///Constructor
  MantidSampleLogDialog(const QString & wsname, MantidUI* mui, Qt::WFlags flags = 0, size_t experimentInfoIndex = 0);

  private slots:
    ///Plot logs
    void importSelectedLogs();

    /// Show the stats of the selected log
    void showLogStatistics();
    void showLogStatisticsOfItem(QTreeWidgetItem * item);

    ///Context menu popup
    void popupMenu(const QPoint & pos);

    ///Import a single item
    void importItem(QTreeWidgetItem *item);

    void selectExpInfoNumber(int num);

private:
  ///Initialize the layout
  void init();

  ///A tree widget
  QTreeWidget *m_tree;

  ///The workspace name
  std::string m_wsname;

  /// Index into the ExperimentInfo list.
  size_t m_experimentInfoIndex;

  /// The actual experiment info being looked at.
  Mantid::API::ExperimentInfo_const_sptr m_ei;

  ///Buttons to do things
  QPushButton *buttonPlot, *buttonClose;

  /// Filter radio buttons
  QRadioButton *filterNone, *filterStatus, *filterPeriod, *filterStatusPeriod;

  /// Number of statistic values
  static const std::size_t NUM_STATS = 7;

  /// Stats labels
  QLabel* statLabels[NUM_STATS]; //minLabel, maxLabel, meanLabel, timeAverageLabel, medianLabel, stddevLabel, durationLabel;

  /// Testboxes with stats data
  QLineEdit  * statValues[NUM_STATS];

  /// Widget to select the # of the experiment info to look at.
  QSpinBox * m_spinNumber;

  ///A pointer to the MantidUI object
  MantidUI* m_mantidUI;
  /// these values are used to specify the format of the log file, all of which are stored as strings
  enum logType
  {
    string,                           ///< indicates the log is a string, no other known formating
    numTSeries,                       ///< for time series properties that contain numbers
    stringTSeries,                    ///< for logs that are string time series properties
    numeric,                          ///< for logs that are single numeric values (int or double)
    numericArray                      ///< for logs that are an array of numeric values (int or double)
  };
};

#endif //MANTIDSAMPLELOGDIALOG_H_
