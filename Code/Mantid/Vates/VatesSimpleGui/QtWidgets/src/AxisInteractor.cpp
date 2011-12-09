#include "MantidVatesSimpleGuiQtWidgets/AxisInteractor.h"
#include "MantidVatesSimpleGuiQtWidgets/AxisInformation.h"
#include "MantidVatesSimpleGuiQtWidgets/Indicator.h"
#include "MantidVatesSimpleGuiQtWidgets/ScalePicker.h"

#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

#include <QAction>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QString>
#include <QVBoxLayout>

#include <cmath>
#include <iostream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

AxisInteractor::AxisInteractor(QWidget *parent) : QWidget(parent)
{
  this->indicatorContextMenu = NULL;
  this->orientation = Qt::Vertical;
  this->scalePos = AxisInteractor::RightScale;

  this->setStyleSheet(QString::fromUtf8("QGraphicsView {background: transparent;}"));

  this->graphicsView = new QGraphicsView(this);
  this->graphicsView->setMouseTracking(true);
  this->graphicsView->setFrameShape(QFrame::NoFrame);
  this->graphicsView->setFrameShadow(QFrame::Plain);
  this->graphicsView->setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
  this->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  this->scaleWidget = new QwtScaleWidget(this);
  this->scaleWidget->setSpacing(0);
  this->scaleWidget->setMargin(0);
  this->scaleWidget->setColorBarWidth(0);
  this->scaleWidget->setPenWidth(1);

  this->scene = new QGraphicsScene(this);
  this->scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  this->isSceneGeomInit = false;
  this->graphicsView->setScene(this->scene);

  this->engine = new QwtLinearScaleEngine;
  this->transform = new QwtScaleTransformation(QwtScaleTransformation::Linear);
  this->scalePicker = new ScalePicker(this->scaleWidget);
  QObject::connect(this->scalePicker, SIGNAL(makeIndicator(const QPoint &)),
                   this, SLOT(createIndicator(const QPoint &)));

  QObject::connect(this->scene, SIGNAL(selectionChanged()), this,
                   SLOT(getIndicator()));

  this->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
                   this, SLOT(showContextMenu(const QPoint &)));
}

void AxisInteractor::widgetLayout()
{
  // All set for vertical orientation
  QSize scaleSize(80, 400);
  QSize gvSize(20, 400);
  QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

  if (this->orientation == Qt::Vertical)
  {
    this->boxLayout = new QHBoxLayout(this);
    this->scaleWidget->setFixedWidth(scaleSize.width());
    this->scaleWidget->setMinimumHeight(scaleSize.height());
    this->graphicsView->setFixedWidth(gvSize.width());
    this->graphicsView->setMinimumHeight(gvSize.height());
    switch (this->scalePos)
    {
    case LeftScale:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::RightScale);
      this->boxLayout->addWidget(this->graphicsView, 0);
      this->boxLayout->addWidget(this->scaleWidget, 1);
      break;
    }
    case RightScale:
    default:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::LeftScale);
      this->boxLayout->addWidget(this->scaleWidget, 0);
      this->boxLayout->addWidget(this->graphicsView, 1);
      break;
    }
    }
  }
  else // Qt::Horizontal
  {
    this->boxLayout = new QVBoxLayout(this);
    scaleSize.transpose();
    gvSize.transpose();
    policy.transpose();
    this->scaleWidget->setMinimumWidth(scaleSize.width());
    this->scaleWidget->setFixedHeight(scaleSize.height());
    this->graphicsView->setMinimumWidth(gvSize.width());
    this->graphicsView->setFixedHeight(gvSize.height());
    switch (this->scalePos)
    {
    case BottomScale:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::TopScale);
      this->boxLayout->addWidget(this->scaleWidget, 0);
      this->boxLayout->addWidget(this->graphicsView, 1);
      break;
    }
    case TopScale:
    default:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::BottomScale);
      this->boxLayout->addWidget(this->graphicsView, 0);
      this->boxLayout->addWidget(this->scaleWidget, 1);
      break;
    }
    }
  }
  this->boxLayout->setContentsMargins(0, 0, 0, 0);
  this->boxLayout->setSpacing(0);
  this->scaleWidget->setSizePolicy(policy);
  this->graphicsView->setSizePolicy(policy);
}

/**
 * This function is responsible for setting the scale widget with the incoming
 * information for the associated dataset axis. If an update or a change to the
 * axis is necessary, the QwtScaleTransformation must be passed as NULL to the
 * setScaleDiv call.
 * @param info the incoming axis information container
 * @param update flag to determine if scale just needs updating
 */
void AxisInteractor::setInformation(AxisInformation *info, bool update)
{
  this->setBounds(info, update);
  this->scaleWidget->setTitle(QString(info->getTitle().c_str()));
}

/**
 * This function is responsible for setting the upper and lower limit of the
 * axis scale from the incoming information. If an update or a change to the
 * axis is necessary, the QwtScaleTransformation must be passed as NULL to the
 * setScaleDiv call.
 * @param info the incoming axis information container
 * @param update flag to determine if scale just needs updating
 */
void AxisInteractor::setBounds(AxisInformation *info, bool update)
{
  QwtScaleTransformation *temp = this->transform;
  if (update)
  {
    temp = NULL;
  }
  this->scaleWidget->setScaleDiv(temp,
                                 this->engine->divideScale(info->getMinimum(),
                                                           info->getMaximum(),
                                                           10, 0));
}

