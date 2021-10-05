// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/XIntegrationControl.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include "MantidKernel/ConfigService.h"

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>

#include <cfloat>
#include <numeric>

namespace MantidQt::MantidWidgets {

XIntegrationScrollBar::XIntegrationScrollBar(QWidget *parent)
    : QFrame(parent), m_resizeMargin(5), m_init(false), m_resizingLeft(false), m_resizingRight(false), m_moving(false),
      m_changed(false), m_x(0), m_width(0), m_minimum(0.0), m_maximum(1.0) {
  setMouseTracking(true);
  setFrameShape(StyledPanel);
  m_slider = new QPushButton(this);
  m_slider->setMouseTracking(true);
  m_slider->setFocusPolicy(Qt::StrongFocus);
  m_slider->move(0, 0);
  m_slider->installEventFilter(this);
  m_slider->setToolTip("Resize to change integration range");
}

void XIntegrationScrollBar::resizeEvent(QResizeEvent * /*unused*/) {
  if (!m_init) {
    if (!m_isDiscrete) {
      m_slider->resize(width(), height());
    } else {
      m_slider->resize(2 * m_resizeMargin + 1, height());
    }
    m_init = true;
  } else {
    set(m_minimum, m_maximum);
  }
}

void XIntegrationScrollBar::mouseMoveEvent(QMouseEvent *e) { QFrame::mouseMoveEvent(e); }

/**
 * Process events comming to the slider
 * @param object :: pointer to the slider
 * @param e :: event
 */
bool XIntegrationScrollBar::eventFilter(QObject *object, QEvent *e) {
  auto *slider = dynamic_cast<QPushButton *>(object);
  if (!slider)
    return false;

  // first we take care of events that don't depend on the slider being discrete or not
  switch (e->type()) {
  case QEvent::Leave:
    if (QApplication::overrideCursor()) {
      QApplication::restoreOverrideCursor();
    }
    return true;

  case QEvent::KeyRelease: {
    QKeyEvent *keyEv = static_cast<QKeyEvent *>(e);

    // for key press, update only on release of the key
    if ((keyEv->key() == Qt::Key_Left || keyEv->key() == Qt::Key_Right || keyEv->key() == Qt::Key_Up ||
         keyEv->key() == Qt::Key_Down) &&
        !keyEv->isAutoRepeat()) {
      emit changed(m_minimum, m_maximum);
    }
    m_changed = false;
    return true;
  }

  case QEvent::MouseButtonRelease: {
    m_resizingLeft = false;
    m_resizingRight = false;
    m_moving = false;
    if (m_changed) {
      emit changed(m_minimum, m_maximum);
    }
    m_changed = false;
    return false;
  }

  case QEvent::MouseButtonPress: {
    auto *me = static_cast<QMouseEvent *>(e);
    m_x = me->x();
    m_width = m_slider->width();
    if (m_x < m_resizeMargin) {
      m_resizingLeft = true;
    } else if (m_x > m_width - m_resizeMargin) {
      m_resizingRight = true;
    } else {
      m_moving = true;
    }
    return false;
  }
  default:
    break;
  }

  // in all other cases, the treatment is different
  if (m_isDiscrete) {
    return manageEventDiscrete(e);
  } else {
    return manageEventContinuous(e);
  }
}

/**
 * @brief XIntegrationScrollBar::manageEventContinuous
 * Takes care of qt event when the slider is pseudo continuous
 * @param e the qt event to manage
 * @return
 */
bool XIntegrationScrollBar::manageEventContinuous(QEvent *e) {
  switch (e->type()) {
  case QEvent::MouseMove: {
    auto *me = static_cast<QMouseEvent *>(e);
    int x = me->x();
    int w = m_slider->width();
    if (x < m_resizeMargin || x > w - m_resizeMargin) {
      if (!QApplication::overrideCursor()) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
      }
    } else {
      QApplication::restoreOverrideCursor();
    }

    if (m_moving) {
      int idx = x - m_x;
      int new_x = m_slider->x() + idx;
      if (new_x >= 0 && new_x + m_slider->width() <= this->width()) {
        int new_y = m_slider->y();
        m_slider->move(new_x, new_y);
        m_changed = true;
        updateMinMax();
      }
    } else if (m_resizingLeft) {
      int idx = x - m_x;
      int new_x = m_slider->x() + idx;
      int new_w = m_slider->width() - idx;
      if (new_x >= 0 && new_w > 2 * m_resizeMargin) {
        m_slider->move(new_x, m_slider->y());
        m_slider->resize(new_w, m_slider->height());
        m_changed = true;
        updateMinMax();
      }
    } else if (m_resizingRight) {
      int dx = x - m_x;
      int new_w = m_width + dx;
      int xright = m_slider->x() + new_w;
      if (xright <= this->width() && new_w > 2 * m_resizeMargin) {
        m_slider->resize(new_w, m_slider->height());
        m_changed = true;
        updateMinMax();
      }
    }
    return true;
  }
  case QEvent::KeyPress: {

    QKeyEvent *keyEv = static_cast<QKeyEvent *>(e);
    int step = 1;
    int sliderx = m_slider->x();
    int slidery = m_slider->y();
    int sliderWidth = m_slider->width();
    int sliderHeight = m_slider->height();
    int totalWidth = this->width();

    switch (keyEv->key()) {
    case Qt::Key_Left: {
      if (sliderx >= step) {
        m_slider->move(sliderx - step, slidery);
      } else {
        m_slider->move(0, slidery);
      }
      break;
    }

    case Qt::Key_Right: {
      if (sliderx + sliderWidth + step < totalWidth) {
        m_slider->move(sliderx + step, slidery);
      } else {
        m_slider->move(totalWidth - sliderWidth, slidery);
      }
      break;
    }

    case Qt::Key_Up: {
      // widen the range

      // expand to the left, depending on the space on this side
      if (sliderx > step) {
        m_slider->resize(sliderWidth + step, sliderHeight);
        m_slider->move(sliderx - step, slidery);
      } else {
        m_slider->resize(sliderWidth + sliderx, sliderHeight);
        m_slider->move(0, slidery);
      }

      // then expand to the right, depending on the space on this side
      if (sliderx + sliderWidth + step <= totalWidth) {
        m_slider->resize(m_slider->width() + step, sliderHeight);
      } else {
        m_slider->resize(totalWidth - m_slider->x(), sliderHeight);
      }
      break;
    }

    case Qt::Key_Down: {
      // shrink the range

      // only change the range if it is not already minimal
      if (sliderWidth > m_resizeMargin) {

        // shrink depending on how far the range is from being minimal
        if (sliderWidth - 2 * step >= m_resizeMargin) {
          m_slider->move(sliderx + step, slidery);
          m_slider->resize(sliderWidth - 2 * step, sliderHeight);
        } else {
          m_slider->move(sliderx + (sliderWidth - m_resizeMargin) / 2, slidery);
          m_slider->resize(m_resizeMargin, sliderHeight);
        }
      }
      break;
    }
    default:
      return false;
    }
    m_changed = true;
    updateMinMax();
    return true;
  }
  default:
    return false;
  }
}

