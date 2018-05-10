#include "MantidQtWidgets/SliceViewer/LineOverlay.h"
#include "MantidKernel/Utils.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include <QPainter>
#include <QRect>
#include <QShowEvent>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace SliceViewer {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LineOverlay::LineOverlay(QwtPlot *plot, QWidget *parent)
    : QWidget(parent), m_plot(plot), m_snapEnabled(false), m_snapX(0.1),
      m_snapY(0.1), m_snapLength(0), m_shown(true), m_showHandles(true),
      m_showLine(true), m_angleSnapMode(false), m_angleSnap(45) {
  m_creation = true; // Will create with the mouse
  m_rightButton = false;
  m_dragHandle = HandleNone;

  m_pointA = QPointF(0.0, 0.0);
  m_pointB = QPointF(1.0, 1.0);
  m_width = 0.1;
  // setAttribute(Qt::WA_TransparentForMouseEvents);
  // We need mouse events all the time
  setMouseTracking(true);
  // setAttribute(Qt::WA_TransparentForMouseEvents);
  // Make sure mouse propagates
  setAttribute(Qt::WA_NoMousePropagation, false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LineOverlay::~LineOverlay() {}

//----------------------------------------------------------------------------------------------
/** Reset the line. User will have to click to create it */
void LineOverlay::reset() {
  m_creation = true; // Will create with the mouse
  m_rightButton = false;
  m_dragHandle = HandleNone;
  this->update();
}

//----------------------------------------------------------------------------------------------
/** Set point A's position
 * @param pointA :: plot coordinates */
void LineOverlay::setPointA(QPointF pointA) {
  m_pointA = pointA;
  this->update(); // repaint
  emit lineChanging(m_pointA, m_pointB, m_width);
}

/** Set point B's position
 * @param pointB :: plot coordinates */
void LineOverlay::setPointB(QPointF pointB) {
  m_pointB = pointB;
  this->update(); // repaint
  emit lineChanging(m_pointA, m_pointB, m_width);
}

/** Set the width of integration
 * @param width :: in plot coordinates */
void LineOverlay::setWidth(double width) {
  m_width = width;
  this->update(); // repaint
  emit lineChanging(m_pointA, m_pointB, m_width);
}

//----------------------------------------------------------------------------------------------
/** Set the snap-to-grid spacing in the X direction.
 * @param spacing :: spacing */
void LineOverlay::setSnapX(double spacing) { m_snapX = spacing; }

/** Set the snap-to-grid spacing in the Y direction.
 * @param spacing :: spacing */
void LineOverlay::setSnapY(double spacing) { m_snapY = spacing; }

/** Set the snap-to-grid spacing in both directions.
 * @param spacing :: spacing */
void LineOverlay::setSnap(double spacing) {
  m_snapX = spacing;
  m_snapY = spacing;
}

/** Set the snap-to-grid spacing in the Y direction.
 * @param enabled :: enable spacing */
void LineOverlay::setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }

/** Set the snap-to-line-length spacing
 * @param spacing :: spacing */
void LineOverlay::setSnapLength(double spacing) { m_snapLength = spacing; }

/** Sets whether any of the control is visible
 * @param shown :: if false, the the control is not drawng */
void LineOverlay::setShown(bool shown) { m_shown = shown; }

/** Sets whether to show the mouse handles
 * @param shown :: if false, the mouse handles are invisible */
void LineOverlay::setShowHandles(bool shown) { m_showHandles = shown; }

/** Sets whether to show the central line
 * @param shown :: if false, the central line is invisible */
void LineOverlay::setShowLine(bool shown) { m_showLine = shown; }

/** Sets whether we are in "creation" mode
 * For use by python directly specifying the position
 *
 * @param creation :: True for creation mode.
 */
void LineOverlay::setCreationMode(bool creation) {
  m_creation = creation;
  this->update();
}

/** Gets whether any of the control is visible
 * @return whether the overlay is shown */
bool LineOverlay::isShown() const { return m_shown; }

void LineOverlay::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);

  QPointF a, b;
  double width;
  bool shown;

  tsv.selectLine("PointA");
  tsv >> a;
  tsv.selectLine("PointB");
  tsv >> b;
  tsv.selectLine("Width");
  tsv >> width;
  tsv.selectLine("Shown");
  tsv >> shown;

  setPointA(a);
  setPointB(b);
  setWidth(width);
  setShown(shown);

  setCreationMode(false);
}