void AxisInteractor::createIndicator(const QPoint &point)
{
  QRect gv_rect = this->graphicsView->geometry();
  if (! this->isSceneGeomInit)
  {
    this->scene->setSceneRect(gv_rect);
    this->isSceneGeomInit = true;
  }
  Indicator *tri = new Indicator();
  QObject::connect(tri, SIGNAL(indicatorMoved(const QPoint &, int)),
                   this->scalePicker,
                   SLOT(determinePosition(const QPoint &, int)));
  tri->setOrientation(this->scalePos);
  tri->setPoints(point, gv_rect);
  this->scene->addItem(tri);
}

void AxisInteractor::setIndicatorName(const QString &name)
{
  QList<QGraphicsItem *> list = this->scene->items();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      if (item->toolTip().isEmpty())
      {
        // This must be the most recently added
        item->setToolTip(name);
      }
    }
  }
}

void AxisInteractor::selectIndicator(const QString &name)
{
  this->clearSelections();
  QList<QGraphicsItem *> list = this->scene->items();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      if (item->toolTip() == name)
      {
        item->setSelected(true);
      }
    }
  }
}

bool AxisInteractor::hasIndicator()
{
  QList<QGraphicsItem *> list = this->scene->selectedItems();
  if (list.count() > 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void AxisInteractor::clearSelections()
{
  QList<QGraphicsItem *> list = this->scene->selectedItems();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      item->setSelected(false);
    }
  }
}

void AxisInteractor::updateIndicator(double value)
{
  QPoint *pos = this->scalePicker->getLocation(value);
  QList<QGraphicsItem *> list = this->scene->selectedItems();
  if (list.count() > 0)
  {
    Indicator *item = static_cast<Indicator *>(list.at(0));
    item->updatePos(*pos);
  }
}

AxisInteractor::ScalePos AxisInteractor::scalePosition() const
{
  return this->scalePos;
}

void AxisInteractor::setOrientation(Qt::Orientation orient, ScalePos scalePos)
{
  this->scalePos = scalePos;
  this->orientation = orient;
  this->widgetLayout();
}

void AxisInteractor::setScalePosition(ScalePos scalePos)
{
  if ((scalePos == BottomScale) || (scalePos == TopScale))
  {
    this->setOrientation(Qt::Horizontal, scalePos);
  }
  else if ((scalePos == LeftScale) || (scalePos == RightScale))
  {
    this->setOrientation(Qt::Vertical, scalePos);
  }
}

void AxisInteractor::getIndicator()
{
  QList<QGraphicsItem *> list = this->scene->selectedItems();
  if (1 == list.size())
  {
    QGraphicsItem *item = list.at(0);
    if (item->type() == IndicatorItemType)
    {
      emit this->indicatorSelected(item->toolTip());
    }
  }
}

void AxisInteractor::showContextMenu(const QPoint &pos)
{
  QPoint globalPos = this->mapToGlobal(pos);
  QList<QGraphicsItem *> list = this->scene->items();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      if (item->isUnderMouse())
      {
        item->setSelected(false);
        this->createContextMenu();
        QAction *selectedItem = this->indicatorContextMenu->exec(globalPos);
        if (selectedItem)
        {
          if (QString("Delete") == selectedItem->text())
          {
            this->scene->removeItem(item);
            emit this->deleteIndicator(item->toolTip());
          }
          if (QString("Hide") == selectedItem->text())
          {
            bool isVisible = !selectedItem->isChecked();
            emit this->showOrHideIndicator(isVisible, item->toolTip());
            static_cast<Indicator *>(item)->changeIndicatorColor(isVisible);
          }
        }
      }
    }
  }
}

void AxisInteractor::createContextMenu()
{
  if (this->indicatorContextMenu)
  {
    return;
  }
  this->indicatorContextMenu = new QMenu();
  QAction *hideAction = this->indicatorContextMenu->addAction("Hide");
  hideAction->setCheckable(true);
  this->indicatorContextMenu->addAction("Delete");
}

int AxisInteractor::numIndicators()
{
  int count = 0;
  QList<QGraphicsItem *> list = this->scene->items();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      count++;
    }
  }
  return count;
}

void AxisInteractor::deleteAllIndicators()
{
  QList<QGraphicsItem *> list = this->scene->items();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      this->scene->removeItem(item);
      emit this->deleteIndicator(item->toolTip());
    }
  }
}

/**
 * @return the axis scale title
 */
QString AxisInteractor::getTitle()
{
  return this->scaleWidget->title().text();
}

/**
 * @return the axis scale maximum
 */
double AxisInteractor::getMaximum()
{
#if QWT_VERSION >= 0x050200
  return this->scaleWidget->scaleDraw()->scaleDiv().upperBound();
#else
  return this->scaleWidget->scaleDraw()->scaleDiv().hBound();
#endif
}

/**
 * @return the axis scale minimum
 */
double AxisInteractor::getMinimum()
{
#if QWT_VERSION >= 0x050200
  return this->scaleWidget->scaleDraw()->scaleDiv().lowerBound();
#else
  return this->scaleWidget->scaleDraw()->scaleDiv().lBound();
#endif
}

/**
 * This function searches the list of indicators and deletes the requested
 * one.
 * @param name the requested indicator to delete
 */
void AxisInteractor::deleteRequestedIndicator(const QString &name)
{
  QList<QGraphicsItem *> list = this->scene->items();
  for (int i = 0; i < list.count(); ++i)
  {
    QGraphicsItem *item = list.at(i);
    if (item->type() == IndicatorItemType)
    {
      if (item->toolTip() == name)
      {
        this->scene->removeItem(item);
        emit this->deleteIndicator(item->toolTip());
      }
    }
  }
}

}
}
}
