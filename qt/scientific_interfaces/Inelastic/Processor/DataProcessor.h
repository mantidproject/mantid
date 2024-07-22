// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

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

/** DataProcessor

  This class defines common functionality of tabs used in the Inelastic Data
  Processor interface.
*/
class MANTIDQT_INELASTIC_DLL DataProcessor : public InelasticTab {
  Q_OBJECT

public:
  DataProcessor(QObject *parent = nullptr);
  ~DataProcessor() override;

  /// Set the presenter for the output plotting options
  void setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter);

  /// Clear the workspaces held by the output plotting options
  void clearOutputPlotOptionsWorkspaces();
  /// Set the active workspaces used in the plotting options
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);

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
