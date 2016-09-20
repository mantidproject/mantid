#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "ui_MSDFit.h"
#include "IndirectDataAnalysisTab.h"

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
  void algorithmComplete(bool error);

private:
  Ui::MSDFit m_uiForm;
  QString m_currentWsName;
  QtTreePropertyBrowser *m_msdTree;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