std::string LineOverlay::saveToProject() const {
  API::TSVSerialiser tsv;

  tsv.writeLine("PointA") << getPointA();
  tsv.writeLine("PointB") << getPointB();
  tsv.writeLine("Width") << getWidth();
  tsv.writeLine("Shown") << isShown();

  return tsv.outputLines();
}

//----------------------------------------------------------------------------------------------
/** Turn angle snap on or off
 * @param angleSnap :: true for always angle snap. */
void LineOverlay::setAngleSnapMode(bool angleSnap) {
  m_angleSnapMode = angleSnap;
}

/** Sets the angle increments to snap to.
 * @param snapDegrees :: snap amount, in degrees */
void LineOverlay::setAngleSnap(double snapDegrees) {
  m_angleSnap = snapDegrees;
}

//----------------------------------------------------------------------------------------------
/// @return point A's position in plot coordinates
const QPointF &LineOverlay::getPointA() const { return m_pointA; }

/// @return point B's position in plot coordinates
const QPointF &LineOverlay::getPointB() const { return m_pointB; }

/// @return width of the line in plot coordinates
double LineOverlay::getWidth() const { return m_width; }

//----------------------------------------------------------------------------------------------
/// Return the recommended size of the widget
QSize LineOverlay::sizeHint() const {
  // TODO: Is there a smarter way to find the right size?
  return QSize(20000, 20000);
  // Always as big as the canvas
  // return m_plot->canvas()->size();
}

QSize LineOverlay::size() const { return m_plot->canvas()->size(); }
int LineOverlay::height() const { return m_plot->canvas()->height(); }
int LineOverlay::width() const { return m_plot->canvas()->width(); }

//----------------------------------------------------------------------------------------------
/** Tranform from plot coordinates to pixel coordinates
 * @param coords :: coordinate point in plot coordinates
 * @return pixel coordinates */
QPoint LineOverlay::transform(QPointF coords) const {
  int xA = m_plot->transform(QwtPlot::xBottom, coords.x());
  int yA = m_plot->transform(QwtPlot::yLeft, coords.y());
  return QPoint(xA, yA);
}

//----------------------------------------------------------------------------------------------
/** Inverse transform: from pixels to plot coords
 * @param pixels :: location in pixels
 * @return plot coordinates (float)   */
QPointF LineOverlay::invTransform(QPoint pixels) const {
  double xA = m_plot->invTransform(QwtPlot::xBottom, pixels.x());
  double yA = m_plot->invTransform(QwtPlot::yLeft, pixels.y());
  return QPointF(xA, yA);
}

//----------------------------------------------------------------------------------------------
/** Snap a point to the grid (if enabled)
 * @param original :: original point
 * @return snapped to grid in either or both dimensions  */
QPointF LineOverlay::snap(QPointF original) const {
  if (!m_snapEnabled)
    return original;
  else {
    QPointF out = original;
    // Snap to grid
    if (m_snapX > 0)
      out.setX(Utils::rounddbl(out.x() / m_snapX) * m_snapX);
    if (m_snapY > 0)
      out.setY(Utils::rounddbl(out.y() / m_snapY) * m_snapY);
    return out;
  }
}

//----------------------------------------------------------------------------------------------
/** Draw a handle (for dragging) at the given plot coordinates */
QRect LineOverlay::drawHandle(QPainter &painter, QPointF coords, QColor brush) {
  int size = 8;
  QPoint center = transform(coords);
  QRect marker(center.x() - size / 2, center.y() - size / 2, size, size);
  if (this->m_showHandles) {
    painter.setPen(QColor(255, 0, 0));
    painter.setBrush(brush);
    painter.drawRect(marker);
  }
  return marker;
}

