#ifndef MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_

#include "MantidQtCustomInterfaces/IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class CalcCorr : public IDATab
  {
    Q_OBJECT

  public:
    CalcCorr(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() {return "AbsF2P";}

  private slots:
    void shape(int index);
    void useCanChecked(bool checked);
    void tcSync();

  private:
    QDoubleValidator * m_dblVal;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_ */
