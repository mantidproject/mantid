// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/SingleSelector.h"

#include <qwt_plot_picker.h>

#include <QEvent>
#include <QMouseEvent>

#include "MantidQtWidgets/Plotting/Qwt/PreviewPlot.h"

using namespace MantidQt::MantidWidgets;

SingleSelector::SingleSelector(QwtPlot *plot, SelectType type, double position,
                               bool visible)
    : QwtPlotPicker(plot->canvas()), m_type(type), m_position(position),
      m_lowerBound(0.0), m_upperBound(0.0), m_singleMarker(nullptr),
      m_plot(plot), m_canvas(plot->canvas()), m_markerMoving(false),
      m_visible(visible), m_pen(nullptr), m_moveCursor(nullptr) {
  init();
}

SingleSelector::SingleSelector(PreviewPlot *plot, SelectType type,
                               double position, bool visible)
    : QwtPlotPicker(plot->canvas()), m_type(type), m_position(position),
      m_lowerBound(0.0), m_upperBound(0.0), m_singleMarker(nullptr),
      m_plot(plot->getPlot()), m_canvas(plot->canvas()), m_markerMoving(false),
      m_visible(visible), m_pen(nullptr), m_moveCursor(nullptr) {
  init();
}

void SingleSelector::init() {
  m_canvas->installEventFilter(this);

  m_canvas->setCursor(Qt::PointingHandCursor);
  m_singleMarker = new QwtPlotMarker();

  QwtPlotMarker::LineStyle lineStyle;

  switch (m_type) {
  case XSINGLE:
    lineStyle = QwtPlotMarker::VLine;
    m_moveCursor = Qt::SizeHorCursor;
    break;
  case YSINGLE:
    lineStyle = QwtPlotMarker::HLine;
    m_moveCursor = Qt::SizeVerCursor;
    break;
  default:
    lineStyle = QwtPlotMarker::Cross;
    break;
  }

  m_singleMarker->setLineStyle(lineStyle);
  m_singleMarker->attach(m_plot);
  m_singleMarker->setYValue(1.0);

  setPosition(m_position);

  m_pen = new QPen();
  m_pen->setColor(Qt::blue);
  m_pen->setStyle(Qt::DashDotLine);
  m_singleMarker->setLinePen(*m_pen);
}

bool SingleSelector::eventFilter(QObject *obj, QEvent *evn) {
  Q_UNUSED(obj);
  // Do not handle the event if the widget is set to be invisible
  if (!m_visible)
    return false;

  switch (evn->type()) {
  case QEvent::MouseButtonPress: // User has started moving something (perhaps)
  {
    QPoint p = ((QMouseEvent *)evn)->pos();
    double x(0.0), xPlusdx(0.0);
    switch (m_type) {
    case XSINGLE:
      x = m_plot->invTransform(QwtPlot::xBottom, p.x());
      xPlusdx = m_plot->invTransform(QwtPlot::xBottom, p.x() + 3);
      break;
    case YSINGLE:
      x = m_plot->invTransform(QwtPlot::yLeft, p.y());
      xPlusdx = m_plot->invTransform(QwtPlot::yLeft, p.y() + 3);
      break;
    }
    if (isInsideBounds(x) && isMarkerMoving(x, xPlusdx)) {
      m_markerMoving = true;
      m_canvas->setCursor(m_moveCursor);
      return true;
    } else {
      return false;
    }
    break;
  }
  case QEvent::MouseMove: // User is in the process of moving something
                          // (perhaps)
  {
    if (m_markerMoving) {
      QPoint p = ((QMouseEvent *)evn)->pos();
      double x(0.0);
      switch (m_type) {
      case XSINGLE:
        x = m_plot->invTransform(QwtPlot::xBottom, p.x());
        break;
      case YSINGLE:
        x = m_plot->invTransform(QwtPlot::yLeft, p.y());
        break;
      }
      if (isInsideBounds(x)) {
        setPosition(x);
        emit valueChanged(x);
      } else {
        m_canvas->setCursor(Qt::PointingHandCursor);
        m_markerMoving = false;
      }
      m_plot->replot();
      return true;
    } else {
      return false;
    }
    break;
  }
  case QEvent::MouseButtonRelease: {
    m_canvas->setCursor(Qt::PointingHandCursor);
    m_markerMoving = false;

    if (m_markerMoving)
      return true;
    else
      return false;
    break;
  }
  default:
    return false;
  }
}

void SingleSelector::setColour(const QColor &colour) {
  m_pen->setColor(colour);
  m_singleMarker->setLinePen(*m_pen);
}

void SingleSelector::setBounds(const std::pair<double, double> &bounds) {
  setBounds(bounds.first, bounds.second);
}

void SingleSelector::setBounds(const double minimum, const double maximum) {
  setLowerBound(minimum);
  setUpperBound(maximum);
}

void SingleSelector::setLowerBound(const double minimum) {
  m_lowerBound = minimum;
  if (m_lowerBound > getPosition())
    setPosition(minimum);
}

void SingleSelector::setUpperBound(const double maximum) {
  m_upperBound = maximum;
  if (m_upperBound < getPosition())
    setPosition(maximum);
}

void SingleSelector::setPosition(const double position) {
  if (isInsideBounds(position)) {
    setLinePosition(position);
    m_position = position;
    emit valueChanged(position);
  }
}

void SingleSelector::setLinePosition(const double position) {
  switch (m_type) {
  case XSINGLE:
    m_singleMarker->setValue(position, 1.0);
    break;
  case YSINGLE:
    m_singleMarker->setValue(1.0, position);
    break;
  }
  m_plot->replot();
}

double SingleSelector::getPosition() const { return m_position; }

/**
 * @brief Show or hide the marking lines
 * @param state
 */
void SingleSelector::setVisible(bool visible) {
  if (visible)
    m_singleMarker->show();
  else
    m_singleMarker->hide();

  m_plot->replot();
  m_visible = visible;
}

/**
 * @brief Returns whether or not the selector is visible
 * @return Trye if the selector is visible
 */
bool SingleSelector::isVisible() const { return m_visible; }

/**
 * @brief Returns the selector type
 * @return The type of the range selector
 * @return The type of the range selector
 */
SingleSelector::SelectType SingleSelector::getType() const { return m_type; }

/**
 * @brief dettach the line objects marking the minimum and maximum from any plot
 * widget
 */
void SingleSelector::detach() { m_singleMarker->attach(nullptr); }

/**
 * @brief Find out if user is moving the line marking the position of the
 * minimum
 * @param x new candidate position for the minimum
 * @param xPlusdx safety boundary indicating we are closer to the minimum than
 * to the maximum
 * @return
 */
bool SingleSelector::isMarkerMoving(double x, double xPlusdx) {
  return (fabs(x - m_position) <= fabs(xPlusdx - x));
}

/**
 * @brief Check the position (of the mouse pointer) is neither below the lowest
 * allowed value, nor above the hightest allowed value
 * @param x
 * @return true if position within the allowed range
 */
bool SingleSelector::isInsideBounds(double x) {
  return (x >= m_lowerBound && x <= m_upperBound);
}
