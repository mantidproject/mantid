#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENTVIEW_H_

#include "DllConfig.h"
#include "ui_InstrumentWidget.h"
#include "IInstrumentView.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
#include <memory>
#include <QCheckBox>
#include <QShortcut>

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
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentView
    : public QWidget,
      public IInstrumentView {
  Q_OBJECT
public:
  InstrumentView(Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent = nullptr);
  void subscribe(InstrumentViewSubscriber *notifyee) override;

  void disableAll() override;
  void enableAll() override;

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
  void connectSettingsChange(QTableWidget &edit);
  void connectSettingsChange(QDoubleSpinBox &edit);
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

  Ui::InstrumentWidget m_ui;
  InstrumentViewSubscriber *m_notifyee;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_INSTRUMENTVIEW_H_ */
