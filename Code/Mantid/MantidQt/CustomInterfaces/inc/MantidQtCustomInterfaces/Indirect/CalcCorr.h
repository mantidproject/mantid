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

    bool doValidation(bool silent = false);

  private slots:
    void absCorComplete(bool error);
    void postProcessComplete(bool error);
    void getBeamWidthFromWorkspace(const QString& wsName);

  private:
    void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg, QString shape);
    void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg, QString shape);

    Ui::CalcCorr m_uiForm;

  };

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CALCCORR_H_ */
