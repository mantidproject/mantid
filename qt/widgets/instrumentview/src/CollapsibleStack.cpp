// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/CollapsibleStack.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QSizePolicy>

namespace MantidQt::MantidWidgets {

CollapsiblePanelLabel::CollapsiblePanelLabel(const QString &caption, CollapsiblePanel *parent)
    : QLabel(caption, dynamic_cast<QWidget *>(parent)), m_parentPanel(parent) {
  setFrameStyle(QFrame::WinPanel);
  setFrameShadow(QFrame::Raised);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void CollapsiblePanelLabel::mousePressEvent(QMouseEvent *e) {
  if (e->buttons() & Qt::LeftButton) {
    emit collapseOrExpand();
  } else {
    e->ignore();
  }
}

void CollapsiblePanelLabel::paintEvent(QPaintEvent *e) {
  QLabel::paintEvent(e);
  QPainter painter(this);
  QFontMetrics fm(font());
  int h = fm.height() - 4;
  h = h / 2 * 2; // h is even
  if (h > 0) {
    int w = h / 2;
    int x = this->width() - 2 * h;
    int y = (this->height() - h) / 2;
    QPolygon tri(3);
    if (m_parentPanel->isCollapsed()) {
      tri.setPoint(0, x, y);
      tri.setPoint(1, x + w + w, y);
      tri.setPoint(2, x + w, y + h);
    } else {
      tri.setPoint(0, x, y + h);
      tri.setPoint(1, x + w + w, y + h);
      tri.setPoint(2, x + w, y);
    }
    painter.setBrush(QBrush(QColor(Qt::black)));
    painter.drawPolygon(tri);
  }
}

CollapsiblePanel::CollapsiblePanel(const QString &caption, QWidget *parent)
    : QWidget(parent), m_widget(nullptr), m_isCollapsed(false), m_isFixed(false), m_maxHeight(QWIDGETSIZE_MAX) {
  m_layout = new QVBoxLayout(this);
  m_label = new CollapsiblePanelLabel(caption, this);
  m_layout->addWidget(m_label);
  m_layout->setMargin(0);
  setLayout(m_layout);
  connect(m_label, SIGNAL(collapseOrExpand()), this, SLOT(collapsedOrExpanded()));
}

void CollapsiblePanel::setWidget(QWidget *widget, const bool fixedHeight) {
  m_widget = widget;
  m_widget->setParent(this);
  m_layout->addWidget(m_widget);
  if (fixedHeight) {
    m_isFixed = true;
    m_maxHeight = m_label->sizeHint().height() + m_widget->sizeHint().height();
  }
}

void CollapsiblePanel::setCaption(const QString &caption) { m_label->setText(caption); }

void CollapsiblePanel::collapsedOrExpanded() {
  if (m_isCollapsed) {
    expandCaption();
  } else {
    collapseCaption();
  }
}

bool CollapsiblePanel::isCollapsed() const { return m_isCollapsed; }

bool CollapsiblePanel::isFixed() const { return m_isFixed; }

void CollapsiblePanel::collapseCaption() {
  m_isCollapsed = true;
  emit collapsed();
}

void CollapsiblePanel::expandCaption() {
  m_isCollapsed = false;
  emit expanded();
}

QWidget *CollapsiblePanel::getWidget() const { return m_widget; }

CollapsiblePanelLabel *CollapsiblePanel::getLabel() const { return m_label; }

void CollapsiblePanel::setFixedHeight(const int height) { QWidget::setFixedHeight(std::min(height, m_maxHeight)); }

CollapsibleStack::CollapsibleStack(QWidget *parent) : QWidget(parent) {
  m_splitterLayout = new QSplitter(Qt::Vertical, this);
  m_splitterLayout->setContentsMargins(0, 0, 0, 0);
  m_splitterLayout->setChildrenCollapsible(false);

  m_baseLayout = new QVBoxLayout(this);
  m_baseLayout->addWidget(m_splitterLayout);
  setLayout(m_baseLayout);
}

/**
 * Add a new panel to the bottom of the stack
 * @param caption :: Caption text for the widget
 * @param widget :: Inner widget for the panel
 * @param fixedHeight :: Bool value, if true the widget will not expand and be fixed at the height of its size hint
 */
CollapsiblePanel *CollapsibleStack::addPanel(const QString &caption, QWidget *widget, const bool fixedHeight) {
  auto *panel = new CollapsiblePanel(caption, this);
  panel->setWidget(widget, fixedHeight);
  m_splitterLayout->addWidget(panel);

  connect(panel, SIGNAL(collapsed()), this, SLOT(panelCollapsed()));
  connect(panel, SIGNAL(expanded()), this, SLOT(panelExpanded()));
  return panel;
}

void CollapsibleStack::panelCollapsed() {
  auto *panel = dynamic_cast<CollapsiblePanel *>(sender());
  panel->getWidget()->hide();
  int labelHeight = panel->getLabel()->height();
  panel->setFixedHeight(labelHeight);
  updateStretch();
}

void CollapsibleStack::panelExpanded() {
  auto *panel = dynamic_cast<CollapsiblePanel *>(sender());
  panel->getWidget()->show();
  panel->setFixedHeight(QWIDGETSIZE_MAX); // If you set fixed height to QWIDGETSIZE_MAX it unfixes the height
  updateStretch();
}

bool CollapsibleStack::allCollapsedOrFixed() const {
  for (auto *p : m_splitterLayout->children()) {
    const auto *panel = dynamic_cast<CollapsiblePanel *>(p);
    if (panel && !panel->isCollapsed() && !panel->isFixed()) {
      return false;
    }
  }
  return true;
}

void CollapsibleStack::updateStretch() {
  int n = m_baseLayout->count();
  if (allCollapsedOrFixed()) {
    if (n == 1) {
      m_baseLayout->addStretch();
    }
  } else if (n == 2) { // n == 2 implies that a stretch has been added
    m_baseLayout->removeItem(m_baseLayout->itemAt(1));
  }
}
} // namespace MantidQt::MantidWidgets