/**
 * @brief XIntegrationScrollBar::manageEventDiscrete
 * Manage events for the scroll bar when the bar is supposed to work along discrete intervals rather than pseudo
 * continuous ones.
 * @param e the qt event
 */
bool XIntegrationScrollBar::manageEventDiscrete(QEvent *e) {

  switch (e->type()) {
  case QEvent::KeyPress: {
    QKeyEvent *keyEv = static_cast<QKeyEvent *>(e);

    switch (keyEv->key()) {
    case Qt::Key_Right:
      if (m_currentStepMax < m_stepsTotal - 1) {
        m_currentStepMax += 1;
        m_currentStepMin += 1;
      }
      break;
    case Qt::Key_Left:
      if (m_currentStepMin > 0) {
        m_currentStepMin -= 1;
        m_currentStepMax -= 1;
      }
      break;
    case Qt::Key_Up:
      // condition so that the slider alternate the growing side
      if (((m_currentStepMax + m_currentStepMin) % 2 == 0 || m_currentStepMin == 0) &&
          m_currentStepMax < m_stepsTotal - 1) {
        m_currentStepMax += 1;
      } else if (m_currentStepMin > 0) {
        m_currentStepMin -= 1;
      }
      break;
    case Qt::Key_Down:
      // first check if the slider can be smaller
      if (m_currentStepMax - m_currentStepMin > 0) {
        // if so, alternate from which side it shrinks
        if ((m_currentStepMax + m_currentStepMin) % 2 == 1) {
          m_currentStepMax -= 1;
        } else {
          m_currentStepMin += 1;
        }
      }
      break;
    default:
      // an unhelpful key was pressed, nothing to do here
      return false;
    }
    // this wasn't the default case, a useful key was actually pressed, so the values need to be updated
    setDiscreteValues();
    emit running(m_minimum, m_maximum);
    return true;
  }

  case QEvent::MouseMove: {
    auto *me = static_cast<QMouseEvent *>(e);
    int x = me->x();
    int w = m_slider->width();
    if (x < m_resizeMargin || x > w - m_resizeMargin) {
      if (!QApplication::overrideCursor()) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
      }
    } else {
      QApplication::restoreOverrideCursor();
    }

    int idx = x - m_x;

    if (m_moving) {
      int new_x = m_slider->x() + idx;
      int width = m_currentStepMax - m_currentStepMin;
      m_currentStepMin = std::min(
          static_cast<int>(std::round(static_cast<double>(std::max(new_x, 0) * (m_stepsTotal - 1)) / this->width())),
          m_stepsTotal - width - 1);
      m_currentStepMax = m_currentStepMin + width;

    } else if (m_resizingLeft) {
      int new_x = m_slider->x() + idx;
      m_currentStepMin =
          static_cast<int>(std::round(static_cast<double>(std::max(new_x, 0) * (m_stepsTotal - 1)) / this->width()));
      m_currentStepMin = std::min(m_currentStepMin, m_currentStepMax);

    } else if (m_resizingRight) {
      int new_x = m_slider->x() + idx + m_width;
      m_currentStepMax = static_cast<int>(
          std::round(static_cast<double>(std::min(new_x, this->width()) * (m_stepsTotal - 1) / this->width())));
      m_currentStepMax = std::max(m_currentStepMax, m_currentStepMin);
    } else {
      // the mouse is just moving around, no need to update anything
      return true;
    }

    m_changed = true;
    setDiscreteValues();
    emit running(m_minimum, m_maximum);

    return true;
  }
  default:
    return false;
  }
}

