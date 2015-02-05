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
  class DLLExport ApplyCorr : public IDATab
  {
    Q_OBJECT

  public:
    ApplyCorr(QWidget * parent = 0);

  private slots:
    /// Handles the geometry being changed
    void handleGeometryChange(int index);
    /// Handles a new sample being loaded
    void newData(const QString &dataName);
    /// Updates the preview mini plot
    void plotPreview(int specIndex);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);
    bool validateScaleInput(); ///< validate input for Scale option.
    /// ask the user if they wish to rebin the can
    bool requireCanRebin();
    /// Pointer to the result workspace (for plotting)
    Mantid::API::MatrixWorkspace_sptr outputWs;

  private slots:
    void scaleMultiplierCheck(bool state); ///< handle checking/unchecking of "Scale: Multiply Container by"
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_ */
