#ifndef MANTIDQT_API_WIDGETSCROLLBARFEATURE_H_
#define MANTIDQT_API_WIDGETSCROLLBARFEATURE_H_

//----------------------------------
// Forward declarations
//----------------------------------
class QScrollArea;
class QVBoxLayout;
class QWidget;

namespace MantidQt {
namespace API {

/**
 * Adds scrollbar functionality to a QWidget.
 *
 * TODO: Add details, usage, etc
 */
class WidgetScrollbarFeature {
public:
  explicit WidgetScrollbarFeature(QWidget *target);
  virtual ~WidgetScrollbarFeature();

  /// Check whether the target is currently scrollable
  bool enabled() const;

  /// Enable or disable scrollable behaviour
  void setEnabled(bool enable);

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

#endif // MANTIDQT_API_WIDGETSCROLLBARFEATURE_H_
