// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_WIDGETSCROLLBARDECORATOR_H_
#define MANTIDQT_API_WIDGETSCROLLBARDECORATOR_H_

//----------------------------------
// Includes
//----------------------------------
#include "MantidKernel/System.h"

//----------------------------------
// Forward declarations
//----------------------------------
class QScrollArea;
class QVBoxLayout;
class QWidget;

namespace MantidQt {
namespace API {

/**
 * WidgetScrollbarDecorator : Adds scrollbar functionality to a QWidget.
 *
 * Mainly intended to make @c QDialogs and @c QMainWindows scrollable since
 * this feature is missing in Qt. This allows particularly large dialogs to be
 * used on laptops and other devices with small screens.
 *
 * Both horizontal and vertical scrollbars are provided. Scrollbars are only
 * visible when needed (when the dialog/widget is too small to display all
 * widgets at their minimum size). Qt automatically attempts to resize dialogs
 * to fit on the screen that they show up on.
 *
 * Usage:
 *
 * 1. Create an instance of this class (usually as a class member)
 *
 * @code
 * private:
 *   WidgetScrollbarDecorator m_scrollbars;
 * @endcode
 *
 * 2. Initialise with the widget that needs scrollbars (usually @c this )
 *
 * @code
 * ExampleDialog::ExampleDialog() : m_scrollbars(this) {}
 * @endcode
 *
 * 3. Enable scrollbars (only after the UI has been set up!)
 *
 * @code
 *   ui->setupUi(this); // MUST happen first
 *   m_scrollbars.setEnabled(true);
 * @endcode
 *
 * Details:
 *
 * The reason scrollbars should only be enabled after the UI is set up is that
 * this decorator works by pulling the main layout of the target widget out
 * (along with all widgets inside of it), adding its own layout and scrollarea
 * widgets to the target, and finally placing the original layout inside of the
 * scrollarea.
 *
 * For Designer-created dialogs, this means that setupUi(this) must be called
 * before scrollbars are enabled. Widgets can still be accessed as usual via
 * the @c ui object. This includes adding, removing, moving, etc widgets, as
 * long as it is done using @c ui->mainLayout rather than @c this->layout().
 *
 * For manually created dialogs, this means that you have to at least create a
 * main layout for your dialog before enabling the scrollbars. Note that after
 * enabling scrollbars, @c this->layout() will no longer be the layout that
 * you created. So, if you want to access your layout or add more widgets
 * later, you should keep a pointer to it and work with that.
 *
 *
 *
 */
class DLLExport WidgetScrollbarDecorator {
public:
  explicit WidgetScrollbarDecorator(QWidget *target);
  virtual ~WidgetScrollbarDecorator();

  /// Check whether the target is currently scrollable
  bool enabled() const;

  /// Enable or disable scrollable behaviour
  void setEnabled(bool enable);

  /// Set the width, in pixels, at which scrollbars should appear
  void setThresholdWidth(int width);

  /// Set the height, in pixels, at which scrollbars should appear
  void setThresholdHeight(int height);

  /// Set the size, in pixels, at which scrollbars should appear
  void setThresholdSize(int width, int height);

private:
  QVBoxLayout *m_layout;     ///< Main layout used when scrollable
  QScrollArea *m_scrollarea; ///< Used to provide scrolling functionality
  QWidget *m_viewport;       ///< Single widget inside QScrollArea
  QWidget *m_offscreen;      ///< Used to hold above widgets when disabled

  QWidget *m_target; ///< The target widget that is given scrollbars
  bool m_enabled;    ///< Whether the target is currently scrollable
};

} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_WIDGETSCROLLBARDECORATOR_H_
