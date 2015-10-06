#include "CollapsiblePanel.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QPolygon>

#include <stdexcept>
#include <iostream>

CaptionLabel::CaptionLabel(const QString& caption,QWidget* parent):QLabel(caption,parent),m_collapsed(false)
{
  setFrameStyle(QFrame::WinPanel);
  setFrameShadow(QFrame::Raised);
}

void CaptionLabel::mousePressEvent(QMouseEvent* e)
{
  if (e->buttons() & Qt::LeftButton)
  {
    e->accept();
    m_collapsed = !m_collapsed;
    emit collapseOrExpand(m_collapsed);
  }
  else
  {
    e->ignore();
  }
}

void CaptionLabel::paintEvent(QPaintEvent *e)
{
  QLabel::paintEvent(e);
  QPainter painter(this);
  QFontMetrics fm(this->font());
  int h = fm.height() - 4;
  h = h / 2 * 2; // h is even
  if (h > 0)
  {
    int w = h / 2;
    int x = this->width() - 2*h;
    int y = (this->height() - h) / 2;
    QPolygon tri(3);
    if (m_collapsed)
    {
      tri.setPoint(0, x, y);
      tri.setPoint(1, x + w + w ,y);
      tri.setPoint(2, x + w, y + h);
    }
    else
    {
      tri.setPoint(0, x, y + h);
      tri.setPoint(1, x + w + w, y + h);
      tri.setPoint(2, x + w, y);
    }
    painter.setBrush(QBrush(QColor(Qt::black)));
    painter.drawPolygon(tri);
  }
}

void CaptionLabel::collapse()
{
    m_collapsed = true;
    emit collapseOrExpand(m_collapsed);
}

void CaptionLabel::expand()
{
    m_collapsed = false;
    emit collapseOrExpand(m_collapsed);
}

CollapsiblePanel::CollapsiblePanel(const QString& caption,QWidget* parent):
QWidget(parent),m_widget(NULL)
{
  m_layout = new QVBoxLayout(this);
  m_label = new CaptionLabel(caption,this);
  m_layout->addWidget(m_label);
  m_layout->setMargin(0);
  connect(m_label,SIGNAL(collapseOrExpand(bool)),this,SLOT(collapseOrExpand(bool)));
}

void CollapsiblePanel::setWidget(QWidget* widget)
{
  if (m_widget)
  {
    throw std::runtime_error("CollapsiblePanel already has a widget");
  }
  m_widget = widget;
  m_widget->setParent(this);
  m_layout->addWidget(m_widget);
}

void CollapsiblePanel::setCaption(const QString& caption)
{
  m_label->setText(caption);
}

void CollapsiblePanel::collapseOrExpand(bool collapse)
{
  if (!m_widget) return;
  if (collapse)
  {
    m_widget->hide();
    emit collapsed();
  }
  else
  {
    m_widget->show();
    emit expanded();
  }
}

bool CollapsiblePanel::isCollapsed()const
{
  return m_label->isCollapsed();
}

void CollapsiblePanel::collapse()
{
  collapseOrExpand(true);
}

void CollapsiblePanel::expand()
{
  collapseOrExpand(false);
}

CollapsibleStack::CollapsibleStack(QWidget* parent):
QWidget(parent)
{
  m_layout = new QVBoxLayout(this);
  m_layout->setMargin(0);
  setLayout(m_layout);
}

/**
  * Add a new panel to the bottom of the stack and set its caption and the inner widget
  */
CollapsiblePanel* CollapsibleStack::addPanel(const QString& caption,QWidget* widget)
{
  CollapsiblePanel *panel = new CollapsiblePanel(caption,this);
  panel->setWidget(widget);
  m_layout->addWidget(panel);
  connect(panel,SIGNAL(collapsed()),this,SLOT(updateStretch()));
  connect(panel,SIGNAL(expanded()),this,SLOT(updateStretch()));
  return panel;
}

bool CollapsibleStack::allCollapsed()const
{
  int n = m_layout->count();
  for(int i = 0; i < n; ++i)
  {
    CollapsiblePanel* panel = dynamic_cast<CollapsiblePanel*>(m_layout->itemAt(i)->widget());
    if (panel && !panel->isCollapsed())
    {
      return false;
    }
  }
  return true;
}

void CollapsibleStack::updateStretch()
{
  int i = m_layout->count();
  if (i == 0) return;
  --i; // point to the last item
  CollapsiblePanel* panel = dynamic_cast<CollapsiblePanel*>(m_layout->itemAt(i)->widget());
  if (allCollapsed())
  {// make sure that the last item is a stretch
    if (panel) // if it's a panel there is no stretch
    {
      m_layout->addStretch();
    }
  }
  else
  {
    if (!panel) // then it must be a stretch
    {
      m_layout->removeItem(m_layout->itemAt(i));
    }
  }
}
