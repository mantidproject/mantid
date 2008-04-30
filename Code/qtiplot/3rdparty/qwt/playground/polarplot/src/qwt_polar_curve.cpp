/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#include <qpainter.h>
#include "qwt_global.h"
#include "qwt_scale_map.h"
#include "qwt_double_rect.h"
#include "qwt_math.h"
#include "qwt_polygon.h"
#include "qwt_symbol.h"
#include "qwt_legend.h"
#include "qwt_legend_item.h"
#include "qwt_polar_curve.h"

static int verifyRange(int size, int &i1, int &i2)
{
    if (size < 1)
        return 0;

    i1 = qwtLim(i1, 0, size-1);
    i2 = qwtLim(i2, 0, size-1);

    if ( i1 > i2 )
        qSwap(i1, i2);

    return (i2 - i1 + 1);
}

class QwtPolarCurve::PrivateData
{
public:
    PrivateData():
        style(QwtPolarCurve::Lines)
    {
        symbol = new QwtSymbol();
        pen = QPen(Qt::black);
    }

    ~PrivateData()
    {
        delete symbol;
    }

    QwtPolarCurve::CurveStyle style;
    QwtSymbol *symbol;
    QPen pen;
};

QwtPolarCurve::QwtPolarCurve():
    QwtPolarItem(QwtText())
{
    init();
}

QwtPolarCurve::QwtPolarCurve(const QwtText &title):
    QwtPolarItem(title)
{
    init();
}

QwtPolarCurve::QwtPolarCurve(const QString &title):
    QwtPolarItem(QwtText(title))
{
    init();
}

QwtPolarCurve::~QwtPolarCurve()
{
    delete d_points;
    delete d_data;
}

void QwtPolarCurve::init()
{
    setItemAttribute(QwtPolarItem::AutoScale);
    setItemAttribute(QwtPolarItem::Legend);

    d_data = new PrivateData;
    d_points = new QwtPolygonFData(QwtArray<QwtDoublePoint>());

    setZ(20.0);
#if QT_VERSION >= 0x040000
    setRenderHint(RenderAntialiased, true);
#endif
}

//! \return QwtPolarCurve::Rtti_PolarCurve
int QwtPolarCurve::rtti() const
{
    return QwtPolarItem::Rtti_PolarCurve;
}

void QwtPolarCurve::setStyle(CurveStyle style)
{
    if ( style != d_data->style )
    {
        d_data->style = style;
        itemChanged();
    }
}

QwtPolarCurve::CurveStyle QwtPolarCurve::style() const 
{ 
    return d_data->style; 
}

void QwtPolarCurve::setSymbol(const QwtSymbol &s )
{
    delete d_data->symbol;
    d_data->symbol = s.clone();
    itemChanged();
}

const QwtSymbol &QwtPolarCurve::symbol() const 
{ 
    return *d_data->symbol; 
}

void QwtPolarCurve::setPen(const QPen &p)
{
    if ( p != d_data->pen )
    {
        d_data->pen = p;
        itemChanged();
    }
}

const QPen& QwtPolarCurve::pen() const 
{ 
    return d_data->pen; 
}

void QwtPolarCurve::setData(const QwtData &data)
{
    delete d_points;
    d_points = data.copy();
    itemChanged();
}

void QwtPolarCurve::draw(QPainter *painter,
    const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
    const QwtDoublePoint &pole, double /*radius*/,
    const QwtDoubleRect &) const
{
    draw(painter, azimuthMap, radialMap, pole, 0, -1);
}

void QwtPolarCurve::draw(QPainter *painter,
    const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
    const QwtDoublePoint &pole, int from, int to) const
{
    if ( !painter || dataSize() <= 0 )
        return;

    if (to < 0)
        to = dataSize() - 1;

    if ( verifyRange(dataSize(), from, to) > 0 )
    {
        painter->save();
        painter->setPen(d_data->pen);

        drawCurve(painter, d_data->style, 
            azimuthMap, radialMap, pole, from, to);
        painter->restore();

        if (d_data->symbol->style() != QwtSymbol::NoSymbol)
        {
            painter->save();
            drawSymbols(painter, *d_data->symbol, 
                azimuthMap, radialMap, pole, from, to);
            painter->restore();
        }
    }
}

