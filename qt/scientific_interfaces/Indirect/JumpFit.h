#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_

#include "IndirectFitAnalysisTab.h"
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
  void handleWidthChange(const QString &text);
  /// Find all spectra with width data in the workspace
  void findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws);
  /// Handles plotting results of algorithm on miniplot
  void algorithmComplete(bool error) override;
  /// Handles plotting and saving
  void updatePreviewPlots() override;
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void updatePlotRange() override;
  void saveClicked();
  void plotClicked();
  void updatePlotOptions() override;

protected:
  size_t getWidth() const;
  int minimumSpectrum() const override;
  int maximumSpectrum() const override;

  std::string createSingleFitOutputName() const override;
  Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const override;

  Mantid::API::IAlgorithm_sptr
  deleteWorkspaceAlgorithm(const std::string &workspaceName);

  Mantid::API::IAlgorithm_sptr
  scaleAlgorithm(const std::string &workspaceToScale,
                 const std::string &outputName, double scaleFactor);

  void enablePlotResult() override;
  void disablePlotResult() override;
  void enableSaveResult() override;
  void disableSaveResult() override;
  void enablePlotPreview() override;
  void disablePlotPreview() override;
  void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void removeGuessPlot() override;

private:
  std::map<std::string, size_t>
  findAxisLabelsWithSubstrings(Mantid::API::TextAxis *axis,
                               const std::vector<std::string> &substrings,
                               const size_t &maximumNumber) const;

  void disablePlotGuess() override;
  void enablePlotGuess() override;

  // The UI form
  std::unique_ptr<Ui::JumpFit> m_uiForm;

  /// Map of axis labels to spectrum number
  std::map<std::string, size_t> m_spectraList;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
