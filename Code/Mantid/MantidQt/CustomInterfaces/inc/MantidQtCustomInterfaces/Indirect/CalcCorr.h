#ifndef MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_

#include "ui_CalcCorr.h"
#include "IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport CalcCorr : public IDATab
  {
    Q_OBJECT

  public:
    CalcCorr(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void shape(int index);
    void useCanChecked(bool checked);
    void tcSync();
    void getBeamWidthFromWorkspace(const QString& wsname);

  private:
    Ui::CalcCorr m_uiForm;
    QDoubleValidator * m_dblVal;
    QDoubleValidator * m_posDblVal;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_ */
