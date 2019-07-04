// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_

#include "Common/QWidgetGroup.h"
#include "IEventView.h"
#include "ui_EventWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

/** EventView : Provides an interface for the "Event Handling" widget
in
the ISIS Reflectometry interface.
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
  void onToggleUniform(bool isChecked);
  void onToggleUniformEven(bool isChecked);
  void onToggleCustom(bool isChecked);
  void onToggleLogValue(bool isChecked);
  void onToggleDisabledSlicing(bool isChecked);

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

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_ */
