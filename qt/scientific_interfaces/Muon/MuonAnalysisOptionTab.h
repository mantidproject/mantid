// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"

#include "MuonAnalysisHelper.h"

namespace MantidQt {
namespace CustomInterfaces {

namespace Muon {

/**
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.

@author Anders Markvardsen, ISIS, RAL
*/

class MuonAnalysisOptionTab : public QWidget {
  Q_OBJECT
public:
  /// Types of the start time
  enum StartTimeType { FirstGoodData, TimeZero, Custom };

  /// Type of rebin
  enum RebinType { NoRebin, FixedRebin, VariableRebin };

  /// Types of new plot policies
  enum NewPlotPolicy { NewWindow, PreviousWindow };

  /// Constructor
  MuonAnalysisOptionTab(Ui::MuonAnalysis &uiForm, const QString &settingsGroup);

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

  /// Return multiple fitting mode on/off selection
  Muon::MultiFitState getMultiFitState() const;

signals:
  /// Update the plot because something has changed.
  void settingsTabUpdatePlot();

  /// Emitted when plot style parameters has changed.
  void plotStyleChanged();

  /// Emitted when multi fitting mode is turned on/off
  void multiFitStateChanged(int state);
  void loadAllGroupChanged(int state);
  void loadAllPairsChanged(int state);

private:
  /// Default widget values
  static const QString START_TIME_DEFAULT;
  static const QString FINISH_TIME_DEFAULT;
  static const QString MIN_Y_DEFAULT;
  static const QString MAX_Y_DEFAULT;
  static const QString FIXED_REBIN_DEFAULT;
  static const QString VARIABLE_REBIN_DEFAULT;

  /// The Muon Analysis UI file.
  Ui::MuonAnalysis &m_uiForm;

  /// Auto-saver for all the widgets
  MuonAnalysisHelper::WidgetAutoSaver m_autoSaver;

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
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
