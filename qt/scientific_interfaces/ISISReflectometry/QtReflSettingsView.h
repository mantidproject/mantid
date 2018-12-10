// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_QTREFLSETTINGSVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLSETTINGSVIEW_H_

#include "ExperimentOptionDefaults.h"
#include "IReflSettingsView.h"
#include "InstrumentOptionDefaults.h"
#include "ui_ReflSettingsWidget.h"
#include <memory>

namespace MantidQt {

namespace MantidWidgets {
class HintingLineEdit;
}

namespace CustomInterfaces {

// Forward decs
class IReflSettingsPresenter;

/** QtReflSettingsView : Provides an interface for the "Settings" widget in the
ISIS Reflectometry interface.
*/
class QtReflSettingsView : public QWidget, public IReflSettingsView {
  Q_OBJECT
public:
  /// Constructor
  explicit QtReflSettingsView(int group, QWidget *parent = nullptr);
  /// Destructor
  ~QtReflSettingsView() override;
  /// Returns the presenter managing this view
  IReflSettingsPresenter *getPresenter() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;
  /// Return selected analysis mode
  std::string getAnalysisMode() const override;
  /// Return the per-angle options
  std::map<std::string, MantidQt::MantidWidgets::DataProcessor::OptionsQMap>
  getPerAngleOptions() const override;
  /// Return start overlap for transmission runs
  std::string getStartOverlap() const override;
  /// Return end overlap for transmission runs
  std::string getEndOverlap() const override;
  /// Return selected polarisation corrections
  std::string getPolarisationCorrections() const override;
  /// Return CRho
  std::string getCRho() const override;
  /// Return CAlpha
  std::string getCAlpha() const override;
  /// Return CAp
  std::string getCAp() const override;
  /// Return Cpp
  std::string getCPp() const override;
  /// Return FloodCorrection
  std::string getFloodCorrection() const override;
  /// Return FloodWorkspace
  std::string getFloodWorkspace() const override;
  /// Return integrated monitors option
  std::string getIntMonCheck() const override;
  /// Return monitor integral wavelength min
  std::string getMonitorIntegralMin() const override;
  /// Return monitor integral wavelength max
  std::string getMonitorIntegralMax() const override;
  /// Return monitor background wavelength min
  std::string getMonitorBackgroundMin() const override;
  /// Return monitor background wavelength max
  std::string getMonitorBackgroundMax() const override;
  /// Return wavelength min
  std::string getLambdaMin() const override;
  /// Return wavelength max
  std::string getLambdaMax() const override;
  /// Return I0MonitorIndex
  std::string getI0MonitorIndex() const override;
  /// Return selected detector correction type
  std::string getDetectorCorrectionType() const override;
  /// Return selected summation type
  std::string getSummationType() const override;
  /// Return selected reduction type
  std::string getReductionType() const override;
  /// Return debug option
  bool getDebugOption() const override;
  /// Return whether to include partial bins
  bool getIncludePartialBins() const override;
  /// Set the status of whether polarisation corrections should be enabled
  void setIsPolCorrEnabled(bool enable) const override;
  /// Set default values for experiment and instrument settings
  void setExpDefaults(ExperimentOptionDefaults defaults) override;
  void setInstDefaults(InstrumentOptionDefaults defaults) override;
  /// Check if experiment settings are enabled
  bool experimentSettingsEnabled() const override;
  /// Check if instrument settings are enabled
  bool instrumentSettingsEnabled() const override;
  /// Check if detector correction is enabled
  bool detectorCorrectionEnabled() const override;
  /// Creates hints for 'Stitch1DMany'
  void
  createStitchHints(const std::vector<MantidWidgets::Hint> &hints) override;
  void disableAll() override;
  void enableAll() override;

  void showOptionLoadErrors(
      std::vector<InstrumentParameterTypeMissmatch> const &errors,
      std::vector<MissingInstrumentParameterValue> const &missingValues)
      override;

public slots:
  /// Request presenter to obtain default values for settings
  void requestExpDefaults() const;
  void requestInstDefaults() const;
  void summationTypeChanged(int reductionTypeIndex);
  /// Sets enabled status for polarisation corrections and parameters
  void setPolarisationOptionsEnabled(bool enable) override;
  void setReductionTypeEnabled(bool enable) override;
  void setIncludePartialBinsEnabled(bool enable) override;
  void setDetectorCorrectionEnabled(bool enable) override;
  void notifySettingsChanged();
  QString messageFor(
      std::vector<MissingInstrumentParameterValue> const &missingValues) const;
  QString messageFor(const InstrumentParameterTypeMissmatch &typeError) const;
  /// Adds another row to the per-angle options table
  void addPerAngleOptionsTableRow();

private slots:
  void setPolCorPageForIndex(int index);
  void floodCorComboBoxChanged(const QString &text);

private:
  /// Initialise the interface
  void initLayout();
  void initOptionsTable();
  void initFloodCorControls();
  void registerSettingsWidgets(Mantid::API::IAlgorithm_sptr alg);
  void registerInstrumentSettingsWidgets(Mantid::API::IAlgorithm_sptr alg);
  void registerExperimentSettingsWidgets(Mantid::API::IAlgorithm_sptr alg);
  void setToolTipAsPropertyDocumentation(QWidget &widget,
                                         std::string const &propertyName,
                                         Mantid::API::IAlgorithm_sptr alg);

  template <typename Widget>
  void registerSettingWidget(Widget &widget, std::string const &propertyName,
                             Mantid::API::IAlgorithm_sptr alg);
  void connectSettingsChange(QLineEdit &edit);
  void connectSettingsChange(QComboBox &edit);
  void connectSettingsChange(QCheckBox &edit);
  void connectSettingsChange(QGroupBox &edit);
  void connectSettingsChange(QTableWidget &edit);
  QLineEdit &stitchOptionsLineEdit() const;
  void setSelected(QComboBox &box, std::string const &str);
  void setText(QLineEdit &lineEdit, int value);
  void setText(QLineEdit &lineEdit, double value);
  void setText(QLineEdit &lineEdit, std::string const &value);
  void setText(QLineEdit &lineEdit, boost::optional<int> value);
  void setText(QLineEdit &lineEdit, boost::optional<double> value);
  void setText(QLineEdit &lineEdit, boost::optional<std::string> const &value);
  void setText(QTableWidget &table, std::string const &propertyName,
               double value);
  void setText(QTableWidget &table, std::string const &propertyName,
               boost::optional<double> value);
  void setText(QTableWidget &table, std::string const &propertyName,
               boost::optional<std::string> value);
  void setText(QTableWidget &table, std::string const &propertyName,
               std::string const &value);
  void setText(QTableWidget &table, std::string const &propertyName,
               const QString &value);
  void setChecked(QCheckBox &checkBox, bool checked);
  std::string getText(QLineEdit const &lineEdit) const;
  std::string getText(QComboBox const &box) const;
  /// Put the per-angle options for a row into a map
  MantidQt::MantidWidgets::DataProcessor::OptionsQMap
  createOptionsMapForRow(const int row) const;

  /// The widget
  Ui::ReflSettingsWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflSettingsPresenter> m_presenter;
  /// Whether or not polarisation corrections should be enabled
  mutable bool m_isPolCorrEnabled;
  /// The stitch params entry widget
  MantidQt::MantidWidgets::HintingLineEdit *m_stitchEdit;
  /// The algorithm properties relating to the options table
  /// @todo Could we use the data processor whitelist to get the properties
  /// instead?
  QStringList m_columnProperties;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_QTREFLSETTINGSVIEW_H_ */
