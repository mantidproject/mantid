#include "AxisInteractor.h"
#include "Indicator.h"
#include "ScalePicker.h"

#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

#include <QAction>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QGridLayout>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QString>

#include <cmath>
#include <iostream>
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

  this->gridLayout = new QGridLayout(this);
  this->scaleWidget = new QwtScaleWidget(this);

	this->scene = new QGraphicsScene(this);
	this->scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	this->isSceneGeomInit = false;

	//this->widgetLayout();

	this->graphicsView->setScene(this->scene);
  //this->graphicsView->installEventFilter(this);

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
  if (!this->gridLayout->isEmpty())
  {
    for (int i = 0; i < this->gridLayout->count(); ++i)
    {
      this->gridLayout->removeItem(this->gridLayout->itemAt(i));
    }
  }

  // All set for vertical orientation
  int scaleWidth = 50;
  int scaleHeight = 150;
  int gvWidth = 50;
  int gvHeight = 150;
  QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Minimum);

  if (this->orientation == Qt::Vertical)
  {
    switch (this->scalePos)
    {
    case LeftScale:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::RightScale);
      this->gridLayout->addWidget(this->graphicsView, 0, 0, 1, 1);
      this->gridLayout->addWidget(this->scaleWidget, 0, 1, 1, 1);
      break;
    }
    case RightScale:
    default:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::LeftScale);
      this->gridLayout->addWidget(this->scaleWidget, 0, 0, 1, 1);
      this->gridLayout->addWidget(this->graphicsView, 0, 1, 1, 1);
      break;
    }
    }
  }
  else // Qt::Horizontal
  {
    qSwap(scaleWidth, scaleHeight);
    qSwap(gvWidth, gvHeight);
    policy.transpose();
    switch (this->scalePos)
    {
    case BottomScale:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::TopScale);
      this->gridLayout->addWidget(this->scaleWidget, 0, 0, 1, 1);
      this->gridLayout->addWidget(this->graphicsView, 1, 0, 1, 1);
      break;
    }
    case TopScale:
    default:
    {
      this->scaleWidget->setAlignment(QwtScaleDraw::BottomScale);
      this->gridLayout->addWidget(this->graphicsView, 0, 0, 1, 1);
      this->gridLayout->addWidget(this->scaleWidget, 1, 0, 1, 1);
      break;
    }
    }
  }
  this->scaleWidget->setSizePolicy(policy);
  this->scaleWidget->setMinimumSize(QSize(scaleWidth, scaleHeight));
  this->graphicsView->setSizePolicy(policy);
  this->graphicsView->setMinimumSize(QSize(gvWidth, gvHeight));
  this->setSizePolicy(policy);
}

void AxisInteractor::setInformation(QString title, double min, double max)
{
	this->scaleWidget->setTitle(title);
	this->scaleWidget->setScaleDiv(this->transform,
			this->engine->divideScale(std::floor(min), std::ceil(max), 10, 0));
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
        this->scalePicker, SLOT(determinePosition(const QPoint &, int)));
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

bool AxisInteractor::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == this->graphicsView)
	{
		if (event->type() == QEvent::MouseButtonPress ||
				event->type() == QEvent::MouseButtonDblClick)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			if (mouseEvent->button() == Qt::RightButton)
			{
				// Want to eat these so users don't add the indicators
				// via the QGraphicsView (Yum!)
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return AxisInteractor::eventFilter(obj, event);
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
            emit this->deleteIndicator(item->toolTip());
            this->scene->removeItem(item);
          }
          if (QString("Hide") == selectedItem->text())
          {

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