/**
 * @brief XIntegrationScrollBar::setDiscreteValues
 * If the slider is discrete, convert the current step min and step max to new min and max (between 0 and 1) and update
 * the slider. Mostly a convenience function.
 */
void XIntegrationScrollBar::setDiscreteValues() {
  if (!m_isDiscrete)
    return;
  set(std::max((static_cast<double>(m_currentStepMin) - 0.1) / (m_stepsTotal - 1), 0.),
      std::min((static_cast<double>(m_currentStepMax) + 0.1) / (m_stepsTotal - 1), 1.));
}

/**
 * Return the minimum value (between 0 and 1)
 */
double XIntegrationScrollBar::getMinimum() const { return m_minimum; }

/**
 * Return the maximum value (between 0 and 1)
 */
double XIntegrationScrollBar::getMaximum() const { return m_maximum; }

/**
 * Return the width == maximum - minimum (value is between 0 and 1)
 */
double XIntegrationScrollBar::getWidth() const { return m_maximum - m_minimum; }

/**
 * @brief XIntegrationScrollBar::setStepsTotal
 * Set the total number of steps and reset the current scroll bar internal variables to match these new values.
 * @param steps the new total number of step for the discrete bar.
 */
void XIntegrationScrollBar::setStepsTotal(int steps) {
  m_stepsTotal = steps;
  m_currentStepMin = 0;
  m_currentStepMax = 0;
  m_minimum = 0;
  m_maximum = 0;
}

/**
 * Set new minimum and maximum values
 */
