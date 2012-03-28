#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtMantidWidgets/MWDiag.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QTableWidget>

namespace Ui
{
  class MuonAnalysis;
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{


/** 
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.    

@author Anders Markvardsen, ISIS, RAL

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class MuonAnalysisOptionTab : public QWidget
{
 Q_OBJECT
public:
  /// Constructor
  MuonAnalysisOptionTab(Ui::MuonAnalysis& uiForm, const QString& group) : m_uiForm(uiForm), m_settingsGroup(group) {}

  /// Initialise the layout of Muon Analysis.
  void initLayout();

  /// When no data loaded set various buttons etc to inactive
  void noDataAvailable();

  /// When data loaded set various buttons etc to active
  void nowDataAvailable();


public slots:
  /// Set the run time in muon analysis and save into settings.
  void runTimeComboBox(int index);

  /// Enable/Disable editing of Y axis and save the setting.
  void runyAxisAutoscale(bool state);

  /// Set whether the user can see and edit the rebin steps. Also saves setting.
  void runRebinComboBox(int index);


signals:
  /// Update the plot because something has changed.
  void settingsTabUpdatePlot();

  /// Tell Muon interface to show the muon graphs
  void notHidingGraphs();


private:
  /// The Muon Analysis UI file.
  Ui::MuonAnalysis& m_uiForm;
  
  /// group defaults are saved to
  const QString& m_settingsGroup;


private slots:  
  /// Save the settings for time axis start and validate the entry.
  void runTimeAxisStartAtInput();

  /// Save the settings for time axis end and validate the entry.
  void runTimeAxisFinishAtInput();

  /// Save the settings for Y axis min and validate the entry.
  void runyAxisMinimumInput();

  /// Save the settings for Y axis max and validate the entry.
  void runyAxisMaximumInput();

  /// Save the settings for rebin steps and validate the entry.
  void runOptionStepSizeText();

  /// Open the Muon Analysis Plotting help (Wiki).
  void muonAnalysisHelpSettingsClicked();
  
  /// Save the settings of plot creation.
  void plotCreationChanged(int);
  
  /// Save the settings of plot type.
  void plotTypeChanged(int);
  
  /// Save the settings of whether to show error bars.
  void errorBarsChanged(bool);
  
  /// Save the settings of whether to show the toolbars.
  void toolbarsChanged(bool);

  /// Save the settings of whether to show the previous graphs.
  void hideGraphsChanged(bool);

  /// Validate the Y minimum.
  void validateYMin();
  
  /// Validate the Y maximum.
  void validateYMax();
  
  /// Opens the managed directory dialog for easier access for the user.
  void openDirectoryDialog();
};

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
