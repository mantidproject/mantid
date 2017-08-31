#include "MantidQtWidgets/MplCpp/MplEvent.h"

/**
 * Constructor
 * @param pos The point relative to generating widget's
 * @param dataPos The point defined in data coordinates. Maybe null to indicate
 * that the event was not within the Axis limits
 * @param button The button that was pressed
 */
MplMouseEvent::MplMouseEvent(QPointF dataPos) : m_dataPos(dataPos) {}
