// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/RangeSelector.h"

#include <qwt_plot_picker.h>

#include <QEvent>
#include <QMouseEvent>

#include "MantidQtWidgets/Plotting/Qwt/PreviewPlot.h"

using namespace MantidQt::MantidWidgets;

RangeSelector::RangeSelector(QwtPlot *plot, SelectType type, bool visible,
                             bool infoOnly)
    : QwtPlotPicker(plot->canvas()), m_type(type), m_min(0.0), m_max(0.0),
      m_lower(0.0), m_higher(0.0), m_mrkMin(nullptr), m_mrkMax(nullptr),
      m_plot(plot), m_canvas(plot->canvas()), m_minChanging(false),
      m_maxChanging(false), m_infoOnly(infoOnly), m_visible(visible),
      m_pen(nullptr), m_movCursor() {
  init();
}

RangeSelector::RangeSelector(PreviewPlot *plot, SelectType type, bool visible,
                             bool infoOnly)
    : QwtPlotPicker(plot->m_uiForm.plot->canvas()), m_type(type), m_min(0.0),
      m_max(0.0), m_lower(0.0), m_higher(0.0), m_mrkMin(nullptr),
      m_mrkMax(nullptr), m_plot(plot->m_uiForm.plot),
      m_canvas(plot->m_uiForm.plot->canvas()), m_minChanging(false),
      m_maxChanging(false), m_infoOnly(infoOnly), m_visible(visible),
      m_pen(nullptr), m_movCursor() {
  init();
}

void RangeSelector::init() {
  m_canvas->installEventFilter(this);

  m_canvas->setCursor(
      Qt::PointingHandCursor); ///< @todo Make this an option at some point

  m_mrkMin = new QwtPlotMarker();
  m_mrkMax = new QwtPlotMarker();

  QwtPlotMarker::LineStyle lineStyle;

  switch (m_type) {
  case XMINMAX:
  case XSINGLE:
    lineStyle = QwtPlotMarker::VLine;
    m_movCursor = Qt::SizeHorCursor;
    break;
  case YMINMAX:
  case YSINGLE:
    lineStyle = QwtPlotMarker::HLine;
    m_movCursor = Qt::SizeVerCursor;
    break;
  default:
    lineStyle = QwtPlotMarker::Cross;
    break;
  }

  switch (m_type) {
  case XMINMAX:
  case YMINMAX:
    m_mrkMax->setLineStyle(lineStyle);
    m_mrkMax->attach(m_plot);
    m_mrkMax->setYValue(1.0);
  case XSINGLE:
  case YSINGLE:
    m_mrkMin->setLineStyle(lineStyle);
    m_mrkMin->attach(m_plot);
    m_mrkMin->setYValue(0.0);
    break;
  }

  m_minChanging = false;
  m_maxChanging = false;

  setMin(100); // known starting values
  setMax(200);

  /// Setup pen with default values
  m_pen = new QPen();
  m_pen->setColor(Qt::blue);
  m_pen->setStyle(Qt::DashDotLine);

  // Apply pen to marker objects
  m_mrkMin->setLinePen(*m_pen);
  m_mrkMax->setLinePen(*m_pen);
}

