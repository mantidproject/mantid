#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_

#include "ui_JumpFit.h"
#include "IndirectDataAnalysisTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport JumpFit : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  JumpFit(QWidget *parent = 0);

  // Inherited methods from IndirectDataAnalysisTab
  void setup();
  bool validate();
  void run();
  void runImpl(bool plot = false, bool save = false);
  /// Load default settings into the interface
  void loadSettings(const QSettings &settings);

private slots:
  /// Handle when the sample input is ready
  void handleSampleInputReady(const QString &filename);
  /// Slot to handle plotting a different spectrum of the workspace
  void handleWidthChange(const QString &text);
  /// Slot for when the range on the range selector changes
  void qRangeChanged(double min, double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val);
  /// Find all spectra with width data in the workspace
  void findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws);
  /// Handles plotting results of algorithm on miniplot
  void fitAlgDone(bool error);
  /// Handles plotting fit result in MantidPlot
  void plotFitResult(bool error);
  /// Handles running preview algorithm
  void runPreviewAlgorithm();
  /// Handles a fit algorithm being selected
  void fitFunctionSelected(const QString &functionName);

private:
  /// Gets a list of parameter names for a given fit function
  QStringList getFunctionParameters(const QString &functionName);

  // The UI form
  Ui::JumpFit m_uiForm;

  // Map of axis labels to spectrum number
  std::map<std::string, int> m_spectraList;

  QtTreePropertyBrowser* m_jfTree;

  Mantid::API::IAlgorithm_sptr m_fitAlg;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
