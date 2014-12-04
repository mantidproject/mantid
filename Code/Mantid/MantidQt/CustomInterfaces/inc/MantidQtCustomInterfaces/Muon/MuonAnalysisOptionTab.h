#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"

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

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  /// Types of the start time
  enum StartTimeType { FirstGoodData, TimeZero, Custom };

  /// Type of rebin
  enum RebinType { NoRebin, FixedRebin, VariableRebin };

  /// Types of new plot policies
  enum NewPlotPolicy { NewWindow, PreviousWindow };

  /// Constructor
  MuonAnalysisOptionTab(Ui::MuonAnalysis& uiForm, const QString& settingsGroup);

  /// Initialise the layout of the tab
  void initLayout();

  /// Get plot style parameters from widgets
  QMap<QString, QString> parsePlotStyleParams() const;

  /// Retrieve selected type of the start time
  StartTimeType getStartTimeType();

  /// Retrieve custom start time value
  double getCustomStartTime();

  /// Retrieve custom finish time value
  double getCustomFinishTime();

  /// Retrieve a type of rebin user has selected
  RebinType getRebinType();

  /// Retrieve a vairable rebin params string as specified by user
  std::string getRebinParams();

  /// Retrieve a binning step as specified by user
  double getRebinStep();

  /// Return currently selected new plot policy
  NewPlotPolicy newPlotPolicy();

signals:
  /// Update the plot because something has changed.
  void settingsTabUpdatePlot();

  /// Emitted when plot style parameters has changed.
  void plotStyleChanged();

private:
  /// Default widget values
  static const QString START_TIME_DEFAULT;
  static const QString FINISH_TIME_DEFAULT;
  static const QString MIN_Y_DEFAULT;
  static const QString MAX_Y_DEFAULT;
  static const QString FIXED_REBIN_DEFAULT;
  static const QString VARIABLE_REBIN_DEFAULT;

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
