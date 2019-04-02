// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/InputController.h"

#include <QApplication>
#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>

#include <cmath>

namespace MantidQt {
namespace MantidWidgets {

//--------------------------------------------------------------------------------

InputController::InputController(QObject *parent, bool contextAllowed)
    : QObject(parent), m_canShowContextMenu(contextAllowed) {}

//--------------------------------------------------------------------------------

/**
 * Constructor.
 * @param parent :: The parent object.
 */
InputController3DMove::InputController3DMove(QObject *parent)
    : InputController(parent, false), m_isButtonPressed(false) {}

/**
 * Process the mouse press event.
 * Send out movement initialization signals.
 */
void InputController3DMove::mousePressEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::MidButton) {
    emit initZoom(event->x(), event->y());
    m_isButtonPressed = true;
  } else if (event->buttons() & Qt::LeftButton) {
    emit initRotation(event->x(), event->y());
    m_isButtonPressed = true;
  } else if (event->buttons() & Qt::RightButton) {
    emit initTranslation(event->x(), event->y());
    m_isButtonPressed = true;
  }
}

/**
 * Process the mouse move event.
 * Send out surface movement signals.
 */
void InputController3DMove::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    emit rotate(event->x(), event->y());
  } else if (event->buttons() & Qt::RightButton) {
    emit translate(event->x(), event->y());
  } else if (event->buttons() & Qt::MidButton) {
    emit zoom(event->x(), event->y());
  }
}

/**
 * Process the mouse release event.
 * Finalize the interaction.
 */
void InputController3DMove::mouseReleaseEvent(QMouseEvent * /*unused*/) {
  m_isButtonPressed = false;
  emit finish();
}

/**
 * Process the mouse wheel event.
 * Send the wheel zoom signal.
 */
void InputController3DMove::wheelEvent(QWheelEvent *event) {
  emit wheelZoom(event->x(), event->y(), event->delta());
}

//--------------------------------------------------------------------------------

/**
 * Constructor.
 * @param parent :: The parent object.
 */
InputControllerPick::InputControllerPick(QObject *parent)
    : InputController(parent), m_isButtonPressed(false) {}

/**
 * Process the mouse press event.
 */
void InputControllerPick::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isButtonPressed = true;
    m_rect.setRect(event->x(), event->y(), 1, 1);
    emit pickPointAt(event->x(), event->y());
  }
}

/**
 * Process the mouse move event.
 */
void InputControllerPick::mouseMoveEvent(QMouseEvent *event) {
  if (m_isButtonPressed) {
    m_rect.setBottomRight(QPoint(event->x(), event->y()));
    emit setSelection(m_rect);
  } else {
    emit touchPointAt(event->x(), event->y());
  }
}

/**
 * Process the mouse release event.
 */
void InputControllerPick::mouseReleaseEvent(QMouseEvent * /*unused*/) {
  m_isButtonPressed = false;
  emit finishSelection();
}

//--------------------------------------------------------------------------------

/**
 * Constructor.
 */
InputControllerDrawShape::InputControllerDrawShape(QObject *parent)
    : InputController(parent), m_creating(false), m_x(0), m_y(0), m_shapeType(),
      m_isButtonPressed(false) {}

/**
 * Process the mouse press event. Sends addShape or selectAt signal.
 */
void InputControllerDrawShape::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isButtonPressed = true;
    if (m_creating && !m_shapeType.isEmpty()) {
      emit addShape(m_shapeType, event->x(), event->y(), m_borderColor,
                    m_fillColor);
    } else if (event->modifiers() & Qt::ControlModifier) {
      emit selectCtrlAt(event->x(), event->y());
    } else {
      emit selectAt(event->x(), event->y());
    }
    m_x = event->x();
    m_y = event->y();
    m_rect.setRect(event->x(), event->y(), 1, 1);
  }
}

/**
 * Process the mouse move event. If the left mouse button is down sends editing
 * signals.
 */
void InputControllerDrawShape::mouseMoveEvent(QMouseEvent *event) {
  if (m_isButtonPressed) {
    if (m_creating) {
      emit moveRightBottomTo(event->x(), event->y());
    } else {
      emit moveBy(event->x() - m_x, event->y() - m_y);
      m_rect.setBottomRight(QPoint(event->x(), event->y()));
      m_x = event->x();
      m_y = event->y();
      emit setSelection(m_rect);
    }
  } else {
    emit touchPointAt(event->x(), event->y());
  }
}

