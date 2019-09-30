// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTER_H_

#include "DllConfig.h"
#include "MantidQtWidgets/Common/IFunctionBrowser.h"
#include "MantidQtWidgets/Common/IMuonFitFunctionModel.h"
#include "MuonAnalysisHelper.h"
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

/** MuonAnalysisFitFunctionPresenter : Updates fit browser from function widget

  Handles interaction between FunctionBrowser widget (View) and fit property
  browser (Model).
  Implemented as a QObject to handle signals and slots.
*/
class MANTIDQT_MUONINTERFACE_DLL MuonAnalysisFitFunctionPresenter : QObject {
  Q_OBJECT
public:
  /// Constructor
  MuonAnalysisFitFunctionPresenter(
      QObject *parent,
      MantidQt::MantidWidgets::IMuonFitFunctionModel *fitBrowser,
      MantidQt::MantidWidgets::IFunctionBrowser *funcBrowser);
  /// Toggle multiple fitting mode
  void setMultiFitState(Muon::MultiFitState state);

  /// Set function in model (fit property browser)
  void setFunctionInModel(const Mantid::API::IFunction_sptr &function);
public slots:
  /// Update function and pass to fit property browser
  void updateFunction();
  /// Update function and pass to fit property browser, then fit
  void updateFunctionAndFit(bool sequential);
  /// When fit finished, update parameters in function browser
  void handleFitFinished(const QString &wsName = "");
  /// When parameter edited in function browser, update in fit property browser
  void handleParameterEdited(const QString &funcIndex,
                             const QString &paramName);
  /// When "Clear model" selected, clear function browser
  void handleModelCleared();
  /// Pass show/hide parameter errors to function browser
  void handleErrorsEnabled(bool enabled);
  /// When number of datasets to fit changes, update function browser
  void updateNumberOfDatasets(int nDatasets);
  /// When user changes dataset index, update function browser
  void handleDatasetIndexChanged(int index);

private:
  /// Connect signals and slots
  void doConnect();
  /// Suspend updates to function parameters, or turn back on
  void setParameterUpdates(bool on);
  /// Non-owning pointer to muon fit property browser
  MantidQt::MantidWidgets::IMuonFitFunctionModel *m_fitBrowser;
  /// Non-owning pointer to function browser widget
  MantidQt::MantidWidgets::IFunctionBrowser *m_funcBrowser;
  /// Whether multi fitting is disabled(function browser is hidden) or enabled
  Muon::MultiFitState m_multiFitState;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTER_H_ */