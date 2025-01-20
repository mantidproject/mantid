// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IInstrumentView.h"
#include "ui_InstrumentWidget.h"
#include <QCheckBox>
#include <QLineEdit>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** QtInstrumentView : Provides an interface for the "Instrument" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtInstrumentView : public QWidget, public IInstrumentView {
  Q_OBJECT
public:
  QtInstrumentView(const Mantid::API::IAlgorithm_sptr &algorithmForTooltips, QWidget *parent = nullptr);
  void subscribe(InstrumentViewSubscriber *notifyee) override;
  void connectInstrumentSettingsWidgets() override;
  void disconnectInstrumentSettingsWidgets() override;

  int getMonitorIndex() const override;
  void setMonitorIndex(int value) override;
  bool getIntegrateMonitors() const override;
  void setIntegrateMonitors(bool value) override;

  double getLambdaMin() const override;
  void setLambdaMin(double value) override;
  double getLambdaMax() const override;
  void setLambdaMax(double value) override;
  void showLambdaRangeInvalid() override;
  void showLambdaRangeValid() override;

  double getMonitorBackgroundMin() const override;
  void setMonitorBackgroundMin(double value) override;
  double getMonitorBackgroundMax() const override;
  void setMonitorBackgroundMax(double value) override;
  void showMonitorBackgroundRangeInvalid() override;
  void showMonitorBackgroundRangeValid() override;

  double getMonitorIntegralMin() const override;
  void setMonitorIntegralMin(double value) override;
  double getMonitorIntegralMax() const override;
  void setMonitorIntegralMax(double value) override;
  void showMonitorIntegralRangeInvalid() override;
  void showMonitorIntegralRangeValid() override;
  void showCalibrationFilePathInvalid() override;
  void showCalibrationFilePathValid() override;

  bool getCorrectDetectors() const override;
  void setCorrectDetectors(bool value) override;
  std::string getDetectorCorrectionType() const override;
  void setDetectorCorrectionType(std::string const &value) override;

  std::string getCalibrationFilePath() const override;
  void setCalibrationFilePath(std::string const &value) override;

  void disableAll() override;
  void enableAll() override;
  void enableDetectorCorrectionType() override;
  void disableDetectorCorrectionType() override;

public slots:
  void onSettingsChanged();
  void onRestoreDefaultsRequested();
  void browseToCalibrationFile();

private:
  /// Initialise the interface
  void initLayout();
  void registerSettingsWidgets(const Mantid::API::IAlgorithm_sptr &alg);
  void registerInstrumentSettingsWidgets(const Mantid::API::IAlgorithm_sptr &alg);
  void setToolTipAsPropertyDocumentation(QWidget &widget, std::string const &propertyName,
                                         const Mantid::API::IAlgorithm_sptr &alg);

  template <typename Widget>
  void registerSettingWidget(Widget &widget, std::string const &propertyName, const Mantid::API::IAlgorithm_sptr &alg);
  void connectSettingsChange(QLineEdit &edit);
  void connectSettingsChange(QComboBox &edit);
  void connectSettingsChange(QCheckBox &edit);
  void connectSettingsChange(QSpinBox &edit);
  void connectSettingsChange(QDoubleSpinBox &edit);
  void disconnectSettingsChange(QLineEdit &edit);
  void disconnectSettingsChange(QComboBox &edit);
  void disconnectSettingsChange(QCheckBox &edit);
  void disconnectSettingsChange(QSpinBox &edit);
  void disconnectSettingsChange(QDoubleSpinBox &edit);
  void setSelected(QComboBox &box, std::string const &str);
  void setText(QLineEdit &lineEdit, int value);
  void setText(QLineEdit &lineEdit, double value);
  void setText(QLineEdit &lineEdit, std::string const &value);
  void setText(QLineEdit &lineEdit, std::optional<int> value);
  void setText(QLineEdit &lineEdit, boost::optional<double> value);
  void setText(QLineEdit &lineEdit, boost::optional<std::string> const &value);
  void setChecked(QCheckBox &checkBox, bool checked);
  std::string getText(QLineEdit const &lineEdit) const;
  std::string getText(QComboBox const &box) const;

  Ui::InstrumentWidget m_ui;
  InstrumentViewSubscriber *m_notifyee;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
