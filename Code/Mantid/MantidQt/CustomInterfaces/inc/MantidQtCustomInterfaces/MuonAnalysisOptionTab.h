#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"

#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  using namespace MuonAnalysisHelper;

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/

class MuonAnalysisOptionTab : public QWidget
{
 Q_OBJECT
public:
  /// Constructor
  MuonAnalysisOptionTab(Ui::MuonAnalysis& uiForm, const QString& settingsGroup);

  /// Initialise the layout of the tab
  void initLayout();

  /// Get plot style parameters from widgets
  QMap<QString, QString> parsePlotStyleParams() const;

signals:
  /// Update the plot because something has changed.
  void settingsTabUpdatePlot();

  /// Emitted when plot style parameters has changed.
  void plotStyleChanged();

private:
  /// The Muon Analysis UI file.
  Ui::MuonAnalysis& m_uiForm;

  /// Auto-saver for all the widgets
  WidgetAutoSaver m_autoSaver;

private slots:  
  /// Open the Muon Analysis Settings help (Wiki).
  void muonAnalysisHelpSettingsClicked();

  /// Open the Muon Analysis Settings help and navigate to rebin section. (Wiki)
  void rebinHelpClicked();

  /// Run when time axis combo-box is changed
  void onTimeAxisChanged(int index);

  /// Run when autoscale check-box state is changed
  void onAutoscaleToggled(bool state);
};

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
