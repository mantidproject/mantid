// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtIntPropertyManager"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Plotting/ExternalPlotter.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNamePresenter.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

#include <boost/none_t.hpp>
#include <optional>

#include <QDoubleValidator>
#include <QMap>
#include <QPair>

#include <algorithm>
#include <map>
#include <unordered_map>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {

/** InelasticTab

Provided common functionality of all indirect interface tabs.

@author Dan Nixon
@date 08/10/2014
*/
class MANTID_SPECTROSCOPY_DLL InelasticTab : public QObject {
  Q_OBJECT

public:
  InelasticTab(QObject *parent = nullptr);
  virtual ~InelasticTab() override = default;

  void displayWarning(std::string const &message);

protected:
  /// Set the presenter for the run widget
  void setRunWidgetPresenter(std::unique_ptr<RunPresenter> presenter);

  /// Set the presenter for the output name widget
  void setOutputNamePresenter(IOutputNameView *view);

  /// Set the presenter for the output plotting options
  void setOutputPlotOptionsPresenter(
      IOutputPlotOptionsView *view, PlotWidget const &plotType = PlotWidget::Spectra,
      std::string const &fixedIndices = "",
      std::optional<std::map<std::string, std::string>> const &availableActions = std::nullopt);

  /// Run the load algorithms
  bool loadFile(const std::string &filename, const std::string &outputName, const int specMin = -1,
                const int specMax = -1, bool loadHistory = true);

  /// Add a SaveNexusProcessed step to the batch queue
  void addSaveWorkspaceToQueue(const std::string &wsName, const std::string &filename = "");
  void addSaveWorkspaceToQueue(const QString &wsName, const QString &filename = "");

  /// Function to set the range limits of the plot
  void setPlotPropertyRange(MantidWidgets::RangeSelector *rs, QtProperty *min, QtProperty *max,
                            const std::pair<double, double> &bounds);
  /// Function to set the range selector on the mini plot
  void setRangeSelector(MantidWidgets::RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                        const std::pair<double, double> &range,
                        const std::optional<std::pair<double, double>> &bounds = std::nullopt);
  /// Sets the min of the range selector if it is less than the max
  void setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty,
                           MantidWidgets::RangeSelector *rangeSelector, double newValue);
  /// Sets the max of the range selector if it is more than the min
  void setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty,
                           MantidWidgets::RangeSelector *rangeSelector, double newValue);

  /// Function to run an algorithm on a seperate thread
  void runAlgorithm(const Mantid::API::IAlgorithm_sptr &algorithm);

  /// Checks the ADS for a workspace named `workspaceName`,
  /// opens a warning box for plotting/saving if none found
  bool checkADSForPlotSaveWorkspace(const std::string &workspaceName, const bool plotting, const bool warn = true);

  std::unique_ptr<RunPresenter> m_runPresenter;

  std::unique_ptr<OutputNamePresenter> m_outputNamePresenter;

  std::unique_ptr<OutputPlotOptionsPresenter> m_plotOptionsPresenter;

  /// Parent QWidget (if applicable)
  QWidget *m_parentWidget;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;

  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;

  /// Double manager to create properties
  QtDoublePropertyManager *m_dblManager;
  /// Boolean manager to create properties
  QtBoolPropertyManager *m_blnManager;
  /// Group manager to create properties
  QtGroupPropertyManager *m_grpManager;

  /// Double editor facotry for the properties browser
  DoubleEditorFactory *m_dblEdFac;
  /// QtCheckBoxFactory
  QtCheckBoxFactory *m_blnEdFac;

  /// Algorithm runner object to execute chains algorithms on a seperate thread
  /// from the GUI
  MantidQt::API::BatchAlgorithmRunner *m_batchAlgoRunner;

  /// Validator for int inputs
  QIntValidator *m_valInt;
  /// Validator for double inputs
  QDoubleValidator *m_valDbl;
  /// Validator for positive double inputs
  QDoubleValidator *m_valPosDbl;

  Mantid::Types::Core::DateAndTime m_tabStartTime;
  Mantid::Types::Core::DateAndTime m_tabEndTime;
  std::string m_pythonExportWsName;

  std::unique_ptr<Widgets::MplCpp::ExternalPlotter> m_plotter;
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;

public slots:
  void exportPythonScript();

protected slots:
  /// Slot to handle when an algorithm finishes running
  virtual void algorithmFinished(bool error);

private slots:
  virtual void handleDataReady(QString const &dataName) { UNUSED_ARG(dataName); };

signals:
  /// Send signal to parent window to show a message box to user
  void showMessageBox(const std::string &message);
};
} // namespace CustomInterfaces
} // namespace MantidQt