bool RangeSelector::eventFilter(QObject *obj, QEvent *evn) {
  Q_UNUSED(obj);
  // Do not handle the event if the widget is set to be invisible
  if (!m_visible || m_infoOnly) {
    return false;
  }

  switch (evn->type()) {
  case QEvent::MouseButtonPress: // User has started moving something (perhaps)
  {
    QPoint p = ((QMouseEvent *)evn)->pos();
    double x(0.0), xPlusdx(0.0);
    switch (m_type) {
    case XMINMAX:
    case XSINGLE:
      x = m_plot->invTransform(QwtPlot::xBottom, p.x());
      xPlusdx = m_plot->invTransform(QwtPlot::xBottom, p.x() + 3);
      break;
    case YMINMAX:
    case YSINGLE:
      x = m_plot->invTransform(QwtPlot::yLeft, p.y());
      xPlusdx = m_plot->invTransform(QwtPlot::yLeft, p.y() + 3);
      break;
    }
    if (inRange(x, fabs(x - xPlusdx))) {
      if (changingMin(x, xPlusdx)) {
        m_minChanging = true;
        m_canvas->setCursor(m_movCursor);
        setMin(x);
        m_plot->replot();
        return true;
      } else if (changingMax(x, xPlusdx)) {
        m_maxChanging = true;
        m_canvas->setCursor(m_movCursor);
        setMax(x);
        m_plot->replot();
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
    break;
  }
  case QEvent::MouseMove: // User is in the process of moving something
                          // (perhaps)
  {
    if (m_minChanging || m_maxChanging) {
      QPoint p = ((QMouseEvent *)evn)->pos();
      double x(0.0), xPlusdx(0.0);
      switch (m_type) {
      case XMINMAX:
      case XSINGLE:
        x = m_plot->invTransform(QwtPlot::xBottom, p.x());
        xPlusdx = m_plot->invTransform(QwtPlot::xBottom, p.x() + 3);
        break;
      case YMINMAX:
      case YSINGLE:
        x = m_plot->invTransform(QwtPlot::yLeft, p.y());
        xPlusdx = m_plot->invTransform(QwtPlot::yLeft, p.y() + 3);
        break;
      }
      if (inRange(x, fabs(x - xPlusdx))) {
        if (m_minChanging) {
          if (x <= m_max) {
            setMin(x);
          } else {
            setMax(x);
            m_minChanging = false;
            m_maxChanging = true;
          }
        } else {
          if (x >= m_min) {
            setMax(x);
          } else {
            setMin(x);
            m_minChanging = true;
            m_maxChanging = false;
          }
        }
      } else {
        m_canvas->setCursor(Qt::PointingHandCursor);
        m_minChanging = false;
        m_maxChanging = false;
        emit selectionChangedLazy(m_min, m_max);
      }
      m_plot->replot();
      return true;
    } else {
      return false;
    }
    break;
  }
  case QEvent::MouseButtonRelease: // User has finished moving something
                                   // (perhaps)
  {
    if (m_minChanging || m_maxChanging) {
      m_canvas->setCursor(Qt::PointingHandCursor);
      m_minChanging = false;
      m_maxChanging = false;
      emit selectionChangedLazy(m_min, m_max);
      return true;
    } else {
      return false;
    }
    break;
  }
  default:
    return false;
  }
}

/**
 * @brief set the lowest and highest values for the position of the minimum and
 * maximum
 * @post ensures the lines marking the position of the maximum and the minimum
 * will be within the new [lowest,highest] range
 * @param min
 * @param max
 */
void RangeSelector::setRange(double min, double max) {
  m_lower = (min < max) ? min : max;
  m_higher = (min < max) ? max : min;
  verify();
  emit rangeChanged(min, max);
}

/**
 * @brief set the lowest and highest values for the position of the minimum and
 * maximum
 * @param range
 */
void RangeSelector::setRange(const std::pair<double, double> &range) {
  this->setRange(range.first, range.second);
}

/**
 * @brief get the lowest and highest limits for the range selector
 * @return the limits for the allowed values of the range selector
 */
std::pair<double, double> RangeSelector::getRange() const {
  std::pair<double, double> limits(m_lower, m_higher);
  return limits;
}

/**
 * @brief Update the line object marking the minimum, and replot
 * @param val new position of the minimum
 */
void RangeSelector::setMinLinePos(double val) {
  switch (m_type) {
  case XMINMAX:
  case XSINGLE:
    m_mrkMin->setValue(val, 1.0);
    break;
  case YMINMAX:
  case YSINGLE:
    m_mrkMin->setValue(1.0, val);
    break;
  }
  m_plot->replot();
}

/**
 * @brief Update the line object marking the maximum, and replot
 * @param val new position of the maximum
 */
void RangeSelector::setMaxLinePos(double val) {
  switch (m_type) {
  case XMINMAX:
  case XSINGLE:
    m_mrkMax->setValue(val, 1.0);
    break;
  case YMINMAX:
  case YSINGLE:
    m_mrkMax->setValue(1.0, val);
    break;
  }
  m_plot->replot();
}

/**
 * @brief syntactic sugar for RangeSelector::setMin
 */
void RangeSelector::setMinimum(double val) { setMin(val); }

/**
 * @brief syntactic sugar for RangeSelector::setMax
 */
void RangeSelector::setMaximum(double val) { setMax(val); }

/**
 * @brief Attach to the plot widget the line objects marking the minimum and
 * maximum
 */
void RangeSelector::reapply() {
  m_mrkMin->attach(m_plot);
  m_mrkMax->attach(m_plot);
}

/**
 * @brief dettach the line objects marking the minimum and maximum from any plot
 * widget
 */
void RangeSelector::detach() {
  m_mrkMin->attach(nullptr);
  m_mrkMax->attach(nullptr);
}

void RangeSelector::setColour(QColor colour) {
  m_pen->setColor(colour);
  switch (m_type) {
  case XMINMAX:
  case YMINMAX:
    m_mrkMax->setLinePen(*m_pen);
  case XSINGLE:
  case YSINGLE:
    m_mrkMin->setLinePen(*m_pen);
    break;
  }
}

void RangeSelector::setInfoOnly(bool state) { m_infoOnly = state; }

/**
 * @brief Show or hide the marking lines
 * @param state
 */
void RangeSelector::setVisible(bool state) {
  if (state) {
    m_mrkMin->show();
    m_mrkMax->show();
  } else {
    m_mrkMin->hide();
    m_mrkMax->hide();
  }
  m_plot->replot();
  m_visible = state;
}

/**
 * @brief Update the position of the lines marking the minimum and maximum, and
 * signal the changes
 * @post ensures the lines marking the position of the maximum and the minimum
 * will be within the new [lowest,highest] range
 * @param min the position of the minimum
 * @param max the position of the maximum
 */
void RangeSelector::setMaxMin(const double min, const double max) {
  if (min == m_min && max == m_max) {
    // this is just to save work, the comparison above may fail if min or max
    // are represented differently in the machine but that wont cause bad result
    return;
  }
  m_min = (min > m_lower) ? min : m_lower;
  m_max = (max < m_higher) ? max : m_higher;
  setMinLinePos(m_min);
  setMaxLinePos(m_max);
  emit selectionChanged(m_min, m_max);
  emit minValueChanged(m_min);
  emit maxValueChanged(m_max);
}

/** Update the position of the line marking the minimum, and signal the change
 * @post ensures the new position is above the minimum value allowed
 * @param val the position of the minimum
 */
void RangeSelector::setMin(double val) {
  if (val != m_min) {
    m_min = (val > m_lower) ? val : m_lower;
    setMinLinePos(m_min);
    emit minValueChanged(m_min);
    emit selectionChanged(m_min, m_max);
  }
}

/** Update the position of the line marking the maximum, and signal the change
 * @post ensures the new position is below the maximum value allowed
 * @param val the position of the maximum
 */
void RangeSelector::setMax(double val) {
  if (val != m_max) {
    m_max = (val < m_higher) ? val : m_higher;
    setMaxLinePos(m_max);
    emit maxValueChanged(m_max);
    emit selectionChanged(m_min, m_max);
  }
}

/**
 * @brief Find out if user is moving the line marking the position of the
 * minimum
 * @param x new candidate position for the minimum
 * @param xPlusdx safety boundary indicating we are closer to the minimum than
 * to the maximum
 * @return
 */
bool RangeSelector::changingMin(double x, double xPlusdx) {
  return (fabs(x - m_min) <= fabs(xPlusdx - x));
}

/**
 * @brief Find out if user is moving the line marking the position of the
 * maximum
 * @param x new candidate position for the maximum
 * @param xPlusdx safety boundary indicating we are closer to the maximum than
 * to the minimum
 * @return
 */
bool RangeSelector::changingMax(double x, double xPlusdx) {
  return (fabs(x - m_max) <= fabs(xPlusdx - x));
}

/**
 * @brief Ensure that current position for minimum is not below the lowest
 * allowed value, and that the current position for the maximum is not above the
 * highest allowed value. Also ensure the minimum is lower than the maximum
 */
void RangeSelector::verify() {
  double min(m_min);
  double max(m_max);

  if (min > max) {
    std::swap(min, max);
  }

  if (min < m_lower || min > m_higher) {
    min = m_lower;
  }
  if (max < m_lower || max > m_higher) {
    max = m_higher;
  }
  setMaxMin(min, max);
}

/**
 * @brief Check the position (of the mouse pointer) is neither below the lowest
 * allowed value, nor above the hightest allowed value
 * @param x
 * @return true if position within the allowed range
 */
bool RangeSelector::inRange(double x, double dx) {
  return (x >= m_lower - dx && x <= m_higher + dx);
}
