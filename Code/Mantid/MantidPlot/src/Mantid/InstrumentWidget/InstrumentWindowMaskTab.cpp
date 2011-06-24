#include "InstrumentWindow.h"
#include "InstrumentWindowMaskTab.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>

#include <numeric>
#include <cfloat>
#include <algorithm>

InstrumentWindowMaskTab::InstrumentWindowMaskTab(InstrumentWindow* instrWindow):
QFrame(instrWindow),
m_instrumentWindow(instrWindow),
m_activity(Select),
m_userEditing(true)
{
  m_instrumentDisplay = m_instrumentWindow->getInstrumentDisplay();

  // main layout
  QVBoxLayout* layout=new QVBoxLayout(this);

  // Create the tool buttons

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

  // Create property browser

    /* Create property managers: they create, own properties, get and set values  */

  m_groupManager = new QtGroupPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  connect(m_doubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(doubleChanged(QtProperty*)));

     /* Create editors and assign them to the managers */

  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  
  layout->addWidget(m_browser);

  // Algorithm buttons

  m_apply = new QPushButton("Apply");
  connect(m_apply,SIGNAL(clicked()),this,SLOT(applyMask()));

  m_clear_all = new QPushButton("Clear All");
  connect(m_clear_all,SIGNAL(clicked()),this,SLOT(clearMask()));

  m_save_as_workspace = new QPushButton("Save As Workspace");

  QGridLayout* buttons = new QGridLayout();
  buttons->addWidget(m_apply,0,0);
  buttons->addWidget(m_clear_all,0,1);
  buttons->addWidget(m_save_as_workspace,1,0,1,2);
  
  layout->addLayout(buttons);
}

void InstrumentWindowMaskTab::init()
{
  connect(m_instrumentDisplay->getSurface(),SIGNAL(shapeCreated()),this,SLOT(shapeCreated()));
  connect(m_instrumentDisplay->getSurface(),SIGNAL(shapeSelected()),this,SLOT(shapeSelected()));
  connect(m_instrumentDisplay->getSurface(),SIGNAL(shapesDeselected()),this,SLOT(shapesDeselected()));
  connect(m_instrumentDisplay->getSurface(),SIGNAL(shapeChanged()),this,SLOT(shapeChanged()));
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
}

void InstrumentWindowMaskTab::shapeSelected()
{
  setProperties();
}

void InstrumentWindowMaskTab::shapesDeselected()
{
  clearProperties();
}

void InstrumentWindowMaskTab::shapeChanged()
{
  if (!m_left) return; // check that everything is ok
  m_userEditing = false; // this prevents resetting shape proeprties by doubleChanged(...)
  QRectF rect = m_instrumentDisplay->getSurface()->getCurrentBoundingRect();
  m_doubleManager->setValue(m_left,rect.left());
  m_doubleManager->setValue(m_top,rect.top());
  m_doubleManager->setValue(m_right,rect.right());
  m_doubleManager->setValue(m_bottom,rect.bottom());
  for(QMap<QtProperty *,QString>::iterator it = m_doublePropertyMap.begin(); it != m_doublePropertyMap.end(); ++it)
  {
    m_doubleManager->setValue(it.key(),m_instrumentDisplay->getSurface()->getCurrentDouble(it.value()));
  }
  for(QMap<QString,QtProperty *>::iterator it = m_pointPropertyMap.begin(); it != m_pointPropertyMap.end(); ++it)
  {
    QtProperty* prop = it.value();
    QList<QtProperty*> subs = prop->subProperties();
    if (subs.size() != 2) continue;
    QPointF p = m_instrumentDisplay->getSurface()->getCurrentPoint(it.key());
    m_doubleManager->setValue(subs[0],p.x());
    m_doubleManager->setValue(subs[1],p.y());
  }
  m_userEditing = true;
}

void InstrumentWindowMaskTab::showEvent (QShowEvent *)
{
  setActivity();
  m_instrumentDisplay->setMouseTracking(true);
}

