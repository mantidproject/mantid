//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/WidgetScrollbarFeature.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

using namespace MantidQt::API;

//-----------------------------------------------------------------------------
/**
 * Constructor.
 *
 * @param target The target widget to be extended with scrollbar feature.
 */
WidgetScrollbarFeature::WidgetScrollbarFeature(QWidget *target)
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
}

//-----------------------------------------------------------------------------
/// Destructor
WidgetScrollbarFeature::~WidgetScrollbarFeature() {
  // Must be deleted manually since it has no parent
  delete m_offscreen;
}

//-----------------------------------------------------------------------------
/**
 * Check whether the target is currently scrollable.
 *
 * @return Returns true if the target is currently scrollable, false otherwise.
 */
bool WidgetScrollbarFeature::enabled() const { return m_enabled; }

//-----------------------------------------------------------------------------
/**
 * Enable or disable scrollable behaviour on the target.
 *
 * TODO: Explain how this works
 *
 * @param enable Whether the target should be scrollable or not.
 */
void WidgetScrollbarFeature::setEnabled(bool enable) {
  if (enable && !enabled()) {
    m_viewport->setLayout(m_target->layout());
    m_target->setLayout(m_layout);
  }

  if (!enable && enabled()) {
    m_offscreen->setLayout(m_target->layout());
    m_target->setLayout(m_viewport->layout());
  }

  m_enabled = enable;
}
