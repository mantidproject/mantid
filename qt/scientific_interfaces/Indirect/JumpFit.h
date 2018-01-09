#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "ui_JumpFit.h"

#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport JumpFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  JumpFit(QWidget *parent = nullptr);

  // Inherited methods from IndirectDataAnalysisTab
  void setup() override;
  bool validate() override;
  void run() override;
  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

protected slots:
  /// Handle when the sample input is ready
  void handleSampleInputReady(const QString &filename);
  /// Slot to handle plotting a different spectrum of the workspace
  void handleWidthChange(const QString &text);
  /// Slot for when the range on the range selector changes
  void qRangeChanged(double min, double max);
  /// Slot to update the guides when the range properties change
  void updateRS(QtProperty *prop, double val);
  /// Find all spectra with width data in the workspace
  void findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws);
  /// Handles plotting results of algorithm on miniplot
  void algorithmComplete(bool error) override;
  /// Handles a fit algorithm being selected
  void fitFunctionSelected(const QString &functionName);
  /// Generates the plot guess data
  void plotGuess();
  /// Handles plotting and saving
  void updatePreviewPlots() override;
  void saveClicked();
  void plotClicked();

protected:
  /// Creates the algorithm to use in fitting.
  Mantid::API::IAlgorithm_sptr
  createFitAlgorithm(Mantid::API::IFunction_sptr func);

  Mantid::API::IAlgorithm_sptr
  processParametersAlgorithm(const std::string &parameterWSName,
                             const std::string &resultWSName);

  Mantid::API::IAlgorithm_sptr
  deleteWorkspaceAlgorithm(const std::string &workspaceName);

  Mantid::API::IAlgorithm_sptr
  renameWorkspaceAlgorithm(const std::string &workspaceToRename,
                           const std::string &newName);

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  // The UI form
  Ui::JumpFit m_uiForm;

  /// Map of axis labels to spectrum number
  std::map<std::string, int> m_spectraList;

  QtTreePropertyBrowser *m_jfTree;

  std::string m_baseName;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