void QwtPolarCurve::drawCurve(QPainter *painter, int style, 
    const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
    const QwtDoublePoint &pole, int from, int to) const
{
    switch (style)
    {
        case Lines:
            drawLines(painter, azimuthMap, radialMap, pole, from, to);
            break;
        case NoCurve:
        default:
            break;
    }
}

void QwtPolarCurve::drawLines(QPainter *painter,
    const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
    const QwtDoublePoint &pole, int from, int to) const
{
    int size = to - from + 1;
    if ( size <= 0 )
        return;

    QwtPolygon polyline(size);
    for (int i = from; i <= to; i++)
    {
        double r = radialMap.xTransform(radius(i));
        const double a = azimuthMap.xTransform(azimuth(i));
        polyline.setPoint(i - from, qwtPolar2Pos(pole, r, a).toPoint() );
    }
    painter->drawPolyline(polyline);
}

void QwtPolarCurve::drawSymbols(QPainter *painter, const QwtSymbol &symbol, 
    const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
    const QwtDoublePoint &pole, int from, int to) const
{
    painter->setBrush(symbol.brush());
    painter->setPen(symbol.pen());

    QRect rect(QPoint(0, 0), symbol.size());

    for (int i = from; i <= to; i++)
    {
        const double r = radialMap.xTransform(radius(i));
        const double a = azimuthMap.xTransform(azimuth(i));

        const QPoint pos = qwtPolar2Pos(pole, r, a).toPoint();

        rect.moveCenter(pos);
        symbol.draw(painter, rect);
    }

}

int QwtPolarCurve::dataSize() const
{
    return d_points->size();
}

void QwtPolarCurve::updateLegend(QwtLegend *legend) const
{
    if ( !legend )
        return;
        
    QwtPolarItem::updateLegend(legend);
    
    QWidget *widget = legend->find(this);
    if ( !widget || !widget->inherits("QwtLegendItem") )
        return;
        
    QwtLegendItem *legendItem = (QwtLegendItem *)widget;
    
#if QT_VERSION < 0x040000
    const bool doUpdate = legendItem->isUpdatesEnabled();
#else
    const bool doUpdate = legendItem->updatesEnabled();
#endif
    legendItem->setUpdatesEnabled(false);
    
    const int policy = legend->displayPolicy();
    
    if (policy == QwtLegend::FixedIdentifier)
    {
        int mode = legend->identifierMode();

        if (mode & QwtLegendItem::ShowLine)
            legendItem->setCurvePen(pen());

        if (mode & QwtLegendItem::ShowSymbol)
            legendItem->setSymbol(symbol());

        if (mode & QwtLegendItem::ShowText)
            legendItem->setText(title());
        else
            legendItem->setText(QwtText());

        legendItem->setIdentifierMode(mode);
    }
    else if (policy == QwtLegend::AutoIdentifier)
    {
        int mode = 0;

        if (QwtPolarCurve::NoCurve != style())
        {
            legendItem->setCurvePen(pen());
            mode |= QwtLegendItem::ShowLine;
        }
        if (QwtSymbol::NoSymbol != symbol().style())
        {
            legendItem->setSymbol(symbol());
            mode |= QwtLegendItem::ShowSymbol;
        }
        if ( !title().isEmpty() )
        {
            legendItem->setText(title());
            mode |= QwtLegendItem::ShowText;
        }
        else
        {
            legendItem->setText(QwtText());
        }
        legendItem->setIdentifierMode(mode);
    }

    legendItem->setUpdatesEnabled(doUpdate);
    legendItem->update();
}
