#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "MantidQtCustomInterfaces/IDATab.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "boost/shared_ptr.hpp"

namespace Mantid
{
  namespace API
  {
    class IFunction;
    class CompositeFunction;
  }
}

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
    void sequential();
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

  private:
    boost::shared_ptr<Mantid::API::CompositeFunction> createFunction(bool tie=false);
    QtProperty* createLorentzian(const QString &);
    void populateFunction(boost::shared_ptr<Mantid::API::IFunction>, boost::shared_ptr<Mantid::API::IFunction>, QtProperty*, const std::string & pref, const bool tie=false);
      
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
    std::string m_cfInputWSName;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
