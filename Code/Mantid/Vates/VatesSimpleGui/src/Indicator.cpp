/*
 * indicator.cpp
 *
 *  Created on: Apr 7, 2011
 *      Author: 2zr
 */

#include "indicator.h"

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QRect>

#include <iostream>
#include <vector>

Indicator::Indicator(QGraphicsItem *parent) : QGraphicsPolygonItem(parent)
{
	this->fillColor = Qt::blue;
	this->outlineColor = Qt::black;
	this->half_base = 5;
	this->setOpacity(1.0);
	this->setBrush(QBrush(this->fillColor));
	this->setPen(QPen(this->outlineColor));
	this->setFlags(QGraphicsItem::ItemIsMovable |
			QGraphicsItem::ItemIsSelectable);
}

void Indicator::setPoints(const QPoint &eloc, const QRect &rect)
{
	int half_width = rect.width() / 2;
	path << QPoint(-half_width, 0);
	path << QPointF(half_width, this->half_base);
	path << QPointF(half_width, -this->half_base);
	// Close the polygon
	path << QPointF(-half_width, 0);
	this->setPolygon(path);
	double height_loc = eloc.y() + 2 * this->half_base;
	this->left_edge = rect.left() + half_width;
	this->setPos(QPointF(this->left_edge, height_loc));
}

void Indicator::printSelf()
{
	QPolygonF poly = this->polygon();
	int psize = poly.size();
	if (poly.isClosed())
	{
		--psize;
	}
	for(int i = 0; i < psize; i++)
	{
		std::cout << "Point " << i << ": " << poly.at(i).x();
		std::cout << ", " << poly.at(i).y() << std::endl;
	}
}

int Indicator::fixVerticalPos(int ylevel)
{
	return ylevel - this->half_base / 2;
}

void Indicator::updatePos(const QPoint &pos)
{
	// Not sure why the y position needs to have this particular offset
	// but it seems to work best.
	this->setPos(this->left_edge, pos.y() + this->half_base * 1.5);
}

void Indicator::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF pos = this->mapToScene(event->pos());
	this->setPos(this->left_edge, this->fixVerticalPos(pos.y()));
}

void Indicator::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	switch (event->button())
	{
	case Qt::LeftButton:
	{
		this->setSelected(false);
		break;
	}
	default:
		QGraphicsPolygonItem::mouseReleaseEvent(event);
		break;
	}
}
