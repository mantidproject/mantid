#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "ConvFitModel.h"
#include "IndirectFitAnalysisTab.h"
#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ConvFit.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport ConvFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  ConvFit(QWidget *parent = nullptr);

  bool canPlotGuess() const override;

  bool doPlotGuess() const override;

protected:
  void setPlotResultEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;
  void enablePlotPreview() override;
  void disablePlotPreview() override;
  void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void removeGuessPlot() override;

private:
  void setupFitTab() override;
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void setModelResolution(const QString &resolutionName);
  void newDataLoaded(const QString &wsName);
  void updatePreviewPlots() override;
  void updatePlotRange() override;
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void backgLevel(double);
  void hwhmMinChanged(double);
  void hwhmMaxChanged(double);
  void updateHWHMFromResolution();
  void saveClicked();
  void plotClicked();
  void updatePlotOptions() override;
  void fitFunctionChanged();
  void parameterUpdated(const Mantid::API::IFunction *function);

private:
  void fwhmChanged(double fwhm);

  void disablePlotGuess() override;
  void enablePlotGuess() override;

  std::string fitTypeString() const;

  std::unique_ptr<Ui::ConvFit> m_uiForm;

  // ShortHand Naming for fit functions
  QHash<QString, std::string> m_fitStrings;
  QHash<QString, std::string> m_fitTypeToFunction;
  ConvFitModel *m_convFittingModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
