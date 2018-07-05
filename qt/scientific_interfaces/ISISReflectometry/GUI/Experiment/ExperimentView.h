#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENTVIEW_H_

#include "DllConfig.h"
#include "ui_ExperimentWidget.h"
#include "IExperimentView.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
#include <memory>
#include <QCheckBox>
#include <QShortcut>

namespace MantidQt {
namespace CustomInterfaces {

/** ExperiementView : Provides an interface for the "Experiement" tab in the
ISIS Reflectometry interface.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentView
    : public QWidget,
      public IExperimentView {
  Q_OBJECT
public:
  ExperimentView(Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent = nullptr);
  void subscribe(ExperimentViewSubscriber *notifyee) override;

  void
  createStitchHints(const std::vector<MantidWidgets::Hint> &hints) override;

  std::string getAnalysisMode() const override;
  void setAnalysisMode(std::string const& analysisMode) override;

  std::string getSummationType() const override;
  void setSummationType(std::string const& summationType) override;

  std::string getReductionType() const override;
  void setReductionType(std::string const& reductionType) override;

  std::vector<std::array<std::string, 8>> getPerAngleOptions() const override;
  void showPerAngleOptionsAsInvalid(int row, int column) override;
  void showPerAngleOptionsAsValid(int row) override;


  double getTransmissionStartOverlap() const override;
  void setTransmissionStartOverlap(double start) override;
  double getTransmissionEndOverlap() const override;
  void setTransmissionEndOverlap(double end) override;

  std::string getPolarisationCorrectionType() const override;
  void setPolarisationCorrectionType(std::string const& type) override;
  double getCRho() const override;
  void setCRho(double cRho) override;
  double getCAlpha() const override;
  void setCAlpha(double cAlpha) override;
  double getCAp() const override;
  void setCAp(double cAp) override;
  double getCPp() const override;
  void setCPp(double cPp) override;

  std::string getStitchOptions() const override;
  void setStitchOptions(std::string const& stitchOptions) override;

  void showOptionLoadErrors(
      std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
      std::vector<MissingInstrumentParameterValue> const &missingValues) override;

  void showPerAngleThetasNonUnique(double tolerance) override;
  void showAllPerAngleOptionsAsValid() override;

  void disableAll() override;
  void enableAll() override;

  void enableReductionType() override;
  void disableReductionType() override;

  void enablePolarisationCorrections() override;
  void disablePolarisationCorrections() override;

  void addPerThetaDefaultsRow() override;
  void removePerThetaDefaultsRow(int rowIndex) override;
public slots:
  void notifySettingsChanged();
  /// Adds another row to the per-angle options table
  void summationTypeChanged(int reductionTypeIndex);
  void onNewPerThetaDefaultsRowRequested();
  void onRemovePerThetaDefaultsRequested();

private:
  void initializeTableItems(QTableWidget& table);
  void initializeTableRow(QTableWidget& table, int row);
  QString messageFor(
      std::vector<MissingInstrumentParameterValue> const &missingValues) const;
  QString messageFor(const InstrumentParameterTypeMissmatch &typeError) const;

  /// Initialise the interface
  void initLayout();
  void initOptionsTable();
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
  void connectSettingsChange(QTableWidget &edit);
  void connectSettingsChange(QDoubleSpinBox &edit);
  QLineEdit &stitchOptionsLineEdit() const;
  void setSelected(QComboBox &box, std::string const &str);
  void setText(QLineEdit &lineEdit, int value);
  void setText(QLineEdit &lineEdit, double value);
  void setText(QLineEdit &lineEdit, std::string const &value);
  void setText(QLineEdit &lineEdit, boost::optional<int> value);
  void setText(QLineEdit &lineEdit, boost::optional<double> value);
  void setText(QLineEdit &lineEdit, boost::optional<std::string> const &value);
  std::string textFromCell(QTableWidgetItem const* maybeNullItem) const;
//  void setText(QTableWidget &table, std::string const &propertyName,
//               double value);
//  void setText(QTableWidget &table, std::string const &propertyName,
//               boost::optional<double> value);
//  void setText(QTableWidget &table, std::string const &propertyName,
//               boost::optional<std::string> value);
//  void setText(QTableWidget &table, std::string const &propertyName,
//               std::string const &value);
//  void setText(QTableWidget &table, std::string const &propertyName,
//               const QString &value);
  void setChecked(QCheckBox &checkBox, bool checked);
  std::string getText(QLineEdit const &lineEdit) const;
  std::string getText(QComboBox const &box) const;
  /// The stitch params entry widget
  MantidQt::MantidWidgets::HintingLineEdit *m_stitchEdit;

  std::unique_ptr<QShortcut> m_deleteShortcut;
  Ui::ExperimentWidget m_ui;
  ExperimentViewSubscriber *m_notifyee;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_EXPERIMENTVIEW_H_ */
