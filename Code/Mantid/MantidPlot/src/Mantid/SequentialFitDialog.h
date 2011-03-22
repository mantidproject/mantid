#ifndef SEQUENTIALFITDIALOG_H
#define SEQUENTIALFITDIALOG_H

//----------------------------
//   Includes
//----------------------------

#include "ui_SequentialFitDialog.h"
#include "MantidAPI/AlgorithmObserver.h"

//----------------------------
//   Forward declarations
//----------------------------

class FitPropertyBrowser;

/** 
    This is a dialog for doing sequential fit.
    (Calls algorithm PlotPeakByLogValue)

    @author Roman Tolchenov, Tessella plc
    @date 4/06/2010

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
class SequentialFitDialog : public QDialog, public Mantid::API::AlgorithmObserver
{
  Q_OBJECT

public:
  
  /// Default constructor
  SequentialFitDialog(FitPropertyBrowser* fitBrowser);

  /// Add a list of workspace names to the data list
  /// Returns false if neither of the workspaces can be loaded
  bool addWorkspaces(const QStringList wsNames);

private:

  /// The form generated with Qt Designer
  Ui::SequentialFitDialog ui;

  /// Pointer to the calling fit browser
  FitPropertyBrowser* m_fitBrowser;

signals:

  /// This signal is fired from finishHandle running in the algorithm's thread
  /// and caught by showPlot slot in the GUI thread
  void needShowPlot();

private slots:

  /// Add a workspace to the data list
  void addWorkspace();
  /// Add a file to the data list
  void addFile();
  /// Remove an input item
  void removeItem();
  /// Actions in response to change of function
  void functionChanged();

  /// Start the fit and close dialog
  void accept();

  /// Show the result plot
  void showPlot();
  /// Display the help page for PlotPeakByLogValue algorithm
  void helpClicked();

  /// called when spectra or workspace index change
  void spectraChanged(int row,int col);

  void plotAgainstLog(bool);

private:

  /// Checks that the logs in workspace wsName are consistent
  /// with logs of other workspaces
  bool validateLogs(const QString wsName);

  /// Populate parameter combo box with possible parameter names
  void populateParameters();

  /// Called when the fit is finished
  void finishHandle(const Mantid::API::IAlgorithm* alg);

  /// set spectrum value for workspace/file in row row
  void setSpectrum(int row,int spec);

  /// set workspace index for workspace/file in row row
  void setWSIndex(int row,int wi);

  void setRange(int row,double from,double to);

  int rowCount()const;

  int defaultSpectrum()const;

  QString name(int row)const;

};



#endif /* SEQUENTIALFITDIALOG_H */
