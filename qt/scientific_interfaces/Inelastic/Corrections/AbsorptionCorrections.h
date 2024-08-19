// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "CorrectionsTab.h"
#include "ui_AbsorptionCorrections.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL AbsorptionCorrections : public CorrectionsTab, public IRunSubscriber {
  Q_OBJECT

public:
  AbsorptionCorrections(QWidget *parent = nullptr);
  ~AbsorptionCorrections();

  Mantid::API::MatrixWorkspace_sptr sampleWorkspace() const;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "AbsorptionCorrections"; }

private slots:
  virtual void algorithmComplete(bool error);
  void saveClicked();
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
  void handlePresetShapeChanges(int index);

private:
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  void validateSampleGeometryInputs(IUserInputValidator *uiv, const QString &shape) const;
  void validateContainerGeometryInputs(IUserInputValidator *uiv, const QString &shape) const;

  void addSaveWorkspace(std::string const &wsName);
  void addShapeSpecificSampleOptions(const Mantid::API::IAlgorithm_sptr &alg, const QString &shape);
  void addShapeSpecificCanOptions(const Mantid::API::IAlgorithm_sptr &alg, QString const &shape);

  void processWavelengthWorkspace();
  void convertSpectrumAxes(const Mantid::API::WorkspaceGroup_sptr &correctionsWs);
  void convertSpectrumAxes(const Mantid::API::WorkspaceGroup_sptr &correctionsGroup,
                           const Mantid::API::MatrixWorkspace_sptr &sample);
  void convertSpectrumAxes(const Mantid::API::MatrixWorkspace_sptr &correction,
                           const Mantid::API::MatrixWorkspace_sptr &sample);

  void getParameterDefaults(const Mantid::Geometry::Instrument_const_sptr &instrument);
  void setBeamWidthValue(const Mantid::Geometry::Instrument_const_sptr &instrument,
                         std::string const &beamWidthParamName) const;
  void setBeamHeightValue(const Mantid::Geometry::Instrument_const_sptr &instrument,
                          std::string const &beamHeightParamName) const;
  void setEventsValue(const Mantid::Geometry::Instrument_const_sptr &instrument,
                      std::string const &eventsParamName) const;
  void setInterpolationValue(const Mantid::Geometry::Instrument_const_sptr &instrument,
                             std::string const &interpolationParamName) const;
  void setMaxAttemptsValue(const Mantid::Geometry::Instrument_const_sptr &instrument,
                           std::string const &maxAttemptsParamName) const;

  void setComboBoxOptions(QComboBox *combobox, std::vector<std::string> const &options);

  std::vector<std::string> getDensityOptions(QString const &method) const;
  std::string getDensityType(std::string const &type) const;
  std::string getNumberDensityUnit(std::string const &type) const;
  QString getDensityUnit(QString const &type) const;
  double getSampleDensityValue(QString const &type) const;
  double getCanDensityValue(QString const &type) const;

  void setSaveResultEnabled(bool enabled);

  bool m_saveAlgRunning{false};

  Ui::AbsorptionCorrections m_uiForm;

  std::shared_ptr<Densities> m_sampleDensities;
  std::shared_ptr<Densities> m_canDensities;
  Mantid::API::IAlgorithm_sptr m_absCorAlgo;
};
} // namespace CustomInterfaces
} // namespace MantidQt
