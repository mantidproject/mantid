#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_

#include "IEventView.h"
#include "ui_EventWidget.h"
#include "QWidgetGroup.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

/** EventView : Provides an interface for the "Event Handling" widget
in
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
class EventView : public QWidget, public IEventView {
  Q_OBJECT
public:
  /// Constructor
  explicit EventView(QWidget *parent = nullptr);
  /// Returns time-slicing values
  void initUniformSliceTypeLayout();
  void initUniformEvenSliceTypeLayout();
  void initLogValueSliceTypeLayout();
  void initCustomSliceTypeLayout();

  void enableSliceType(SliceType sliceType) override;
  void disableSliceType(SliceType sliceType) override;

  void enableSliceTypeSelection() override;
  void disableSliceTypeSelection() override;

  void showCustomBreakpointsInvalid() override;
  void showCustomBreakpointsValid() override;

  void showLogBreakpointsInvalid() override;
  void showLogBreakpointsValid() override;

  std::string logBlockName() const override;
  std::string logBreakpoints() const override;
  std::string customBreakpoints() const override;
  int uniformSliceCount() const override;
  double uniformSliceLength() const override;

  void subscribe(EventViewSubscriber *notifyee) override;

public slots:
  void toggleUniform(bool isChecked);
  void toggleUniformEven(bool isChecked);
  void toggleCustom(bool isChecked);
  void toggleLogValue(bool isChecked);
  void toggleDisabledSlicing(bool isChecked);

  void onUniformEvenChanged(int);
  void onUniformSecondsChanged(double);
  void onCustomChanged(QString const &);
  void onLogValueTypeChanged(QString const &);
  void onLogValuesChanged(QString const &);

private:
  /// Initialise the interface
  void initLayout();
  std::string textFrom(QLineEdit const *const widget) const;

  QWidgetGroup<2> m_uniformGroup;
  QWidgetGroup<2> m_uniformEvenGroup;
  QWidgetGroup<4> m_logValueGroup;
  QWidgetGroup<2> m_customGroup;
  QWidgetGroup<5> m_sliceTypeRadioButtons;

  /// The widget
  Ui::EventWidget m_ui;
  EventViewSubscriber *m_notifyee;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_ */
