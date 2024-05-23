// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/InelasticTab.h"
#include "Common/OutputPlotOptionsPresenter.h"
#include "DllConfig.h"

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
class DataReduction;

/** DataManipulation

  This class defines common functionality of tabs used in the Indirect Data
  Manipulation interface.
*/
class MANTIDQT_INELASTIC_DLL DataManipulation : public InelasticTab {
  Q_OBJECT

public:
  DataManipulation(QObject *parent = nullptr);
  ~DataManipulation() override;

  /// Set the presenter for the output plotting options
  void setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter);
  /// Set the active workspaces used in the plotting options
  /// Clear the workspaces held by the output plotting options
  void clearOutputPlotOptionsWorkspaces();
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);

public slots:
  void runTab();

signals:
  /// Update the Run button on the IDR main window
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString message = "Run", QString tooltip = "");

private slots:
  void tabExecutionComplete(bool error);

protected:
  virtual void runComplete(bool error) { (void)error; };

private:
  virtual void setFileExtensionsByName(bool filter) { (void)filter; };

  std::unique_ptr<OutputPlotOptionsPresenter> m_plotOptionsPresenter;
  bool m_tabRunning;
};
} // namespace CustomInterfaces
} // namespace MantidQt
