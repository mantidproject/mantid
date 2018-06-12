#ifndef MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_

#include "CorrectionsTab.h"
#include "ui_AbsorptionCorrections.h"

#include "../General/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport AbsorptionCorrections : public CorrectionsTab {
  Q_OBJECT

public:
  AbsorptionCorrections(QWidget *parent = nullptr);
  ~AbsorptionCorrections();

  Mantid::API::MatrixWorkspace_sptr sampleWorkspace() const;

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

private slots:
  virtual void algorithmComplete(bool error);
  void saveClicked();
  void plotClicked();
  void getBeamDefaults(const QString &dataName);
  void changeSampleDensityUnit(int);
  void changeCanDensityUnit(int);
  UserInputValidator doValidation();

private:
  void addSaveWorkspace(QString wsName);
  void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg,
                                     QString shape);
  void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg,
                                  QString shape);

  Ui::AbsorptionCorrections m_uiForm;
  /// alg
  Mantid::API::IAlgorithm_sptr m_absCorAlgo;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_ */
