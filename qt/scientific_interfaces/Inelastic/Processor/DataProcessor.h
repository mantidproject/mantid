// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"
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

class MANTIDQT_INELASTIC_DLL IDataProcessor {
public:
  virtual ~IDataProcessor() = default;
  virtual void setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter>) = 0;
  virtual void clearOutputPlotOptionsWorkspaces() = 0;
  virtual void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) = 0;
  virtual void filterInputData(bool filter) = 0;
  virtual void enableLoadHistoryProperty(bool doLoadHistory) = 0;
  virtual void exportPythonDialog() = 0;
  virtual API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(const std::string &wsName,
                                                            const std::string &filename = "") = 0;
};
/** DataProcessor

  This class defines common functionality of tabs used in the Inelastic Data
  Processor interface.
*/
class MANTIDQT_INELASTIC_DLL DataProcessor : public IDataProcessor,
                                             public InelasticTab,
                                             public MantidQt::API::IAlgorithmRunnerSubscriber {

public:
  DataProcessor(QObject *parent = nullptr, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner = nullptr);
  ~DataProcessor() override = default;
  /// Set the presenter for the output plotting options
  void setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter) override;
  /// Overridden from IAlgorithmRunnerSubscriber: Notifies when a batch of algorithms is completed
  void notifyBatchComplete(API::IConfiguredAlgorithm_sptr &algorithm, bool error) override;

  /// Clear the workspaces held by the output plotting options
  void clearOutputPlotOptionsWorkspaces() override;
  /// Set the active workspaces used in the plotting options
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) override;

  API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(const std::string &wsName,
                                                    const std::string &filename = "") override;
  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter) override;
  void enableLoadHistoryProperty(bool doLoadHistory) override;

  void exportPythonDialog() override;

protected:
  virtual void runComplete(bool error) { (void)error; };
  std::unique_ptr<MantidQt::API::IAlgorithmRunner> m_algorithmRunner;

private:
  virtual void setFileExtensionsByName(bool filter) { (void)filter; };
  virtual void setLoadHistory(bool doLoadHistory) { (void)doLoadHistory; }
  std::unique_ptr<OutputPlotOptionsPresenter> m_plotOptionsPresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
