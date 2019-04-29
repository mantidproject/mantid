// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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

#include <cfloat>
#include <numeric>

namespace MantidQt {
namespace MantidWidgets {

XIntegrationScrollBar::XIntegrationScrollBar(QWidget *parent)
    : QFrame(parent), m_resizeMargin(5), m_init(false), m_resizingLeft(false),
      m_resizingRight(false), m_moving(false), m_changed(false), m_x(0),
      m_width(0), m_minimum(0.0), m_maximum(1.0) {
  setMouseTracking(true);
  setFrameShape(StyledPanel);
  m_slider = new QPushButton(this);
  m_slider->setMouseTracking(true);
  m_slider->move(0, 0);
  m_slider->installEventFilter(this);
  m_slider->setToolTip("Resize to change integration range");
}

void XIntegrationScrollBar::resizeEvent(QResizeEvent * /*unused*/) {
  if (!m_init) {
    m_slider->resize(width(), height());
    m_init = true;
  } else {
    set(m_minimum, m_maximum);
  }
}

void XIntegrationScrollBar::mouseMoveEvent(QMouseEvent *e) {
  QFrame::mouseMoveEvent(e);
}

/**
 * Process events comming to the slider
 * @param object :: pointer to the slider
 * @param e :: event
 */
bool XIntegrationScrollBar::eventFilter(QObject *object, QEvent *e) {
  QPushButton *slider = dynamic_cast<QPushButton *>(object);
  if (!slider)
    return false;
  if (e->type() == QEvent::Leave) {
    if (QApplication::overrideCursor()) {
      QApplication::restoreOverrideCursor();
    }
    return true;
  } else if (e->type() == QEvent::MouseButtonPress) {
    QMouseEvent *me = static_cast<QMouseEvent *>(e);
    m_x = me->x();
    m_width = m_slider->width();
    if (m_x < m_resizeMargin) {
      m_resizingLeft = true;
    } else if (m_x > m_width - m_resizeMargin) {
      m_resizingRight = true;
    } else {
      m_moving = true;
    }
  } else if (e->type() == QEvent::MouseButtonRelease) {
    m_resizingLeft = false;
    m_resizingRight = false;
    m_moving = false;
    if (m_changed) {
      emit changed(m_minimum, m_maximum);
    }
    m_changed = false;
  } else if (e->type() == QEvent::MouseMove) {
    QMouseEvent *me = static_cast<QMouseEvent *>(e);
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
  return false;
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
 * Set new minimum and maximum values
 */
void XIntegrationScrollBar::set(double minimum, double maximum) {
  if (minimum < 0 || minimum > 1. || maximum < 0 || maximum > 1.) {
    throw std::invalid_argument(
        "XIntegrationScrollBar : minimum and maximum must be between 0 and 1");
  }
  if (minimum > maximum) {
    std::swap(minimum, maximum);
  }
  m_minimum = minimum;
  m_maximum = maximum;
  int x = static_cast<int>(m_minimum * this->width());
  int w = static_cast<int>((m_maximum - m_minimum) * this->width());
  if (w <= 2 * m_resizeMargin) {
    w = 2 * m_resizeMargin + 1;
  }
  m_slider->move(x, 0);
  m_slider->resize(w, this->height());
}

void XIntegrationScrollBar::updateMinMax() {
  m_minimum = double(m_slider->x()) / this->width();
  m_maximum = m_minimum + double(m_slider->width()) / this->width();
  emit running(m_minimum, m_maximum);
}

//---------------------------------------------------------------------------------//

XIntegrationControl::XIntegrationControl(InstrumentWidget *instrWindow)
    : QFrame(instrWindow), m_instrWindow(instrWindow), m_totalMinimum(0),
      m_totalMaximum(1), m_minimum(0), m_maximum(1) {
  m_scrollBar = new XIntegrationScrollBar(this);
  QHBoxLayout *layout = new QHBoxLayout();
  m_minText = new QLineEdit(this);
  m_minText->setMaximumWidth(100);
  m_minText->setToolTip("Minimum x value");
  m_maxText = new QLineEdit(this);
  m_maxText->setMaximumWidth(100);
  m_maxText->setToolTip("Maximum x value");
  m_units = new QLabel("TOF", this);
  m_setWholeRange = new QPushButton("Reset");
  m_setWholeRange->setToolTip("Reset integration range to maximum");

  layout->addWidget(m_units, 0);
  layout->addWidget(m_minText, 0);
  layout->addWidget(m_scrollBar, 1);
  layout->addWidget(m_maxText, 0);
  layout->addWidget(m_setWholeRange, 0);
  setLayout(layout);
  connect(m_scrollBar, SIGNAL(changed(double, double)), this,
          SLOT(sliderChanged(double, double)));
  connect(m_scrollBar, SIGNAL(running(double, double)), this,
          SLOT(sliderRunning(double, double)));
  connect(m_minText, SIGNAL(editingFinished()), this, SLOT(setMinimum()));
  connect(m_maxText, SIGNAL(editingFinished()), this, SLOT(setMaximum()));
  connect(m_setWholeRange, SIGNAL(clicked()), this, SLOT(setWholeRange()));
  updateTextBoxes();
}

void XIntegrationControl::sliderChanged(double minimum, double maximum) {
  double w = m_totalMaximum - m_totalMinimum;
  m_minimum = m_totalMinimum + minimum * w;
  m_maximum = m_totalMinimum + maximum * w;
  if (w > 0 && (m_maximum - m_minimum) / w >= 0.98) {
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

void XIntegrationControl::setTotalRange(double minimum, double maximum) {
  if (minimum > maximum) {
    std::swap(minimum, maximum);
  }
  m_totalMinimum = minimum;
  m_totalMaximum = maximum;
  m_minimum = minimum;
  m_maximum = maximum;
  updateTextBoxes();
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
  m_scrollBar->set((m_minimum - m_totalMinimum) / w,
                   (m_maximum - m_totalMinimum) / w);
  updateTextBoxes();
  emit changed(m_minimum, m_maximum);
}

void XIntegrationControl::setWholeRange() {
  setRange(m_totalMinimum, m_totalMaximum);
}

double XIntegrationControl::getMinimum() const { return m_minimum; }

double XIntegrationControl::getMaximum() const { return m_maximum; }

double XIntegrationControl::getWidth() const { return m_maximum - m_minimum; }

void XIntegrationControl::updateTextBoxes() {
  m_minText->setText(QString::number(m_minimum));
  m_maxText->setText(QString::number(m_maximum));
  m_setWholeRange->setEnabled(m_minimum != m_totalMinimum ||
                              m_maximum != m_totalMaximum);
}

void XIntegrationControl::setMinimum() {
  bool ok;
  QString text = m_minText->text();
  double minValue = text.toDouble(&ok);
  if (!ok)
    return;
  double maxValue = getMaximum();
  setRange(minValue, maxValue);
}

void XIntegrationControl::setMaximum() {
  bool ok;
  QString text = m_maxText->text();
  double maxValue = text.toDouble(&ok);
  if (!ok)
    return;
  double minValue = getMinimum();
  setRange(minValue, maxValue);
}

void XIntegrationControl::setUnits(const QString &units) {
  m_units->setText(units);
}
} // namespace MantidWidgets
} // namespace MantidQt