//----------------------------------------------------------------------------------------------
/// Paint the overlay
void LineOverlay::paintEvent(QPaintEvent * /*event*/) {
  // Don't paint until created
  // Also, don't paint while right-click dragging (panning) the underlying pic
  if (m_creation || m_rightButton || !m_shown)
    return;

  QPainter painter(this);
  //    int r = rand() % 255;
  //    int g = rand() % 255;
  //    int b = rand() % 255;

  //    painter.setBrush(QBrush(QColor(r,g,b)));

  QPointF diff = m_pointB - m_pointA;
  // Angle of the "width" perpendicular to the line
  double angle = atan2(diff.y(), diff.x()) + M_PI / 2.0;
  QPointF widthOffset(m_width * cos(angle), m_width * sin(angle));

  // Rectangle with a rotation
  QPointF pA1 = m_pointA + widthOffset;
  QPointF pA2 = m_pointA - widthOffset;
  QPointF pB1 = m_pointB + widthOffset;
  QPointF pB2 = m_pointB - widthOffset;

  QPen boxPenLight(QColor(255, 255, 255, 200));
  QPen boxPenDark(QColor(0, 0, 0, 200));
  QPen centerPen(QColor(192, 192, 192, 128));

  // Special XOR pixel drawing
  // painter.setCompositionMode( QPainter::RasterOp_SourceXorDestination ); //
  // RHEL5 has an old version of QT?

  boxPenLight.setDashPattern(QVector<qreal>() << 5 << 5);
  boxPenDark.setDashPattern(QVector<qreal>() << 0 << 5 << 5 << 0);

  // --- Draw the box ---
  boxPenLight.setWidthF(1.0);
  boxPenDark.setWidthF(1.0);

  //    QPoint points[4] = {transform(pA1), transform(pB1), transform(pB2),
  //    transform(pA2)};
  //    painter.drawPolygon(points, 4);

  painter.setPen(boxPenLight);
  painter.drawLine(transform(pA1), transform(pB1));
  painter.drawLine(transform(pB1), transform(pB2));
  painter.drawLine(transform(pA2), transform(pB2));
  painter.drawLine(transform(pA2), transform(pA1));
  painter.setPen(boxPenDark);
  painter.drawLine(transform(pA1), transform(pB1));
  painter.drawLine(transform(pB1), transform(pB2));
  painter.drawLine(transform(pA2), transform(pB2));
  painter.drawLine(transform(pA2), transform(pA1));

  // Go back to normal drawing mode
  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  // --- Draw the central line ---
  if (m_showLine) {
    centerPen.setWidth(2);
    centerPen.setCapStyle(Qt::FlatCap);
    painter.setPen(centerPen);
    painter.drawLine(transform(m_pointA), transform(m_pointB));
  }

  // --- Draw and store the rects of the 4 handles ---
  m_handles.clear();
  m_handles.push_back(drawHandle(painter, m_pointA, QColor(0, 0, 0)));
  m_handles.push_back(drawHandle(painter, m_pointB, QColor(255, 255, 255)));
  m_handles.push_back(drawHandle(
      painter, (m_pointA + m_pointB) / 2 + widthOffset, QColor(0, 255, 255)));
  m_handles.push_back(drawHandle(
      painter, (m_pointA + m_pointB) / 2 - widthOffset, QColor(0, 255, 255)));
}

//==============================================================================================
//================================= MOUSE HANDLING
//=============================================
//==============================================================================================

//-----------------------------------------------------------------------------------------------
/** Return the handle ID over which the mouse is
 * @param pos :: position in pixels of mouse */
LineOverlay::eHandleID LineOverlay::mouseOverHandle(QPoint pos) {
  for (int i = 0; i < m_handles.size(); i++) {
    if (m_handles[i].contains(pos)) {
      return eHandleID(i);
    }
  }
  if (this->mouseOverCenter(pos))
    return HandleCenter;
  else
    return HandleNone;
}

///@return true if value is between A and B. A can be < or > then B.
bool isBetween(double value, double A, double B) {
  if (B > A)
    return (value >= A && value <= B);
  else if (B < A)
    return (value >= B && value <= A);
  else
    return (value == A);
}