/**
 * Process the mouse button release event.
 */
void InputControllerDrawShape::mouseReleaseEvent(QMouseEvent * /*unused*/) {
  m_isButtonPressed = false;
  m_creating = false;
  m_shapeType = "";
  emit finishSelection(m_rect);
}

/**
 * Process the keyboard key press event.
 */
void InputControllerDrawShape::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Delete:
  case Qt::Key_Backspace:
    emit removeSelectedShapes();
    break;
  }
}

/**
 * Process event of the mouse leaving the widget.
 */
void InputControllerDrawShape::leaveEvent(QEvent * /*unused*/) {
  emit restoreOverrideCursor();
}

/**
 * Slot for defining the shape to draw and initializing drawing.
 */
void InputControllerDrawShape::startCreatingShape2D(const QString &type,
                                                    const QColor &borderColor,
                                                    const QColor &fillColor) {
  m_creating = true;
  m_shapeType = type;
  m_borderColor = borderColor;
  m_fillColor = fillColor;
}

/**
 * Action on disabling.
 */
void InputControllerDrawShape::onDisabled() {
  m_creating = false;
  emit disabled();
}
//--------------------------------------------------------------------------------

/**
 * Constructor.
 */
InputControllerMoveUnwrapped::InputControllerMoveUnwrapped(QObject *parent)
    : InputController(parent, false), m_isButtonPressed(false) {}

/**
 * Process the mouse press event.
 */
void InputControllerMoveUnwrapped::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
    m_isButtonPressed = true;
    m_rect.setTopLeft(QPoint(event->x(), event->y()));
  }
}

/**
 * Process the mouse move event.
 */
void InputControllerMoveUnwrapped::mouseMoveEvent(QMouseEvent *event) {
  if (m_isButtonPressed) {
    m_rect.setBottomRight(QPoint(event->x(), event->y()));
    emit setSelectionRect(m_rect);
  }
}

/**
 * Process the mouse button release event.
 */
void InputControllerMoveUnwrapped::mouseReleaseEvent(QMouseEvent *event) {
  if (m_isButtonPressed && event->button() == Qt::LeftButton) {
    emit zoom();
  } else if (m_isButtonPressed && event->button() == Qt::RightButton) {
    if (m_rect.width() > 1 && m_rect.height() > 1) {
      emit unzoom();
    } else {
      emit resetZoom();
    }
  }
  m_rect = QRect(); // reset rect
  m_isButtonPressed = false;
}

//--------------------------------------------------------------------------------

/**
 * Constructor.
 */
InputControllerDraw::InputControllerDraw(QObject *parent)
    : InputController(parent), m_max_size(32), m_size(30),
      m_isLeftButtonPressed(false), m_isRightButtonPressed(false),
      m_isActive(false), m_cursor(nullptr) {}

InputControllerDraw::~InputControllerDraw() { delete m_cursor; }

/**
 * Process the mouse press event.
 */
void InputControllerDraw::mousePressEvent(QMouseEvent *event) {
  m_isActive = true;
  setPosition(QPoint(event->x(), event->y()));
  if (event->button() == Qt::LeftButton) {
    m_isLeftButtonPressed = true;
    signalLeftClick();
  } else if (event->button() == Qt::RightButton) {
    m_isRightButtonPressed = true;
    signalRightClick();
  }
}

/**
 * Process the mouse move event.
 */
void InputControllerDraw::mouseMoveEvent(QMouseEvent *event) {
  m_isActive = true;
  setPosition(QPoint(event->x(), event->y()));
  if (m_isLeftButtonPressed) {
    signalLeftClick();
  } else if (m_isRightButtonPressed) {
    signalRightClick();
  }
}

/**
 * Process the mouse button release event.
 */
void InputControllerDraw::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isLeftButtonPressed = false;
  } else if (event->button() == Qt::RightButton) {
    m_isRightButtonPressed = false;
  }
}

void InputControllerDraw::wheelEvent(QWheelEvent *event) {
  int d = m_size + (event->delta() > 0 ? 4 : -4);
  if (d > 2 && d < m_max_size) {
    m_size = d;
    resize();
    redrawCursor();
    QApplication::restoreOverrideCursor();
    QApplication::setOverrideCursor(QCursor(*m_cursor, 0, 0));
  }
}

void InputControllerDraw::enterEvent(QEvent * /*unused*/) {
  redrawCursor();
  QApplication::setOverrideCursor(QCursor(*m_cursor, 0, 0));
  m_isActive = true;
}

