#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "IndirectConvFitModel.h"
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
  void enablePlotResult() override;
  void disablePlotResult() override;
  void enableSaveResult() override;
  void disableSaveResult() override;
  void enablePlotPreview() override;
  void disablePlotPreview() override;
  void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void removeGuessPlot() override;

private:
  void setup() override;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void setModelFitFunction() override;
  void algorithmComplete(bool error) override;
  void newDataLoaded(const QString &wsName);
  void updatePreviewPlots() override;
  void updatePlotRange() override;
  void singleFit();
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
  bool m_confitResFileType;

  // ShortHand Naming for fit functions
  QHash<QString, std::string> m_fitStrings;
  QHash<QString, std::string> m_fitTypeToFunction;
  IndirectConvFitModel *m_convFittingModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
