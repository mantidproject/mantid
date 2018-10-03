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

  void setRunEnabled(bool enabled);
  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);

  void setRunIsRunning(bool running);
  void setPlotResultIsPlotting(bool plotting);

private slots:
  virtual void algorithmComplete(bool error);
  void saveClicked();
  void plotClicked();
  void runClicked();
  void getBeamDefaults(const QString &dataName);
  void getMonteCarloDefaults(const QString &dataName);
  void setWavelengthsValue(Mantid::Geometry::Instrument_const_sptr instrument,
                           const std::string &wavelengthsParamName) const;
  void setEventsValue(Mantid::Geometry::Instrument_const_sptr instrument,
                      const std::string &eventsParamName) const;
  void setInterpolationValue(Mantid::Geometry::Instrument_const_sptr instrument,
                             const std::string &interpolationParamName) const;
  void setMaxAttemptsValue(Mantid::Geometry::Instrument_const_sptr instrument,
                           const std::string &maxAttemptsParamName) const;
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
