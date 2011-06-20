#include "InstrumentWindow.h"
#include "InstrumentWindowMaskTab.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QMessageBox>

#include <numeric>
#include <cfloat>
#include <algorithm>

InstrumentWindowMaskTab::InstrumentWindowMaskTab(InstrumentWindow* instrWindow):
QFrame(instrWindow),
m_instrumentWindow(instrWindow),
m_activity(Select)
{
  m_instrumentDisplay = m_instrumentWindow->getInstrumentDisplay();

  QVBoxLayout* layout=new QVBoxLayout(this);

  m_move = new QPushButton();
  m_move->setCheckable(true);
  m_move->setAutoExclusive(true);
  m_move->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_move->setToolTip("Move the instrument");

  m_pointer = new QPushButton();
  m_pointer->setCheckable(true);
  m_pointer->setAutoExclusive(true);
  m_pointer->setIcon(QIcon(":/MaskTools/selection-pointer.png"));
  m_pointer->setToolTip("Select and edit shapes");

  m_ellipse = new QPushButton();
  m_ellipse->setCheckable(true);
  m_ellipse->setAutoExclusive(true);
  m_ellipse->setIcon(QIcon(":/MaskTools/selection-circle.png"));
  m_ellipse->setToolTip("Draw an ellipse");

  m_rectangle = new QPushButton();
  m_rectangle->setCheckable(true);
  m_rectangle->setAutoExclusive(true);
  m_rectangle->setIcon(QIcon(":/MaskTools/selection-box.png"));
  m_rectangle->setToolTip("Draw a rectangle");

  m_ring_ellipse = new QPushButton();
  m_ring_ellipse->setCheckable(true);
  m_ring_ellipse->setAutoExclusive(true);
  m_ring_ellipse->setIcon(QIcon(":/MaskTools/selection-circle.png"));
  m_ring_ellipse->setToolTip("Draw an ellipse");

  m_ring_rectangle = new QPushButton();
  m_ring_rectangle->setCheckable(true);
  m_ring_rectangle->setAutoExclusive(true);
  m_ring_rectangle->setIcon(QIcon(":/MaskTools/selection-box.png"));
  m_ring_rectangle->setToolTip("Draw a rectangle");

  QHBoxLayout* toolBox = new QHBoxLayout();
  toolBox->addWidget(m_move);
  toolBox->addWidget(m_pointer);
  toolBox->addWidget(m_ellipse);
  toolBox->addWidget(m_rectangle);
  toolBox->addWidget(m_ring_ellipse);
  toolBox->addWidget(m_ring_rectangle);
  toolBox->addStretch();
  toolBox->setSpacing(2);

  connect(m_move,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_pointer,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ellipse,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_rectangle,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ring_ellipse,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ring_rectangle,SIGNAL(clicked()),this,SLOT(setActivity()));
  m_move->setChecked(true);

  layout->addLayout(toolBox);
  layout->addStretch();
}

void InstrumentWindowMaskTab::init()
{
  connect(m_instrumentDisplay->getSurface(),SIGNAL(shapeCreated()),this,SLOT(shapeCreated()));
}

void InstrumentWindowMaskTab::setActivity()
{
  const QColor borderColor = Qt::red;
  const QColor fillColor = QColor(255,255,255,100);
  if (m_move->isChecked())
  {
    m_activity = Move;
    m_instrumentDisplay->getSurface()->setInteractionModeMove();
  }
  else if (m_pointer->isChecked())
  {
    m_activity = Select;
    m_instrumentDisplay->getSurface()->setInteractionModeDraw();
  }
  else if (m_ellipse->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentDisplay->getSurface()->startCreatingShape2D("ellipse",borderColor,fillColor);
    m_instrumentDisplay->getSurface()->setInteractionModeDraw();
  }
  else if (m_rectangle->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentDisplay->getSurface()->startCreatingShape2D("rectangle",borderColor,fillColor);
    m_instrumentDisplay->getSurface()->setInteractionModeDraw();
  }
  else if (m_ring_ellipse->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentDisplay->getSurface()->startCreatingShape2D("ring ellipse",borderColor,fillColor);
    m_instrumentDisplay->getSurface()->setInteractionModeDraw();
  }
  else if (m_ring_rectangle->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentDisplay->getSurface()->startCreatingShape2D("ring rectangle",borderColor,fillColor);
    m_instrumentDisplay->getSurface()->setInteractionModeDraw();
  }
}

void InstrumentWindowMaskTab::shapeCreated()
{
  m_pointer->setChecked(true);
  setActivity();
}

void InstrumentWindowMaskTab::showEvent (QShowEvent *)
{
  setActivity();
  m_instrumentDisplay->setMouseTracking(true);
}
