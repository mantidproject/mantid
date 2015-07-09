#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "ui_MSDFit.h"
#include "IndirectDataAnalysisTab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport MSDFit : public IndirectDataAnalysisTab
  {
    Q_OBJECT

  public:
    MSDFit(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void singleFit();
    void plotFit(QString wsName = QString(), int specNo = -1);
    void newDataLoaded(const QString wsName);
    void plotInput();
    void specMinChanged(int value);
    void specMaxChanged(int value);
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty* prop, double val);

  private:
    Ui::MSDFit m_uiForm;
    QString m_currentWsName;
    QtTreePropertyBrowser* m_msdTree;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
