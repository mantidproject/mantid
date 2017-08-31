#include "MantidQtWidgets/MplCpp/MplEvent.h"
#include <QMouseEvent>

/**
 * Constructor
 * @param pos The point relative to generating widget's
 * @param dataPos The point defined in data coordinates. Maybe null to indicate
 * that the event was not within the Axis limits
 * @param button The button that was pressed
 */
MantidQt::Widgets::MplCpp::MplMouseEvent::MplMouseEvent(QPoint pos,
                                                        QPointF dataPos,
                                                        Qt::MouseButton button)
    : m_pos(pos), m_dataPos(dataPos), m_button(button) {}