/** Return true if the mouse is over the central line
 * @param pos :: position in pixels of mouse */
bool LineOverlay::mouseOverCenter(QPoint pos) {
  // Mouse position in pixels
  QPointF current = pos;
  // Find the distance (in pixels) between the mouse and the center of the line
  QPointF pixA = transform(m_pointA);
  QPointF pixB = transform(m_pointB);
  QPointF diff = pixB - pixA;
  double distance = fabs(diff.x() * (current.y() - pixA.y()) -
                         (current.x() - pixA.x()) * diff.y()) /
                    sqrt(diff.x() * diff.x() + diff.y() * diff.y());

  // Margin of 6 pixels, and must be between the 2 limits (if the limits are not
  // the same)
  bool retval = false;
  if (distance < 7) {
    retval = true;
    if ((pixA.x() != pixB.x()) &&
        (!isBetween(current.x(), pixA.x(), pixB.x()))) {
      retval = false;
    }
    if ((pixA.y() != pixB.y()) &&
        (!isBetween(current.y(), pixA.y(), pixB.y()))) {
      retval = false;
    }
  }
  return retval;
}

//-----------------------------------------------------------------------------------------------
/** Handle the mouse move event when the line is being dragged
 * @param event mouse event info */
void LineOverlay::handleDrag(QMouseEvent *event) {
  // Is the shift key pressed?
  bool shiftPressed = (event->modifiers() & Qt::ShiftModifier);
  // Currently dragging!
  QPointF current = this->invTransform(event->pos());
  QPointF currentSnap = this->snap(current);
  QPointF diff = m_pointB - m_pointA;
  QPointF dragAmount = current - this->m_dragStart;
  dragAmount = snap(dragAmount);
  double width = 0;

  // Adjust the current mouse position if needed.
  if ((m_snapLength > 0) || shiftPressed || m_angleSnapMode) {
    // This is the distance between the fixed and dragged point
    QPointF currentDiff;
    if (m_dragHandle == HandleA)
      currentDiff = current - m_pointB;
    else if (m_dragHandle == HandleB)
      currentDiff = current - m_pointA;

    // calculate angle
    double angle = atan2(currentDiff.y(), currentDiff.x());

    // Round angle to closest 45 degrees, if in angle snap mode (shift pressed)
    if (shiftPressed || m_angleSnapMode) {
      if (m_angleSnap == 90.0) {
        // special case for 90 snap (axis aligned snapping)
        // use screen coords to determine snap angle
        QPointF currentScreenDiff;
        if (m_dragHandle == HandleA)
          currentScreenDiff = event->pos() - transform(m_pointB);
        else if (m_dragHandle == HandleB)
          currentScreenDiff = event->pos() - transform(m_pointA);

        // Limit angles to 90 based on screen coords, not data coords
        //-y is because the screen y coords are inverted to the data coords
        angle = atan2(-currentScreenDiff.y(), currentScreenDiff.x());
      }
      // convert snap angle to radians
      double angleSnapRad = m_angleSnap / (180.0 / M_PI);
      // round current angle to snap angle
      angle = Utils::rounddbl(angle / angleSnapRad) * angleSnapRad;
    }

    double length;
    // for axis aligned angles just use respective distance for the length
    if (fmod(angle, M_PI) == 0) {
      length = fabs(currentDiff.x());
    } else if (fmod(angle, M_PI) == M_PI / 2) {
      length = fabs(currentDiff.y());
    } else {
      length = sqrt(currentDiff.x() * currentDiff.x() +
                    currentDiff.y() * currentDiff.y());
    }

    // Round length to m_snapLength, if specified
    if (m_snapLength > 0)
      length = Utils::rounddbl(length / m_snapLength) * m_snapLength;

    // Rebuild the mouse position
    currentDiff = QPointF(cos(angle) * length, sin(angle) * length);
    if (m_dragHandle == HandleA)
      currentSnap = snap(m_pointB + currentDiff);
    else if (m_dragHandle == HandleB)
      currentSnap = snap(m_pointA + currentDiff);
  }

  switch (m_dragHandle) {
  case HandleA:
    setPointA(currentSnap);
    break;

  case HandleB:
    setPointB(currentSnap);
    break;

  case HandleWidthBottom:
  case HandleWidthTop:
    // Find the distance between the mouse and the line (see
    // http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html )
    width = fabs(diff.x() * (current.y() - m_pointA.y()) -
                 (current.x() - m_pointA.x()) * diff.y()) /
            sqrt(diff.x() * diff.x() + diff.y() * diff.y());
    setWidth(width);
    break;

  case HandleCenter:
    // Move the whole line around
    m_pointA = m_dragStart_PointA + dragAmount;
    m_pointB = m_dragStart_PointB + dragAmount;
    this->update();
    emit lineChanging(m_pointA, m_pointB, m_width);
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------------------------------------------
/** Event when the mouse moves
 * @param event mouse event info */
void LineOverlay::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::RightButton)
    m_rightButton = true;

  // Do not respond to mouse when hidden
  if (!m_showHandles || !m_shown) {
    event->ignore();
    return;
  }

  // --- Initial creation mode - wait for first click ----
  if (m_creation) {
    // TODO: Custom mouse cursor?
    this->setCursor(Qt::PointingHandCursor);
    // Pass-through event to underlying widget, so that it shows the mouse
    // position
    event->ignore();
    return;
  }

  // --- Initial creation mode ----
  if (m_dragHandle != HandleNone) {
    this->handleDrag(event);
  } else {
    // ---- Just moving the mouse -------------
    if (event->buttons() == Qt::NoButton) {
      LineOverlay::eHandleID hdl = mouseOverHandle(event->pos());
      switch (hdl) {
      case HandleA:
      case HandleB:
        this->setCursor(Qt::SizeHorCursor);
        break;
      case HandleWidthBottom:
      case HandleWidthTop:
        this->setCursor(Qt::SizeVerCursor);
        break;
      case HandleCenter:
        this->setCursor(Qt::PointingHandCursor);
        break;

      default:
        this->setCursor(Qt::CrossCursor);
        break;
      }
    }
  }
  // In all cases, we pass-through the event to underlying widget,
  // so that it shows the mouse position.
  event->ignore();
}

