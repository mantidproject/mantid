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
  class DLLExport ConvFit : public IDATab
  {
    Q_OBJECT

  public:
    ConvFit(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void typeSelection(int index);
    void bgTypeSelection(int index);
    void newDataLoaded(const QString wsName);
    void plotInput();
    void plotGuess(QtProperty*);
    void singleFit();
    void specMinChanged(int value);
    void specMaxChanged(int value);
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
    double getInstrumentResolution(std::string workspaceName);
    QtProperty* createLorentzian(const QString &);
    void createTemperatureCorrection(Mantid::API::CompositeFunction_sptr product);
    void populateFunction(Mantid::API::IFunction_sptr func, Mantid::API::IFunction_sptr comp, QtProperty* group, const std::string & pref, bool tie);
    QString fitTypeString() const;
    QString backgroundString() const;

    QtStringPropertyManager* m_stringManager;
    QtTreePropertyBrowser* m_cfTree;
    QMap<QtProperty*, QtProperty*> m_fixedProps;
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_cfInputWS;
    QString m_cfInputWSName;
    bool m_confitResFileType;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
