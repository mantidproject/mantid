#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "MantidQtCustomInterfaces/IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport MSDFit : public IDATab
  {
    Q_OBJECT

  public:
    MSDFit(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() { return "MSDFit"; }

  private slots:
    void singleFit();
    void plotFit(QString wsName);
    void newDataLoaded(const QString wsName);
    void plotInput();
    void specMinChanged(int value);
    void specMaxChanged(int value);
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty* prop, double val);

  private:
    QString m_currentWsName;
    QtTreePropertyBrowser* m_msdTree;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
