// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_

#include "IReflEventView.h"
#include "QWidgetGroup.h"
#include "ui_ReflEventWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflEventPresenter;

/** QtReflEventView : Provides an interface for the "Event Handling" widget in
the ISIS Reflectometry interface.
*/
class QtReflEventView : public QWidget, public IReflEventView {
  Q_OBJECT
public:
  /// Constructor
  explicit QtReflEventView(int group, QWidget *parent = nullptr);
  /// Destructor
  ~QtReflEventView() override;
  /// Returns the presenter managing this view
  IReflEventPresenter *getPresenter() const override;
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
  Ui::ReflEventWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflEventPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_ */
