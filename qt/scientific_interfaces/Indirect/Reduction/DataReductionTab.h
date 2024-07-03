// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "Common/InelasticTab.h"
#include "Common/InstrumentConfig.h"
#include "Common/OutputPlotOptionsPresenter.h"
#include "Common/RunWidget/RunPresenter.h"

#include "MantidQtWidgets/Common/AlgorithmRunner.h"

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
class IDataReduction;

/** DataReductionTab

  This class defines common functionality of tabs used in the Indirect Data
  Reduction interface.

  @author Samuel Jackson
  @date 13/08/2013
*/
class MANTIDQT_INDIRECT_DLL DataReductionTab : public InelasticTab {
  Q_OBJECT

public:
  DataReductionTab(IDataReduction *idrUI, QObject *parent = nullptr);
  DataReductionTab(IDataReduction *idrUI, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner);
  ~DataReductionTab() override;

  /// Set the presenter for the run widget
  void setRunWidgetPresenter(std::unique_ptr<RunPresenter> presenter);
  /// Set the presenter for the output plotting options
  void setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter);
  /// Set the active workspaces used in the plotting options
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);

public slots:
  void runTab();

signals:
  /// Update the Run button on the IDR main window
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString message = "Run", QString tooltip = "");
  /// Emitted when the instrument setup is changed
  void newInstrumentConfiguration();

protected:
  Mantid::API::MatrixWorkspace_sptr instrumentWorkspace() const;

  QMap<QString, QString> getInstrumentDetails() const;
  QString getInstrumentDetail(QString const &key) const;
  QString getInstrumentDetail(QMap<QString, QString> const &instrumentDetails, QString const &key) const;
  void validateInstrumentDetail(QString const &key) const;
  bool hasInstrumentDetail(QString const &key) const;
  bool hasInstrumentDetail(QMap<QString, QString> const &instrumentDetails, QString const &key) const;
  MantidWidgets::IInstrumentConfig *getInstrumentConfiguration() const;
  QString getInstrumentName() const;
  QString getAnalyserName() const;
  QString getReflectionName() const;

  std::map<std::string, double> getRangesFromInstrument(QString instName = "", QString analyser = "",
                                                        QString reflection = "");

protected:
  IDataReduction *m_idrUI;
  std::unique_ptr<API::IAlgorithmRunner> m_algorithmRunner;
  std::unique_ptr<RunPresenter> m_runPresenter;

private slots:
  void tabExecutionComplete(bool error);
  void handleNewInstrumentConfiguration();

private:
  virtual void setFileExtensionsByName(bool filter) { UNUSED_ARG(filter); };
  virtual void updateInstrumentConfiguration() = 0;

  std::unique_ptr<OutputPlotOptionsPresenter> m_plotOptionsPresenter;
  bool m_tabRunning;
};
} // namespace CustomInterfaces
} // namespace MantidQt