void XIntegrationScrollBar::set(double minimum, double maximum) {
  if (minimum < 0 || minimum > 1. || maximum < 0 || maximum > 1.) {
    throw std::invalid_argument("XIntegrationScrollBar : minimum and maximum must be between 0 and 1");
  }
  if (minimum > maximum) {
    std::swap(minimum, maximum);
  }
  m_minimum = minimum;
  m_maximum = maximum;
  m_currentStepMin = static_cast<int>(std::round(minimum * static_cast<double>(m_stepsTotal - 1)));
  m_currentStepMax = static_cast<int>(std::round(maximum * static_cast<double>(m_stepsTotal - 1)));

  int x = static_cast<int>(m_minimum * this->width());
  int w = static_cast<int>((m_maximum - m_minimum) * this->width());

  if (w <= 2 * m_resizeMargin)
    w = 2 * m_resizeMargin + 1;
  if (x > this->width() - 2 * m_resizeMargin - 1)
    x = this->width() - 2 * m_resizeMargin - 1;

  m_slider->move(x, 0);
  m_slider->resize(w, this->height());
}

/**
 * @brief XIntegrationScrollBar::updateMinMax
 * Use the current slider position to update the integration. Do not use in discrete case.
 */
void XIntegrationScrollBar::updateMinMax() {
  if (!m_isDiscrete) {
    m_minimum = double(m_slider->x()) / this->width();
    m_maximum = m_minimum + double(m_slider->width()) / this->width();
  }
  emit running(m_minimum, m_maximum);
}

//---------------------------------------------------------------------------------//
XIntegrationControl::XIntegrationControl(InstrumentWidget *instrWindow)
    : QFrame(instrWindow), m_instrWindow(instrWindow), m_totalMinimum(0), m_totalMaximum(1), m_minimum(0),
      m_maximum(1) {
  auto *layout = new QHBoxLayout();
  m_minText = new QLineEdit(this);
  m_minText->setMaximumWidth(100);
  m_minText->setToolTip("Minimum x value");
  m_maxText = new QLineEdit(this);
  m_maxText->setMaximumWidth(100);
  m_maxText->setToolTip("Maximum x value");

  m_minSpin = new QSpinBox(this);
  m_minSpin->setMaximumWidth(100);
  m_minSpin->setToolTip("Minimum channel index. Click outside to refresh.");
  m_maxSpin = new QSpinBox(this);
  m_maxSpin->setMaximumWidth(100);
  m_maxSpin->setToolTip("Maximum channel index. Click outside to refresh.");

  m_units = new QLabel("TOF", this);
  m_setWholeRange = new QPushButton("Reset");
  m_setWholeRange->setToolTip("Reset integration range to maximum");
  m_scrollBar = new XIntegrationScrollBar(this);

  layout->addWidget(m_units, 0);
  layout->addWidget(m_minSpin, 0);
  layout->addWidget(m_minText, 0);
  layout->addWidget(m_scrollBar, 1);
  layout->addWidget(m_maxSpin, 0);
  layout->addWidget(m_maxText, 0);

  layout->addWidget(m_setWholeRange, 0);
  setLayout(layout);

  connect(m_scrollBar, SIGNAL(changed(double, double)), this, SLOT(sliderChanged(double, double)));
  connect(m_scrollBar, SIGNAL(running(double, double)), this, SLOT(sliderRunning(double, double)));
  connect(m_minText, SIGNAL(editingFinished()), this, SLOT(setMinimum()));
  connect(m_maxText, SIGNAL(editingFinished()), this, SLOT(setMaximum()));
  connect(m_minSpin, SIGNAL(editingFinished()), this, SLOT(setMinimum()));
  connect(m_maxSpin, SIGNAL(editingFinished()), this, SLOT(setMaximum()));
  connect(m_setWholeRange, SIGNAL(clicked()), this, SLOT(setWholeRange()));
  updateTextBoxes();
}

void XIntegrationControl::sliderChanged(double minimum, double maximum) {
  double w = m_totalMaximum - m_totalMinimum;
  m_minimum = m_totalMinimum + minimum * w;
  m_maximum = m_totalMinimum + maximum * w;
  if (!m_isDiscrete && w > 0 && (m_maximum - m_minimum) / w >= 0.98) {
    m_minimum = m_totalMinimum;
    m_maximum = m_totalMaximum;
  }
  updateTextBoxes();
  emit changed(m_minimum, m_maximum);
}

void XIntegrationControl::sliderRunning(double minimum, double maximum) {
  double w = m_totalMaximum - m_totalMinimum;
  m_minimum = m_totalMinimum + minimum * w;
  m_maximum = m_totalMinimum + maximum * w;
  updateTextBoxes();
}

