#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "IndirectDataAnalysisTab.h"
#include "ui_MSDFit.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport MSDFit : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  MSDFit(QWidget *parent = nullptr);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

private slots:
  void singleFit();
  void newDataLoaded(const QString wsName);
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void saveClicked();
  void plotClicked();
  void algorithmComplete(bool error);
  void modelSelection(int selected);
  void updatePlot(int specNo);
  void updateProperties(int specNo);

private:
  Mantid::API::IAlgorithm_sptr msdFitAlgorithm(const std::string &model,
                                               long specMin, long specMax);
  QtProperty *createModel(const QString &modelName,
                          const std::vector<QString> &modelParameters);
  QHash<QString, QString> createParameterToPropertyMap(const QString &model);
  std::string modelToAlgorithmProperty(const QString &model);

  Ui::MSDFit m_uiForm;
  QtTreePropertyBrowser *m_msdTree;

  QHash<QString, QHash<size_t, double>> m_parameterValues;
  QHash<QString, QString> m_parameterToProperty;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
