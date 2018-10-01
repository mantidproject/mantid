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

protected slots:
  void setModelResolution(const QString &resolutionName);
  void runClicked();
  void saveClicked();
  void plotClicked();
  void updatePlotOptions() override;
  void fitFunctionChanged();

protected:
  bool shouldEnablePlotResult() override { return true; };

  void setPlotResultEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;

  void setRunIsRunning(bool running) override;

private:
  void setupFitTab() override;
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

  void setRunEnabled(bool enabled);
  void setFitSingleSpectrumEnabled(bool enabled);

  void setPlotResultIsPlotting(bool plotting);

  std::string fitTypeString() const;

  std::unique_ptr<Ui::ConvFit> m_uiForm;

  // ShortHand Naming for fit functions
  QHash<QString, std::string> m_fitStrings;
  ConvFitModel *m_convFittingModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
