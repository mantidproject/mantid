#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_

#include "ui_IqtFit.h"
#include "IDATab.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

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
  class DLLExport IqtFit : public IDATab
  {
    Q_OBJECT

  public:
    IqtFit(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void typeSelection(int index);
    void newDataLoaded(const QString wsName);
    void updatePlot();
    void specMinChanged(int value);
    void specMaxChanged(int value);
    void xMinSelected(double val);
    void xMaxSelected(double val);
    void backgroundSelected(double val);
    void propertyChanged(QtProperty*, double);
    void singleFit();
    void plotGuess(QtProperty*);
    void fitContextMenu(const QPoint &);
    void fixItem();
    void unFixItem();

  private:
    boost::shared_ptr<Mantid::API::CompositeFunction> createFunction(bool tie=false);
    boost::shared_ptr<Mantid::API::IFunction> createUserFunction(const QString & name, bool tie=false);
    QtProperty* createExponential(const QString &);
    QtProperty* createStretchedExp(const QString &);
    void setDefaultParameters(const QString& name);
    QString fitTypeString() const;
    void constrainIntensities(Mantid::API::CompositeFunction_sptr func);

    Ui::IqtFit m_uiForm;
    QtStringPropertyManager* m_stringManager;
    QtTreePropertyBrowser* m_ffTree; ///< IqtFit Property Browser
    QtDoublePropertyManager* m_ffRangeManager; ///< StartX and EndX for IqtFit
    QMap<QtProperty*, QtProperty*> m_fixedProps;
    Mantid::API::MatrixWorkspace_sptr m_ffInputWS;
    Mantid::API::MatrixWorkspace_sptr m_ffOutputWS;
    QString m_ffInputWSName;
    QString m_ties;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
