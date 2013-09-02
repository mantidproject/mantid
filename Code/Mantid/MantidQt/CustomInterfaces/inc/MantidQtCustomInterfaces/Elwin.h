#ifndef MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class Elwin : public IDATab
  {
    Q_OBJECT

  public:
    Elwin(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() {return "Elwin";}
    void setDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws);

  private slots:
    void plotInput();
    void twoRanges(QtProperty *, bool);
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty * prop, double val);

  private:
    QwtPlot* m_elwPlot;
    MantidWidgets::RangeSelector* m_elwR1;
    MantidWidgets::RangeSelector* m_elwR2;
    QwtPlotCurve* m_elwDataCurve;
    QtTreePropertyBrowser* m_elwTree;
    QMap<QString, QtProperty*> m_elwProp;
    QtDoublePropertyManager* m_elwDblMng;
    QtBoolPropertyManager* m_elwBlnMng;
    QtGroupPropertyManager* m_elwGrpMng;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_ */