void InputControllerDraw::leaveEvent(QEvent * /*unused*/) {
  QApplication::restoreOverrideCursor();
  m_isActive = false;
}

void InputControllerDraw::redrawCursor() {
  if (!m_cursor) {
    m_cursor = new QPixmap(m_max_size, m_max_size);
  }
  drawCursor(m_cursor);
}

void InputControllerDraw::signalRightClick() {}

//--------------------------------------------------------------------------------

InputControllerSelection::InputControllerSelection(QObject *parent,
                                                   QPixmap *icon)
    : InputControllerDraw(parent), m_rect(0, 0, cursorSize(), cursorSize()) {
  m_image = icon;
}

InputControllerSelection::~InputControllerSelection() { delete m_image; }

void InputControllerSelection::onPaint(QPainter &painter) {
  if (isActive() && !isLeftButtonPressed()) {
    painter.drawPixmap(m_rect.bottomRight(), *m_image);
  }
}

void InputControllerSelection::drawCursor(QPixmap *cursor) {
  cursor->fill(QColor(255, 255, 255, 0));
  QPainter painter(cursor);
  auto size = cursorSize();

  auto pen = QPen(Qt::DashLine);
  QVector<qreal> dashPattern;
  dashPattern << 4 << 4;
  pen.setDashPattern(dashPattern);
  pen.setColor(QColor(0, 0, 0));
  painter.setPen(pen);
  painter.drawRect(QRect(0, 0, size, size));

  pen.setColor(QColor(255, 255, 255));
  pen.setDashOffset(4);
  painter.setPen(pen);
  painter.drawRect(QRect(0, 0, size, size));
}

void InputControllerSelection::setPosition(const QPoint &pos) {
  m_rect.moveTopLeft(pos);
}

void InputControllerSelection::resize() {
  auto size = cursorSize();
  m_rect.setSize(QSize(size, size));
}

void InputControllerSelection::signalLeftClick() { emit selection(m_rect); }

//--------------------------------------------------------------------------------

InputControllerDrawAndErase::InputControllerDrawAndErase(QObject *parent)
    : InputControllerDraw(parent), m_pos(0, 0), m_rect(8), m_creating(false) {
  makePolygon();
}

void InputControllerDrawAndErase::makePolygon() {
  auto r = double(cursorSize()) / 2.0;
  double a = 2.0 * M_PI / double(m_rect.size());
  for (int i = 0; i < m_rect.size(); ++i) {
    double ia = double(i) * a;
    auto x = r + static_cast<int>(r * cos(ia));
    auto y = r + static_cast<int>(r * sin(ia));
    m_rect[i] = QPointF(x, y);
  }
}

void InputControllerDrawAndErase::signalLeftClick() {
  auto poly = m_rect.translated(m_pos);
  if (m_creating) {
    m_creating = false;
    emit addShape(poly, m_borderColor, m_fillColor);
  } else {
    emit draw(poly);
  }
}

void InputControllerDrawAndErase::signalRightClick() {
  auto poly = m_rect.translated(m_pos);
  emit erase(poly);
}

void InputControllerDrawAndErase::drawCursor(QPixmap *cursor) {
  cursor->fill(QColor(255, 255, 255, 0));
  QPainter painter(cursor);

  auto bRect = m_rect.boundingRect();
  auto poly = m_rect.translated(-bRect.topLeft());

  auto pen = QPen(Qt::DashLine);
  QVector<qreal> dashPattern;
  qreal dashLength = cursorSize() < 10 ? 1 : 2;
  dashPattern << dashLength << dashLength;
  pen.setDashPattern(dashPattern);
  pen.setColor(QColor(0, 0, 0));
  painter.setPen(pen);
  painter.drawPolygon(poly);

  pen.setColor(QColor(255, 255, 255));
  pen.setDashOffset(dashLength);
  painter.setPen(pen);
  painter.drawPolygon(poly);
}

void InputControllerDrawAndErase::setPosition(const QPoint &pos) {
  m_pos = pos;
}

void InputControllerDrawAndErase::resize() { makePolygon(); }

void InputControllerDrawAndErase::startCreatingShape2D(
    const QColor &borderColor, const QColor &fillColor) {
  m_borderColor = borderColor;
  m_fillColor = fillColor;
  m_creating = true;
}
} // namespace MantidWidgets
} // namespace MantidQt
