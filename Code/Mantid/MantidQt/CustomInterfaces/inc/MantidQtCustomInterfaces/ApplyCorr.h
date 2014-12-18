#ifndef MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_
#define MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_

#include "MantidAPI/MatrixWorkspace.h"
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

  private slots:
    void handleGeometryChange(int index);
    void newData(const QString &dataName);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
    virtual QString helpURL() {return "AbsCor";}

    QDoubleValidator *m_valPosDbl; ///< validator for positive double inputs.

    bool validateScaleInput(); ///< validate input for Scale option.
    /// ask the user if they wish to rebin the can
    bool requireCanRebin();

  private slots:
    void scaleMultiplierCheck(bool state); ///< handle checking/unchecking of "Scale: Multiply Container by"
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_ */
