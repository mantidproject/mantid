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
  MSDFit(QWidget *parent = 0);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

private slots:
  void singleFit();
  void plotFit(QString wsName = QString(), int specNo = -1);
  void newDataLoaded(const QString wsName);
  void plotInput();
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void saveClicked();
  void plotClicked();
  void plotCurrentPreview();
  void algorithmComplete(bool error);
  void modelSelection(int selected);

private:
  Mantid::API::IAlgorithm_sptr msdFitAlgorithm(long specMin, long specMax);
  QtProperty *createModel(const QString &modelName,
                          const std::vector<QString> modelParameters);

  Ui::MSDFit m_uiForm;
  QtTreePropertyBrowser *m_msdTree;
  Mantid::API::MatrixWorkspace_sptr m_msdInputWS;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
