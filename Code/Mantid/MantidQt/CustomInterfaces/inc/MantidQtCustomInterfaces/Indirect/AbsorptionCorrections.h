#ifndef MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_

#include "ui_AbsorptionCorrections.h"
#include "IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport AbsorptionCorrections : public IDATab
  {
    Q_OBJECT

  public:
    AbsorptionCorrections(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    virtual void algorithmComplete(bool error);

  private:
    void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg, QString shape);
    void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg, QString shape);

    Ui::AbsorptionCorrections m_uiForm;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_ */
