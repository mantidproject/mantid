// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TIMECONTROLWIDGET_H_
#define TIMECONTROLWIDGET_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

// The Windows SDK defines an ERROR macro in um\wingdi.h that conflicts
// with the definition of a enum value in pqOutputWidget.h
#if defined(_WIN32) && defined(ERROR)
#undef ERROR
#endif
#include "ui_TimeControlWidget.h"

namespace Mantid {
namespace Vates {
namespace SimpleGui {

/**
 *
 This class wraps the ParaView time control toolbars into a widget.

 @author Michael Reuter
 @date 24/05/2011
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS TimeControlWidget
    : public QWidget {
  Q_OBJECT

public:
  /// Default constructor.
  TimeControlWidget(QWidget *parent = nullptr);
  /// Default destructor.
  ~TimeControlWidget() override;

public slots:
  /// Enable/disable the animation controls.
  void enableAnimationControls(bool state);
  /// Update information in animation controls.
  void updateAnimationControls(double timeStart, double timeEnd,
                               int numTimesteps);

private:
  Ui::TimeControlWidgetClass ui; ///< The time control widget's UI form
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // TIMECONTROLWIDGET_H_
