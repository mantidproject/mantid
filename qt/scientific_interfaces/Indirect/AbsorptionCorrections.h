// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_

#include "CorrectionsTab.h"
#include "ui_AbsorptionCorrections.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

class DLLExport AbsorptionCorrections : public CorrectionsTab {
  Q_OBJECT

public:
  AbsorptionCorrections(QWidget *parent = nullptr);
  ~AbsorptionCorrections();

  Mantid::API::MatrixWorkspace_sptr sampleWorkspace() const;

private slots:
  virtual void algorithmComplete(bool error);
  void saveClicked();
  void plotClicked();
  void runClicked();
  void getParameterDefaults(QString const &dataName);
  void setSampleDensityOptions(QString const &method);
  void setCanDensityOptions(QString const &method);
  void setSampleDensityUnit(QString const &text);
  void setCanDensityUnit(QString const &text);
  void setSampleDensityValue(QString const &text);
  void setCanDensityValue(QString const &text);
  void changeSampleMaterialOptions(int index);
  void changeCanMaterialOptions(int index);
  void setSampleDensity(double value);
  void setCanDensity(double value);
  UserInputValidator doValidation();

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  void addSaveWorkspace(std::string const &wsName);
  void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg,
                                     QString shape);
  void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg,
                                  QString const &shape);

  void processWavelengthWorkspace();
  void convertSpectrumAxes(Mantid::API::WorkspaceGroup_sptr correctionsWs);
  void convertSpectrumAxes(Mantid::API::WorkspaceGroup_sptr correctionsGroup,
                           Mantid::API::MatrixWorkspace_sptr sample);
  void convertSpectrumAxes(Mantid::API::MatrixWorkspace_sptr correction,
                           Mantid::API::MatrixWorkspace_sptr sample);

  void getParameterDefaults(Mantid::Geometry::Instrument_const_sptr instrument);
  void setBeamWidthValue(Mantid::Geometry::Instrument_const_sptr instrument,
                         std::string const &beamWidthParamName) const;
  void setBeamHeightValue(Mantid::Geometry::Instrument_const_sptr instrument,
                          std::string const &beamHeightParamName) const;
  void setWavelengthsValue(Mantid::Geometry::Instrument_const_sptr instrument,
                           std::string const &wavelengthsParamName) const;
  void setEventsValue(Mantid::Geometry::Instrument_const_sptr instrument,
                      std::string const &eventsParamName) const;
  void setInterpolationValue(Mantid::Geometry::Instrument_const_sptr instrument,
                             std::string const &interpolationParamName) const;
  void setMaxAttemptsValue(Mantid::Geometry::Instrument_const_sptr instrument,
                           std::string const &maxAttemptsParamName) const;

  void setComboBoxOptions(QComboBox *combobox,
                          std::vector<std::string> const &options);

  std::vector<std::string> getDensityOptions(QString const &method) const;
  std::string getDensityType(std::string const &type) const;
  std::string getNumberDensityUnit(std::string const &type) const;
  QString getDensityUnit(QString const &type) const;
  double getSampleDensityValue(QString const &type) const;
  double getCanDensityValue(QString const &type) const;

  void setRunEnabled(bool enabled);
  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPlotResultIsPlotting(bool plotting);

  Ui::AbsorptionCorrections m_uiForm;

  std::shared_ptr<Densities> m_sampleDensities;
  std::shared_ptr<Densities> m_canDensities;
  Mantid::API::IAlgorithm_sptr m_absCorAlgo;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ABSORPTIONCORRECTIONS_H_ */
