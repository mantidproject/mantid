#include "AxisInteractor.h"
#include "Indicator.h"
#include "ScalePicker.h"

#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QList>
#include <QMouseEvent>
#include <QString>

#include <cmath>
#include <iostream>
AxisInteractor::AxisInteractor(QWidget *parent) : QWidget(parent)
{
	this->scene = new QGraphicsScene(this);
	this->scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	this->isSceneGeomInit = false;
	this->ui.setupUi(this);
	this->ui.graphicsView->setScene(this->scene);
	this->ui.graphicsView->installEventFilter(this);
	this->ui.scaleWidget->setAlignment(QwtScaleDraw::LeftScale);
	this->engine = new QwtLinearScaleEngine;
	this->transform = new QwtScaleTransformation(QwtScaleTransformation::Linear);
	this->scalePicker = new ScalePicker(this->ui.scaleWidget);
	QObject::connect(this->scalePicker, SIGNAL(makeIndicator(const QPoint &)),
			this, SLOT(createIndicator(const QPoint &)));
}

void AxisInteractor::setInformation(QString title, double min, double max)
{
	this->ui.scaleWidget->setTitle(title);
	this->ui.scaleWidget->setScaleDiv(this->transform,
			this->engine->divideScale(std::floor(min), std::ceil(max), 10, 0));
}

void AxisInteractor::createIndicator(const QPoint &point)
{
	QRect gv_rect = this->ui.graphicsView->geometry();
	if (! this->isSceneGeomInit)
	{
		this->scene->setSceneRect(gv_rect);
		this->isSceneGeomInit = true;
	}
	Indicator *tri = new Indicator();
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
	if (obj == this->ui.graphicsView)
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