//-----------------------------------------------------------------------------------------------
/** Event when the mouse button is pressed down
 * @param event mouse event info */
void LineOverlay::mousePressEvent(QMouseEvent *event) {
  // Do not respond to mouse when hidden
  if (!m_showHandles || !m_shown) {
    event->ignore();
    return;
  }

  // First left-click = create!
  if (m_creation && (event->buttons() & Qt::LeftButton)) {
    QPointF pt = snap(this->invTransform(event->pos()));
    m_pointB = pt;
    setPointA(pt);
    // And now we are in drag B mode
    m_creation = false;
    m_dragStart = pt;
    m_dragHandle = HandleB;
    return;
  }

  LineOverlay::eHandleID hdl = mouseOverHandle(event->pos());
  // Drag with the left mouse button
  if (hdl != HandleNone && (event->buttons() & Qt::LeftButton)) {
    // Start dragging
    m_dragHandle = hdl;
    m_dragStart = this->invTransform(event->pos());
    m_dragStart_PointA = m_pointA;
    m_dragStart_PointB = m_pointB;
  } else
    // Pass-through event to underlying widget if not over a marker
    event->ignore();
}

//-----------------------------------------------------------------------------------------------
/** Event when the mouse moves
 * @param event mouse event info */
void LineOverlay::mouseReleaseEvent(QMouseEvent *event) {
  if (!(event->buttons() & Qt::RightButton))
    m_rightButton = false;

  // Do not respond to mouse when hidden
  if (!m_showHandles || !m_shown) {
    event->ignore();
    return;
  }

  if (m_dragHandle != HandleNone) {
    // Stop draggin
    m_dragHandle = HandleNone;
    // Drag is over - signal that
    emit lineChanged(m_pointA, m_pointB, m_width);
  } else
    // Pass-through event to underlying widget if not dragging
    event->ignore();
}

} // namespace SliceViewer
} // namespace MantidQt
