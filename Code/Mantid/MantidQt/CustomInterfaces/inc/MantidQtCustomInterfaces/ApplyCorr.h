#ifndef MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_
#define MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_

#include "MantidQtCustomInterfaces/IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class ApplyCorr : public IDATab
  {
    Q_OBJECT

  public:
    ApplyCorr(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() {return "AbsCor";}
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_ */
