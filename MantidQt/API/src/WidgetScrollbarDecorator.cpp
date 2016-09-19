//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/WidgetScrollbarDecorator.h"

#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

using namespace MantidQt::API;

//-----------------------------------------------------------------------------
/**
 * Constructor.
 *
 * @param target The target widget to be extended with scrollbar functionality.
 */
WidgetScrollbarDecorator::WidgetScrollbarDecorator(QWidget *target)
    : m_target(target), m_enabled(false) {
  // Off-screen widget to hold layout/widgets when scrolling disabled
  m_offscreen = new QWidget(0);

  // This layout replaces dialog's main layout when scrolling enabled
  m_layout = new QVBoxLayout(m_offscreen);
  m_layout->setSpacing(0);
  m_layout->setMargin(0);

  // QScrollArea provides scrolling functionality
  m_scrollarea = new QScrollArea(m_offscreen);
  m_scrollarea->setFrameStyle(QFrame::NoFrame);
  m_layout->addWidget(m_scrollarea);

  // Viewport represents the inside of QScrollArea
  // It will take over parentship of the layout and widgets of the target
  m_viewport = new QWidget(m_scrollarea);
  m_scrollarea->setWidget(m_viewport);
  m_scrollarea->setWidgetResizable(true);

  // With QMainWindows we must work on the centralWidget instead
  auto mainwindow = dynamic_cast<QMainWindow *>(m_target);
  if (mainwindow)
    m_target = mainwindow->centralWidget();
}

//-----------------------------------------------------------------------------
/// Destructor
WidgetScrollbarDecorator::~WidgetScrollbarDecorator() {
  // Must be deleted manually since it has no parent
  delete m_offscreen;
}

//-----------------------------------------------------------------------------
/**
 * Check whether the target is currently scrollable.
 *
 * @return true if the target is currently scrollable, false otherwise.
 */
bool WidgetScrollbarDecorator::enabled() const { return m_enabled; }

//-----------------------------------------------------------------------------
/**
 * Enable or disable scrollable behaviour on the target.
 *
 * This works by shuffling layouts using a sparsely documented feature of
 * QWidget::setLayout(). Normally, you cannot remove a layout once it is set
 * without deleting it (and all contained widgets along with it). You also
 * cannot call setLayout() on a widget that already has a layout.
 *
 * However, if the layout you pass to setLayout() is already set on a different
 * widget, that layout and all contained widgets are reparented, effectively
 * removing it from the widget it was on. But, for this to work, you need a
 * layout-less widget to call setLayout() on.
 *
 * Since this class works with three widgets (the target widget, the viewport
 * inside of the scrollable area, and an offscreen dummy widget) and only two
 * layouts (the layout of the target widget and the layout that contains the
 * scrollarea), there is always one widget that has no layout.
 *
 * When scrolling is enabled, m_offscreen is empty.
 * When scrolling is disabled, m_viewport is empty.
 *
 * @param enable Whether the target should be scrollable or not.
 */
void WidgetScrollbarDecorator::setEnabled(bool enable) {
  // Only enable if scrollbars were previously disabled
  if (enable && !enabled()) {
    m_viewport->setLayout(m_target->layout());
    m_target->setLayout(m_layout);
  }

  // Only disable if scrollbars were previously enabled
  if (!enable && enabled()) {
    m_offscreen->setLayout(m_target->layout());
    m_target->setLayout(m_viewport->layout());
  }

  m_enabled = enable;
}

//-----------------------------------------------------------------------------
/**
 * Set the width, in pixels, at which scrollbars should appear.
 *
 * This overrides the default behaviour of preferring to shrink widgets until
 * they reach their minimum size before enabling scrollbars. Note that
 * scrollbars will be enabled before reaching this size setting if the
 * minimum size of all widgets is reached first.
 *
 * Set to 0 to reset to default behaviour.
 *
 * @param width Minimum width target may shrink to before scrollbars appear
 */
void WidgetScrollbarDecorator::setThresholdWidth(int width) {
  m_viewport->setMinimumWidth(width);
}

//-----------------------------------------------------------------------------
/**
 * Set the height, in pixels, at which scrollbars should appear.
 *
 * This overrides the default behaviour of preferring to shrink widgets until
 * they reach their minimum size before enabling scrollbars. Note that
 * scrollbars will be enabled before reaching this size setting if the
 * minimum size of all widgets is reached first.
 *
 * Set to 0 to reset to default behaviour.
 *
 * @param height Minimum height target may shrink to before scrollbars appear
 */
void WidgetScrollbarDecorator::setThresholdHeight(int height) {
  m_viewport->setMinimumHeight(height);
}

//-----------------------------------------------------------------------------
/**
 * Set the size, in pixels, at which scrollbars should appear.
 *
 * This overrides the default behaviour of preferring to shrink widgets until
 * they reach their minimum size before enabling scrollbars. Note that
 * scrollbars will be enabled before reaching this size setting if the
 * minimum size of all widgets is reached first.
 *
 * Set to (0, 0) to reset to default behaviour.
 *
 * @param width Minimum width target may shrink to before scrollbars appear
 * @param height Minimum height target may shrink to before scrollbars appear
 */
void WidgetScrollbarDecorator::setThresholdSize(int width, int height) {
  m_viewport->setMinimumSize(width, height);
}
