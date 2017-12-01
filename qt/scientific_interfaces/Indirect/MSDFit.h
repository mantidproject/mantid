#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "ui_MSDFit.h"

#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport MSDFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  MSDFit(QWidget *parent = nullptr);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void singleFit();
  void newDataLoaded(const QString wsName);
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void saveClicked();
  void plotClicked();
  void algorithmComplete(bool error) override;
  void modelSelection(int selected);
  void updatePreviewPlots() override;
  void plotGuess();

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;
  Mantid::API::IAlgorithm_sptr msdFitAlgorithm(const std::string &model,
                                               int specMin, int specMax);
  std::string modelToAlgorithmProperty(const QString &model);
  Mantid::API::IFunction_sptr createFunction(const QString &modelName);
  Mantid::API::IFunction_sptr
  getFunction(const QString &functionName) const override;

  Ui::MSDFit m_uiForm;
  QtTreePropertyBrowser *m_msdTree;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
