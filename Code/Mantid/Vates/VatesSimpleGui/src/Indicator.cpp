#include "Indicator.h"

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
	this->half_base = 10;
	this->orientation = AxisInteractor::LeftScale;
	this->setOpacity(1.0);
	this->setBrush(QBrush(this->fillColor));
	this->setPen(QPen(this->outlineColor));
	this->setFlags(QGraphicsItem::ItemIsMovable |
			QGraphicsItem::ItemIsSelectable);
}

void Indicator::setOrientation(AxisInteractor::ScalePos orient)
{
  this->orientation = orient;
}

void Indicator::setPoints(const QPoint &eloc, const QRect &rect)
{
  int half_triangle_height = 0;
  int p1_x, p1_y, p2_x, p2_y, p3_x, p3_y;
  p1_x = p1_y = p2_x = p2_y = p3_x = p3_y = 0;
  double apex_pos = 0.0;
  int pa_x, pa_y;
  pa_x = pa_y = 0;

  switch (this->orientation)
  {
  case AxisInteractor::LeftScale:
  case AxisInteractor::RightScale:
    half_triangle_height = rect.width() / 2;
    this->tip_edge = rect.left() + half_triangle_height;
    if (this->orientation == AxisInteractor::LeftScale)
    {
      p1_x = half_triangle_height;
      p2_x = -half_triangle_height;
      p3_x = -half_triangle_height;
    }
    if (this->orientation == AxisInteractor::RightScale)
    {
      p1_x = -half_triangle_height;
      p2_x = half_triangle_height;
      p3_x = half_triangle_height;
    }
    p1_y = 0;
    p2_y = this->half_base;
    p3_y = -this->half_base;
    apex_pos = eloc.y() + 2 * this->half_base;
    pa_x = this->tip_edge;
    pa_y = static_cast<int>(apex_pos);
    break;
  case AxisInteractor::TopScale:
  case AxisInteractor::BottomScale:
    half_triangle_height = rect.height() / 2;
    this->tip_edge = rect.top() + half_triangle_height;
    if (this->orientation == AxisInteractor::TopScale)
    {
      p1_y = half_triangle_height;
      p2_y = -half_triangle_height;
      p3_y = -half_triangle_height;
    }
    if (this->orientation == AxisInteractor::BottomScale)
    {
      p1_y = -half_triangle_height;
      p2_y = half_triangle_height;
      p3_y = half_triangle_height;
    }
    p1_x = 0;
    p2_x = this->half_base;
    p3_x = -this->half_base;
    apex_pos = eloc.x() + 2 * this->half_base;
    pa_x = static_cast<int>(apex_pos);
    pa_y = this->tip_edge;
    break;
  default:
    // If you get here, you have a big problem!
    break;
  }

  // Creating relative position triangle
  this->path << QPointF(p1_x, p1_y);
	this->path << QPointF(p2_x, p2_y);
	this->path << QPointF(p3_x, p3_y);
	// Close the polygon
	this->path << QPointF(p1_x, p1_y);
	this->setPolygon(path);
	// Set actual initial position
	this->setPos(QPointF(pa_x, pa_y));
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

int Indicator::fixPosition(int level)
{
  return level - this->half_base / 2;
}

void Indicator::updatePos(const QPoint &pos)
{
	// Not sure why the y position needs to have this particular offset
	// but it seems to work best.
  switch (this->orientation)
  {
  case AxisInteractor::LeftScale:
  case AxisInteractor::RightScale:
    this->setPos(this->tip_edge, pos.y() + this->half_base * 1.5);
    break;
  case AxisInteractor::TopScale:
  case AxisInteractor::BottomScale:
    this->setPos(pos.x() + this->half_base * 1.5, this->tip_edge);
    break;
  }
}

void Indicator::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF pos = this->mapToScene(event->pos());
  switch (this->orientation)
  {
  case AxisInteractor::LeftScale:
  case AxisInteractor::RightScale:
    this->setPos(this->tip_edge, this->fixPosition(static_cast<int>(pos.y())));
    break;
  case AxisInteractor::TopScale:
  case AxisInteractor::BottomScale:
    this->setPos(this->fixPosition(static_cast<int>(pos.x())), this->tip_edge);
    break;
  }
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
