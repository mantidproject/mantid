#ifndef MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_
#define MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "ui_ApplyCorr.h"
#include "IDATab.h"

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
    /// Handle abs. correction algorithm completion
    void absCorComplete(bool error);
    /// Handle convert units and save algorithm completion
    void postProcessComplete(bool error);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

    void addRebinStep(QString toRebin, QString toMatch);
    void addInterpolationStep(Mantid::API::MatrixWorkspace_sptr toInterpolate, std::string toMatch);

    Ui::ApplyCorr m_uiForm;

    std::string m_originalSampleUnits;

  };

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_APPLYCORR_H_ */
