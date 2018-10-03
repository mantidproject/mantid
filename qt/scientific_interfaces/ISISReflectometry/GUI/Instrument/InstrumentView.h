#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENTVIEW_H_

#include "DllConfig.h"
#include "IInstrumentView.h"
#include "ui_InstrumentWidget.h"
#include <QCheckBox>
#include <QLineEdit>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

/** InstrumentView : Provides an interface for the "Instrument" tab in the
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
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentView : public QWidget,
                                                      public IInstrumentView {
  Q_OBJECT
public:
  InstrumentView(Mantid::API::IAlgorithm_sptr algorithmForTooltips,
                 QWidget *parent = nullptr);
  void subscribe(InstrumentViewSubscriber *notifyee) override;

  int getMonitorIndex() const override;
  bool getIntegrateMonitors() const override;

  double getLambdaMin() const override;
  double getLambdaMax() const override;
  void showLambdaRangeInvalid() override;
  void showLambdaRangeValid() override;

  double getMonitorBackgroundMin() const override;
  double getMonitorBackgroundMax() const override;
  void showMonitorBackgroundRangeInvalid() override;
  void showMonitorBackgroundRangeValid() override;

  double getMonitorIntegralMin() const override;
  double getMonitorIntegralMax() const override;
  void showMonitorIntegralRangeInvalid() override;
  void showMonitorIntegralRangeValid() override;

  bool getCorrectDetectors() const override;
  std::string getDetectorCorrectionType() const override;

  void disableAll() override;
  void enableAll() override;
  void enableDetectorCorrectionType() override;
  void disableDetectorCorrectionType() override;

public slots:
  void onSettingsChanged();

private:
  QString messageFor(
      std::vector<MissingInstrumentParameterValue> const &missingValues) const;
  QString messageFor(const InstrumentParameterTypeMissmatch &typeError) const;

  /// Initialise the interface
  void initLayout();
  void registerSettingsWidgets(Mantid::API::IAlgorithm_sptr alg);
  void registerInstrumentSettingsWidgets(Mantid::API::IAlgorithm_sptr alg);
  void setToolTipAsPropertyDocumentation(QWidget &widget,
                                         std::string const &propertyName,
                                         Mantid::API::IAlgorithm_sptr alg);

  template <typename Widget>
  void registerSettingWidget(Widget &widget, std::string const &propertyName,
                             Mantid::API::IAlgorithm_sptr alg);
  void connectSettingsChange(QLineEdit &edit);
  void connectSettingsChange(QComboBox &edit);
  void connectSettingsChange(QCheckBox &edit);
  void connectSettingsChange(QSpinBox &edit);
  void connectSettingsChange(QDoubleSpinBox &edit);
  void setSelected(QComboBox &box, std::string const &str);
  void setText(QLineEdit &lineEdit, int value);
  void setText(QLineEdit &lineEdit, double value);
  void setText(QLineEdit &lineEdit, std::string const &value);
  void setText(QLineEdit &lineEdit, boost::optional<int> value);
  void setText(QLineEdit &lineEdit, boost::optional<double> value);
  void setText(QLineEdit &lineEdit, boost::optional<std::string> const &value);
  void setChecked(QCheckBox &checkBox, bool checked);
  std::string getText(QLineEdit const &lineEdit) const;
  std::string getText(QComboBox const &box) const;

  Ui::InstrumentWidget m_ui;
  InstrumentViewSubscriber *m_notifyee;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INSTRUMENTVIEW_H_ */
