#ifndef MANTIDQTCUSTOMINTERFACESIDA_ABSCORR_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ABSCORR_H_

#include "ui_AbsCorr.h"
#include "IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport AbsCorr : public IDATab
  {
    Q_OBJECT

  public:
    CalcCorr(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private:
    Ui::CalcCorr m_uiForm;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ABSCORR_H_ */
