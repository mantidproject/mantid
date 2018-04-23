#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "JumpFitModel.h"
#include "ui_JumpFit.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/TextAxis.h"

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
  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  bool doPlotGuess() const override;

protected slots:
  /// Handle when the sample input is ready
  void handleSampleInputReady(const QString &filename);
  /// Slot to handle plotting a different spectrum of the workspace
  void handleWidthChange(int);
  /// Handles plotting results of algorithm on miniplot
  void algorithmComplete(bool error) override;
  /// Handles plotting and saving
  void updatePreviewPlots() override;
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void updatePlotRange() override;
  void plotClicked();
  void updatePlotOptions() override;

protected:
  void enablePlotResult() override;
  void disablePlotResult() override;
  void enableSaveResult() override;
  void disableSaveResult() override;
  void enablePlotPreview() override;
  void disablePlotPreview() override;
  void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void removeGuessPlot() override;

private:
  void setAvailableWidths(const std::vector<std::string> &widths);
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  // The UI form
  JumpFitModel *m_jumpFittingModel;
  std::unique_ptr<Ui::JumpFit> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
