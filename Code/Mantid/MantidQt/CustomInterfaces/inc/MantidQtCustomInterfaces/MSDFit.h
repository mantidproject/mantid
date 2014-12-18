#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "MantidQtCustomInterfaces/IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class MSDFit : public IDATab
  {
    Q_OBJECT

  public:
    MSDFit(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() {return "MSDFit";}

  private slots:
    void singleFit();
    void plotFit(QString wsName);
    void plotInput();
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty* prop, double val);
    
  private:
    QString currentWsName;
    QIntValidator * m_intVal;
    QwtPlot* m_msdPlot;
    MantidWidgets::RangeSelector* m_msdRange;
    QwtPlotCurve* m_msdDataCurve;
    QwtPlotCurve* m_msdFitCurve;
    QtTreePropertyBrowser* m_msdTree;
    QMap<QString, QtProperty*> m_msdProp;
    QtDoublePropertyManager* m_msdDblMng;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
