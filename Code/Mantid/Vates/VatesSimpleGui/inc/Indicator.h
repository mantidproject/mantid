/*
 * indicator.h
 *
 *  Created on: Apr 7, 2011
 *      Author: 2zr
 */

#ifndef INDICATOR_H_
#define INDICATOR_H_

#include <QGraphicsItem>

class QColor;
class QGraphicsSceneMouseEvent;
class QPoint;
class QPolygonF;
class QRect;

const int IndicatorItemType = QGraphicsItem::UserType + 1;

class Indicator: public QGraphicsPolygonItem
{
public:
	enum {Type = IndicatorItemType};

	Indicator(QGraphicsItem *parent = 0);
	virtual ~Indicator() {}
	void printSelf();
	void setPoints(const QPoint &eloc, const QRect &rect);
	int type() const { return Type; }
	void updatePos(const QPoint &pos);

protected:
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
	int fixVerticalPos(int ylevel);

	QColor fillColor;
	QColor outlineColor;
	QPolygonF path;
	int half_base;
	int left_edge;
};

#endif /* INDICATOR_H_ */
