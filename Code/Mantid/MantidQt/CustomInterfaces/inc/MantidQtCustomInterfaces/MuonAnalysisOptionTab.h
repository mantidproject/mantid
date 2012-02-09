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
  ///
  void runTimeComboBox(int index);
  ///
  void runTimeAxisStartAtInput();
  ///
  void runTimeAxisFinishAtInput();
  ///
  void runyAxisMinimumInput();
  ///
  void runyAxisMaximumInput();
  ///
  void runyAxisAutoscale(bool state);
  ///
  void runRebinComboBox(int index);
  ///
  void runOptionStepSizeText();

signals:
  ///
  void settingsTabUpdatePlot();

private:
  ///
  Ui::MuonAnalysis& m_uiForm;
  /// group defaults are saved to
  const QString& m_settingsGroup;

private slots:

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
