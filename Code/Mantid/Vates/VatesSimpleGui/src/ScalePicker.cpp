/*
 * scalepicker.cpp
 *
 *  Created on: Apr 12, 2011
 *      Author: 2zr
 */

#include "ScalePicker.h"

#include <qwt_scale_draw.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

#include <QEvent>
#include <QMouseEvent>
#include <QRect>

#include <iostream>
ScalePicker::ScalePicker(QwtScaleWidget *scale) : QObject(scale)
{
	((QwtScaleWidget *)this->parent())->installEventFilter(this);
}

bool ScalePicker::eventFilter(QObject *object, QEvent *e)
{
    if ( object->inherits("QwtScaleWidget") &&
        e->type() == QEvent::MouseButtonPress )
    {
        mouseClicked((const QwtScaleWidget *)object,
            ((QMouseEvent *)e)->pos());
        return true;
    }

    return QObject::eventFilter(object, e);
}

void ScalePicker::mouseClicked(const QwtScaleWidget *scale, const QPoint &pos)
{
    QRect rect = this->scaleRect(scale);

    int margin = 10; // 10 pixels tolerance
    rect.setRect(rect.x() - margin, rect.y() - margin,
        rect.width() + 2 * margin, rect.height() +  2 * margin);

    if ( rect.contains(pos) ) // No click on the title
    {
        // translate the position in a value on the scale

        double value = 0.0;

        const QwtScaleDraw *sd = scale->scaleDraw();
        switch(scale->alignment())
        {
            case QwtScaleDraw::LeftScale:
            {
                value = sd->map().invTransform(pos.y());
                break;
            }
            case QwtScaleDraw::RightScale:
            {
                value = sd->map().invTransform(pos.y());
                break;
            }
            case QwtScaleDraw::BottomScale:
            {
                value = sd->map().invTransform(pos.x());
                break;
            }
            case QwtScaleDraw::TopScale:
            {
                value = sd->map().invTransform(pos.x());
                break;
            }
        }
        emit makeIndicator(pos);
        emit clicked(value);
    }
}

QPoint *ScalePicker::getLocation(double axisval)
{
	QwtScaleWidget *scale = static_cast<QwtScaleWidget *>(this->parent());
	const QwtScaleDraw *sd = scale->scaleDraw();

    QRect rect = this->scaleRect(scale);

    int margin = 10; // 10 pixels tolerance
    rect.setRect(rect.x() - margin, rect.y() - margin,
        rect.width() + 2 * margin, rect.height() +  2 * margin);

    int point = sd->map().transform(axisval);
    QPoint *pos = new QPoint();

    switch(scale->alignment())
    {
        case QwtScaleDraw::LeftScale:
        {
            pos->setX(rect.x());
            pos->setY(point);
            break;
        }
        case QwtScaleDraw::RightScale:
        {
            pos->setX(rect.x());
            pos->setY(point);
            break;
        }
        case QwtScaleDraw::BottomScale:
        {
            pos->setX(point);
            pos->setY(rect.y());
            break;
        }
        case QwtScaleDraw::TopScale:
        {
            pos->setX(point);
            pos->setY(rect.y());
            break;
        }
    }
    return pos;
}

// The rect of a scale without the title
QRect ScalePicker::scaleRect(const QwtScaleWidget *scale) const
{
    const int bld = scale->margin();
    const int mjt = scale->scaleDraw()->majTickLength();
    const int sbd = scale->startBorderDist();
    const int ebd = scale->endBorderDist();

    QRect rect;
    switch(scale->alignment())
    {
        case QwtScaleDraw::LeftScale:
        {
            rect.setRect(scale->width() - bld - mjt, sbd,
                mjt, scale->height() - sbd - ebd);
            break;
        }
        case QwtScaleDraw::RightScale:
        {
            rect.setRect(bld, sbd,
                mjt, scale->height() - sbd - ebd);
            break;
        }
        case QwtScaleDraw::BottomScale:
        {
            rect.setRect(sbd, bld,
                scale->width() - sbd - ebd, mjt);
            break;
        }
        case QwtScaleDraw::TopScale:
        {
            rect.setRect(sbd, scale->height() - bld - mjt,
                scale->width() - sbd - ebd, mjt);
            break;
        }
    }
    return rect;
}
