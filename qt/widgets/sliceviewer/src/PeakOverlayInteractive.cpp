#include "MantidQtWidgets/SliceViewer/PeakOverlayInteractive.h"
#include "MantidQtWidgets/Common/InputController.h"
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_div.h>

namespace MantidQt {
namespace SliceViewer {

PeakOverlayInteractive::PeakOverlayInteractive(
    PeaksPresenter *const peaksPresenter, QwtPlot *plot, const int plotXIndex,
    const int plotYIndex, QWidget *parent)
    : QWidget(parent), m_presenter(peaksPresenter), m_plot(plot),
      m_plotXIndex(plotXIndex), m_plotYIndex(plotYIndex), m_tool(nullptr) {

  setAttribute(Qt::WA_NoMousePropagation, false);
  setAttribute(Qt::WA_MouseTracking, true);

  // Non-editing by default
  peakDisplayMode();

  this->setVisible(true);
  setUpdatesEnabled(true);
}

PeakOverlayInteractive::~PeakOverlayInteractive() { this->peakDisplayMode(); }

void PeakOverlayInteractive::paintEvent(QPaintEvent *event) {
  // Only paint to update the interactive tool
  if (m_tool) {
    QPainter painter(this);
    m_tool->onPaint(painter);
    painter.end();
  }
  // Sub-classes are responsible for painting their respective peak
  // representations
  this->doPaintPeaks(event);
}

void PeakOverlayInteractive::captureMouseEvents(bool capture) {
  setAttribute(Qt::WA_TransparentForMouseEvents, !capture);
}

void PeakOverlayInteractive::peakDeletionMode() {
  captureMouseEvents(true);
  QApplication::restoreOverrideCursor();
  auto *temp = m_tool;
  auto eraseIcon = new QPixmap(":/PickTools/eraser.png");
  auto *eraseTool =
      new MantidQt::MantidWidgets::InputControllerSelection(this, eraseIcon);
  connect(eraseTool, SIGNAL(selection(QRect)), this, SLOT(erasePeaks(QRect)),
          Qt::QueuedConnection);
  m_tool = eraseTool;
  delete temp;
}

void PeakOverlayInteractive::peakAdditionMode() {
  captureMouseEvents(true);
  QApplication::restoreOverrideCursor();
  auto *temp = m_tool;
  auto *addTool = new MantidQt::MantidWidgets::InputControllerPick(this);
  connect(addTool, SIGNAL(pickPointAt(int, int)), this,
          SLOT(addPeakAt(int, int)));
  m_tool = addTool;
  delete temp;
}

void PeakOverlayInteractive::peakDisplayMode() {
  captureMouseEvents(false /*pass through mouse events*/);
  QApplication::restoreOverrideCursor();
  if (m_tool) {
    delete m_tool;
    m_tool = nullptr;
  }
}

void PeakOverlayInteractive::mousePressEvent(QMouseEvent *e) {
  if (m_tool) {
    m_tool->mousePressEvent(e);
  } else {
    e->ignore();
  }
}

void PeakOverlayInteractive::mouseMoveEvent(QMouseEvent *e) {
  if (m_tool) {
    m_tool->mouseMoveEvent(e);
    this->update();
  }
  e->ignore();
}

void PeakOverlayInteractive::mouseReleaseEvent(QMouseEvent *e) {
  if (m_tool) {
    m_tool->mouseReleaseEvent(e);
  } else {
    e->ignore();
  }
}

void PeakOverlayInteractive::wheelEvent(QWheelEvent *e) {
  if (m_tool) {
    m_tool->wheelEvent(e);
  } else {
    e->ignore();
  }
}

void PeakOverlayInteractive::keyPressEvent(QKeyEvent *e) {
  if (m_tool) {
    m_tool->keyPressEvent(e);
  } else {
    e->ignore();
  }
}

void PeakOverlayInteractive::enterEvent(QEvent *e) {
  if (m_tool) {
    m_tool->enterEvent(e);
  } else {
    e->ignore();
  }
}

void PeakOverlayInteractive::leaveEvent(QEvent *e) {
  if (m_tool) {
    m_tool->leaveEvent(e);
  } else {
    e->ignore();
  }
}

void PeakOverlayInteractive::addPeakAt(int coordX, int coordY) {

  QwtScaleMap xMap = m_plot->canvasMap(m_plotXIndex);
  QwtScaleMap yMap = m_plot->canvasMap(m_plotYIndex);

  const double plotX = xMap.invTransform(double(coordX));
  const double plotY = yMap.invTransform(double(coordY));

  m_presenter->addPeakAt(plotX, plotY);
}

void PeakOverlayInteractive::erasePeaks(const QRect &rect) {
  QwtScaleMap xMap = m_plot->canvasMap(m_plotXIndex);
  QwtScaleMap yMap = m_plot->canvasMap(m_plotYIndex);

  const Left left(xMap.invTransform(rect.left()));
  const Right right(xMap.invTransform(rect.right()));
  const Top top(yMap.invTransform(rect.top()));
  const Bottom bottom(yMap.invTransform(rect.bottom()));
  const SlicePoint slicePoint(-1); // Not required.

  m_presenter->deletePeaksIn(
      PeakBoundingBox(left, right, top, bottom, slicePoint));
}

//----------------------------------------------------------------------------------------------
/// Return the recommended size of the widget
QSize PeakOverlayInteractive::sizeHint() const {
  // TODO: Is there a smarter way to find the right size?
  return QSize(20000, 20000);
  // Always as big as the canvas
  // return m_plot->canvas()->size();
}

QSize PeakOverlayInteractive::size() const { return m_plot->canvas()->size(); }

int PeakOverlayInteractive::height() const {
  return m_plot->canvas()->height();
}

int PeakOverlayInteractive::width() const { return m_plot->canvas()->width(); }
} // namespace SliceViewer
} // namespace MantidQt
