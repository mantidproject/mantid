#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQT_H_

#include "IndirectDataAnalysisTab.h"
#include "ui_Iqt.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport Iqt : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  Iqt(QWidget *parent = nullptr);

protected:
  void setRunEnabled(bool enabled) override;

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

private slots:
  void algorithmComplete(bool error);
  void plotInput(const QString &wsname);
  void rsRangeChangedLazy(double min, double max);
  void updateRS(QtProperty *prop, double val);
  void updatePropertyValues(QtProperty *prop, double val);
  void updateDisplayedBinParameters();
  void runClicked();
  void saveClicked();
  void plotClicked();
  void plotTiled();

private:
  Ui::Iqt m_uiForm;
  QtTreePropertyBrowser *m_iqtTree;
  bool m_iqtResFileType;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQT_H_ */
