// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/SimpleWidget.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

#include <QApplication>
#include <QPixmap>

namespace MantidQt {
namespace MantidWidgets {

/// Constructor
SimpleWidget::SimpleWidget(QWidget *parent) : QWidget(parent) {
  // Receive mouse move events
  setMouseTracking(true);
  // Receive keyboard events
  setFocusPolicy(Qt::StrongFocus);
}

SimpleWidget::~SimpleWidget() {}

/// Assign a surface to draw on
void SimpleWidget::setSurface(boost::shared_ptr<ProjectionSurface> surface) {
  m_surface = surface;
  connect(m_surface.get(), SIGNAL(redrawRequired()), this, SLOT(repaint()),
          Qt::QueuedConnection);
}

/// Redraw the view
/// @param picking :: Set to true to update the picking image regardless the
/// interaction
///   mode of the surface.
void SimpleWidget::updateView(bool picking) {
  if (m_surface) {
    m_surface->updateView(picking);
    update();
  }
}

/// Update the detector information (count values) and redraw
void SimpleWidget::updateDetectors() {
  if (m_surface) {
    m_surface->updateDetectors();
    update();
  }
}

/**
 * Save widget content to a file.
 * @param filename :: A file to save to.
 */
void SimpleWidget::saveToFile(const QString &filename) {
  QPixmap image(size());
  render(&image);
  image.save(filename);
}

void SimpleWidget::paintEvent(QPaintEvent * /*unused*/) {
  if (m_surface) {
    m_surface->drawSimple(this);
  }
}

void SimpleWidget::resizeEvent(QResizeEvent * /*unused*/) {
  if (m_surface) {
    m_surface->updateView();
  }
}

/**
 * Mouse press callback method, It implements mouse button press initialize
 * methods.
 * @param event :: This is the event variable which has the position and button
 * states
 */
void SimpleWidget::mousePressEvent(QMouseEvent *event) {
  if (m_surface) {
    m_surface->mousePressEvent(event);
  }
  update();
}

/**
 * This is mouse move callback method. It implements the actions to be taken
 * when the mouse is moved with a particular button is pressed.
 * @param event :: This is the event variable which has the position and button
 * states
 */
void SimpleWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_surface) {
    m_surface->mouseMoveEvent(event);
  }
  repaint();
}

/**
 * This is mouse button release callback method. This resets the cursor to
 * pointing hand cursor
 * @param event :: This is the event variable which has the position and button
 * states
 */
void SimpleWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (m_surface) {
    m_surface->mouseReleaseEvent(event);
  }
  repaint();
}

/**
 * Mouse wheel event to set the zooming in and out
 * @param event :: This is the event variable which has the status of the wheel
 */
void SimpleWidget::wheelEvent(QWheelEvent *event) {
  if (m_surface) {
    m_surface->wheelEvent(event);
  }
  update();
}

/**
 * Key press event
 * @param event :: This is the event variable which has the status of the
 * keyboard
 */
void SimpleWidget::keyPressEvent(QKeyEvent *event) {
  if (m_surface) {
    m_surface->keyPressEvent(event);
  }
  update();
}

void SimpleWidget::enterEvent(QEvent *event) {
  if (m_surface) {
    m_surface->enterEvent(event);
  }
  update();
}

void SimpleWidget::leaveEvent(QEvent *event) {
  // Restore possible override cursor
  while (QApplication::overrideCursor()) {
    QApplication::restoreOverrideCursor();
  }
  if (m_surface) {
    m_surface->leaveEvent(event);
  }
  update();
}
} // namespace MantidWidgets
} // namespace MantidQt