void XIntegrationControl::discretize() {
  m_totalMinimum = std::round(m_totalMinimum);
  m_totalMaximum = std::round(m_totalMaximum);
  m_minimum = std::round(m_minimum);
  m_maximum = std::round(m_maximum);
}

void XIntegrationControl::setTotalRange(double minimum, double maximum) {
  if (minimum > maximum) {
    std::swap(minimum, maximum);
  }
  m_totalMinimum = minimum;
  m_totalMaximum = maximum;

  if (m_isDiscrete) {
    // if the slider is discrete, we reset it to its usual starting position
    discretize();
    m_scrollBar->setStepsTotal(static_cast<int>(m_totalMaximum - m_totalMinimum + 1));
    setRange(0, 0);
  } else {
    // we keep as much of the previous slider as possible
    if (m_scrollBar->getMinimum() != 0 || m_scrollBar->getMaximum() != 1) {
      m_minimum = std::min(std::max(m_minimum, minimum), m_totalMaximum);
      m_maximum = std::max(std::min(m_maximum, maximum), m_totalMinimum);
      setRange(m_minimum, m_maximum);
    } else {
      m_minimum = minimum;
      m_maximum = maximum;
    }
    updateTextBoxes();
  }
}

void XIntegrationControl::setRange(double minimum, double maximum) {
  if (minimum > maximum) {
    std::swap(minimum, maximum);
  }
  if ((minimum < m_totalMinimum) || (minimum > m_totalMaximum)) {
    minimum = m_totalMinimum;
  }
  if ((maximum > m_totalMaximum) || (maximum < m_totalMinimum)) {
    maximum = m_totalMaximum;
  }
  m_minimum = minimum;
  m_maximum = maximum;
  double w = m_totalMaximum - m_totalMinimum;
  m_scrollBar->set((m_minimum - m_totalMinimum) / w, (m_maximum - m_totalMinimum) / w);

  updateTextBoxes();

  emit changed(m_minimum, m_maximum);
}

void XIntegrationControl::setDiscrete(bool isDiscrete) {
  m_isDiscrete = isDiscrete;
  if (m_isDiscrete) {
    m_minText->hide();
    m_maxText->hide();
    m_minSpin->show();
    m_maxSpin->show();
  } else {
    m_minSpin->hide();
    m_maxSpin->hide();
    m_minText->show();
    m_maxText->show();
  }

  m_scrollBar->setDiscrete(m_isDiscrete);
}

void XIntegrationControl::setWholeRange() { setRange(m_totalMinimum, m_totalMaximum); }

double XIntegrationControl::getMinimum() const { return m_minimum; }

double XIntegrationControl::getMaximum() const { return m_maximum; }

double XIntegrationControl::getWidth() const { return m_maximum - m_minimum; }

void XIntegrationControl::updateTextBoxes() {
  if (m_isDiscrete) {
    discretize();
    m_minSpin->setRange(static_cast<int>(m_totalMinimum), static_cast<int>(m_maximum));
    m_maxSpin->setRange(static_cast<int>(m_minimum), static_cast<int>(m_totalMaximum));
    m_minSpin->setValue(static_cast<int>(m_minimum));
    m_maxSpin->setValue(static_cast<int>(m_maximum));
  } else {
    m_minText->setText(QString::number(m_minimum));
    m_maxText->setText(QString::number(m_maximum));
  }
  m_setWholeRange->setEnabled(m_minimum != m_totalMinimum || m_maximum != m_totalMaximum);
}

void XIntegrationControl::setMinimum() {
  bool ok;
  QString text = m_isDiscrete ? m_minSpin->text() : m_minText->text();
  double minValue = text.toDouble(&ok);
  if (!ok)
    return;
  double maxValue = getMaximum();
  setRange(minValue, maxValue);
}

void XIntegrationControl::setMaximum() {
  bool ok;
  QString text = m_isDiscrete ? m_maxSpin->text() : m_maxText->text();

  double maxValue = text.toDouble(&ok);
  if (!ok)
    return;
  double minValue = getMinimum();
  setRange(minValue, maxValue);
}

void XIntegrationControl::setUnits(const QString &units) { m_units->setText(units); }
} // namespace MantidQt::MantidWidgets
