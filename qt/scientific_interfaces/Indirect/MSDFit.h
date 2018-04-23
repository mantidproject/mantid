#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "MSDFitModel.h"
#include "ui_MSDFit.h"

#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport MSDFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  MSDFit(QWidget *parent = nullptr);

  bool doPlotGuess() const override;

private:
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void singleFit();
  void newDataLoaded(const QString wsName);
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void plotClicked();
  void algorithmComplete(bool error) override;
  void updatePreviewPlots() override;
  void updatePlotRange() override;
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
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  MSDFitModel *m_msdFittingModel;
  std::unique_ptr<Ui::MSDFit> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