void InstrumentWindowMaskTab::clearProperties()
{
  m_browser->clear();
  m_doublePropertyMap.clear();
  m_pointPropertyMap.clear();
  m_pointComponentsMap.clear();
  m_left = NULL;
  m_top = NULL;
  m_right = NULL;
  m_bottom = NULL;
}

void InstrumentWindowMaskTab::setProperties()
{
  clearProperties();
  m_userEditing = false;

  // bounding rect property
  QtProperty* boundingRectGroup = m_groupManager->addProperty("Bounging Rect");
  m_browser->addProperty(boundingRectGroup);
  m_left = m_doubleManager->addProperty("left");
  m_top = m_doubleManager->addProperty("top");
  m_right = m_doubleManager->addProperty("right");
  m_bottom = m_doubleManager->addProperty("bottom");
  boundingRectGroup->addSubProperty(m_left);
  boundingRectGroup->addSubProperty(m_top);
  boundingRectGroup->addSubProperty(m_right);
  boundingRectGroup->addSubProperty(m_bottom);

  // point properties
  QStringList pointProperties = m_instrumentDisplay->getSurface()->getCurrentPointNames();
  foreach(QString name,pointProperties)
  {
    QtProperty* point = m_groupManager->addProperty(name);
    QtProperty* prop_x = m_doubleManager->addProperty("x");
    QtProperty* prop_y = m_doubleManager->addProperty("y");
    point->addSubProperty(prop_x);
    point->addSubProperty(prop_y);
    m_browser->addProperty(point);
    m_pointComponentsMap[prop_x] = name;
    m_pointComponentsMap[prop_y] = name;
    m_pointPropertyMap[name] = point;
  }

  // double properties
  QStringList doubleProperties = m_instrumentDisplay->getSurface()->getCurrentDoubleNames();
  foreach(QString name,doubleProperties)
  {
    QtProperty* prop = m_doubleManager->addProperty(name);
    m_browser->addProperty(prop);
    m_doublePropertyMap[prop] = name;
  }

  shapeChanged();
}

void InstrumentWindowMaskTab::doubleChanged(QtProperty* prop)
{
  if (!m_userEditing) return;
  if (prop == m_left || prop == m_top || prop == m_right || prop == m_bottom)
  {
    QRectF rect(QPointF(m_doubleManager->value(m_left),m_doubleManager->value(m_top)),
                QPointF(m_doubleManager->value(m_right),m_doubleManager->value(m_bottom)));
    m_instrumentDisplay->getSurface()->setCurrentBoundingRect(rect);
  }
  else
  {
    QString name = m_doublePropertyMap[prop];
    if (!name.isEmpty())
    {
      m_instrumentDisplay->getSurface()->setCurrentDouble(name,m_doubleManager->value(prop));
    }
    else
    {
      name = m_pointComponentsMap[prop];
      if (!name.isEmpty())
      {
        QtProperty* point_prop = m_pointPropertyMap[name];
        QList<QtProperty*> subs = point_prop->subProperties();
        if (subs.size() != 2) return;
        QPointF p(m_doubleManager->value(subs[0]),m_doubleManager->value(subs[1]));
        m_instrumentDisplay->getSurface()->setCurrentPoint(name,p);
      }
    }
  }
  m_instrumentDisplay->update();
}

void InstrumentWindowMaskTab::applyMask()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_pointer->setChecked(true);
  setActivity();
  m_instrumentDisplay->repaint(); // to refresh the pick image

  QList<int> dets;
  m_instrumentDisplay->getSurface()->getMaskedDetectors(dets);
  if (!dets.isEmpty())
  {
    QStringList detList;
    foreach(int id,dets)
    {
      detList << QString::number(m_instrumentWindow->getInstrumentActor()->getWorkspaceIndex(id));
    }
    QString param_list = "Workspace=%1;WorkspaceIndexList=%2";
    param_list = param_list.arg(m_instrumentWindow->getWorkspaceName(),detList.join(","));
    emit executeAlgorithm("MaskDetectors",param_list);
  }
  clearMask();
  QApplication::restoreOverrideCursor();
}

void InstrumentWindowMaskTab::clearMask()
{
  m_instrumentDisplay->getSurface()->clearMask();
  m_instrumentDisplay->update();
}
