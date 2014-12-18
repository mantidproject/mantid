#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "MantidQtCustomInterfaces/IDATab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"

#include "boost/shared_ptr.hpp"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class ConvFit : public IDATab
  {
    Q_OBJECT

  public:
    ConvFit(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() {return "ConvFit";}

  private slots:
    void typeSelection(int index);
    void bgTypeSelection(int index);
    void plotInput();
    void plotGuess(QtProperty*);
    void singleFit();
    void minChanged(double);
    void maxChanged(double);
    void backgLevel(double);
    void updateRS(QtProperty*, double);
    void checkBoxUpdate(QtProperty*, bool);
    void hwhmChanged(double);
    void hwhmUpdateRS(double);
    void fitContextMenu(const QPoint &);
    void fixItem();
    void unFixItem();
    void showTieCheckbox(QString);

  private:
    boost::shared_ptr<Mantid::API::CompositeFunction> createFunction(bool tieCentres=false);
    QtProperty* createLorentzian(const QString &);
    void createTemperatureCorrection(Mantid::API::CompositeFunction_sptr product);
    void populateFunction(Mantid::API::IFunction_sptr func, Mantid::API::IFunction_sptr comp, QtProperty* group, const std::string & pref, bool tie);
    QString fitTypeString() const;
    QString backgroundString() const;

    QIntValidator * m_intVal;
    QtStringPropertyManager* m_stringManager;
    QtTreePropertyBrowser* m_cfTree;
    QwtPlot* m_cfPlot;
    QMap<QString, QtProperty*> m_cfProp;
    QMap<QtProperty*, QtProperty*> m_fixedProps;
    MantidWidgets::RangeSelector* m_cfRangeS;
    MantidWidgets::RangeSelector* m_cfBackgS;
    MantidWidgets::RangeSelector* m_cfHwhmRange;
    QtGroupPropertyManager* m_cfGrpMng;
    QtDoublePropertyManager* m_cfDblMng;
    QtBoolPropertyManager* m_cfBlnMng;
    QwtPlotCurve* m_cfDataCurve;
    QwtPlotCurve* m_cfCalcCurve;
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_cfInputWS;
    QString m_cfInputWSName;
    bool m_confitResFileType;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
