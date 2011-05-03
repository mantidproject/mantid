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

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class MantidSampleLogDialog : public QDialog
{
  Q_OBJECT
  
public:
  //Constructor
  MantidSampleLogDialog(const QString & wsname, MantidUI* mui, Qt::WFlags flags = 0);

private slots:
  //Plot logs
  void importSelectedLogs();

  // Show the stats of the selected log
  void showLogStatistics();
  void showLogStatisticsOfItem(QTreeWidgetItem * item);

  //Context menu popup
  void popupMenu(const QPoint & pos);

  //Import a single item
  void importItem(QTreeWidgetItem *item);

private:
  //Initialize the layout
  void init();
  
  //A tree widget
  QTreeWidget *m_tree;

  //The workspace name
  QString m_wsname;

  //Buttons to do things  
  QPushButton *buttonPlot, *buttonClose;

  // Filter radio buttons
  QRadioButton *filterNone, *filterStatus, *filterPeriod, *filterStatusPeriod;
  
  // Stats labels
  QLabel* statLabels[5]; //minLabel, maxLabel, meanLabel, medianLabel, stddevLabel;

  // Testboxes with stats data
  QLineEdit  * statValues[5];


  //A pointer to the MantidUI object
  MantidUI* m_mantidUI;
  /// these values are used to specify the format of the log file, all of which are stored as strings
  enum logType
  {
    string,                           ///< indicates the log is a string, no other known formating
    numTSeries,                       ///< for time series properties that contain numbers
    stringTSeries,                    ///< for logs that are string time series properties
    numeric                           ///< for logs that are single numeric values (int or double)
  };
};

#endif //MANTIDSAMPLELOGDIALOG_H_
