#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_

#include "IReflEventTabView.h"
#include "ui_ReflEventTabWidget.h"
#include "QWidgetGroup.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

/** QtReflEventTabView : Provides an interface for the "Event Handling" widget in
the ISIS Reflectometry interface.

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
class QtReflEventTabView : public QWidget, public IReflEventTabView {
  Q_OBJECT
public:
  /// Constructor
  explicit QtReflEventTabView(QWidget *parent = nullptr);
  /// Returns time-slicing values
  void initUniformSliceTypeLayout();
  void initUniformEvenSliceTypeLayout();
  void initLogValueSliceTypeLayout();
  void initCustomSliceTypeLayout();

  void enableSliceType(SliceType sliceType) override;
  void disableSliceType(SliceType sliceType) override;

  void enableSliceTypeSelection() override;
  void disableSliceTypeSelection() override;

  std::string getLogValueTimeSlicingType() const override;
  std::string getLogValueTimeSlicingValues() const override;
  std::string getCustomTimeSlicingValues() const override;
  std::string getUniformTimeSlicingValues() const override;
  std::string getUniformEvenTimeSlicingValues() const override;

  void subscribe(EventTabViewSubscriber* notifyee) override;

public slots:
  void toggleUniform(bool isChecked);
  void toggleUniformEven(bool isChecked);
  void toggleCustom(bool isChecked);
  void toggleLogValue(bool isChecked);
  void notifySettingsChanged();

private:
  /// Initialise the interface
  void initLayout();
  std::string textFrom(QLineEdit const *const widget) const;
  void registerEventWidgets();
  void connectSettingsChange(QLineEdit &edit);
  void connectSettingsChange(QGroupBox &edit);

  QWidgetGroup<2> m_uniformGroup;
  QWidgetGroup<2> m_uniformEvenGroup;
  QWidgetGroup<4> m_logValueGroup;
  QWidgetGroup<2> m_customGroup;
  QWidgetGroup<4> m_sliceTypeRadioButtons;

  /// The widget
  Ui::ReflEventTabWidget m_ui;
  EventTabViewSubscriber* m_notifyee;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_ */
