/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

// vim: expandtab

#include "qwt_scale_div.h"
#include "qwt_polar_plot.h"
#include "qwt_polar_canvas.h"
#include "qwt_polar_panner.h"

class QwtPolarPanner::PrivateData
{
public:
    PrivateData()
    {
        for ( int scaleId = 0; scaleId < QwtPolar::ScaleCount; scaleId++ )
            isScaleEnabled[scaleId] = true;
    }

    bool isScaleEnabled[QwtPolar::ScaleCount];
};

QwtPolarPanner::QwtPolarPanner(QwtPolarCanvas *plot):
    QwtPanner(plot)
{
    d_data = new PrivateData();

    connect(this, SIGNAL(panned(int, int)),
        SLOT(movePlot(int, int)));
}

//! Destructor
QwtPolarPanner::~QwtPolarPanner()
{
    delete d_data;
}

void QwtPolarPanner::setScaleEnabled(int scaleId, bool on)
{
    if ( scaleId >= 0 && scaleId < QwtPolar::ScaleCount)
        d_data->isScaleEnabled[scaleId] = on;
}

bool QwtPolarPanner::isScaleEnabled(int scaleId) const
{
    if ( scaleId >= 0 && scaleId < QwtPolar::ScaleCount )
        return d_data->isScaleEnabled[scaleId];

    return true;
}

QwtPolarCanvas *QwtPolarPanner::canvas()
{   
    QWidget *w = parentWidget();
    if ( w && w->inherits("QwtPolarCanvas") )
        return (QwtPolarCanvas *)w;

    return NULL;
}

const QwtPolarCanvas *QwtPolarPanner::canvas() const
{   
    return ((QwtPolarPanner *)this)->canvas();
}

QwtPolarPlot *QwtPolarPanner::plot()
{
    QwtPolarCanvas *c = canvas();
    if ( c )
        return c->plot();

    return NULL;
}

const QwtPolarPlot *QwtPolarPanner::plot() const
{
    return ((QwtPolarPanner *)this)->plot();
}

void QwtPolarPanner::movePlot(int dx, int dy)
{
    QwtPolarPlot *plot = QwtPolarPanner::plot();
    if ( plot == NULL || ( dx == 0 && dy == 0 ) )
        return;

    const QwtScaleMap map = plot->scaleMap(QwtPolar::Radius);

    QwtPolarPoint pos = plot->zoomPos();
    if ( map.s1() <= map.s2() )
    {
        pos.setRadius(
            map.xTransform(map.s1() + pos.radius()) - map.p1());
        pos.setPoint(pos.toPoint() - QwtDoublePoint(dx, -dy));
        pos.setRadius(
            map.invTransform(map.p1() + pos.radius()) - map.s1());
    }
    else
    {
        pos.setRadius(
            map.xTransform(map.s1() - pos.radius()) - map.p1());
        pos.setPoint(pos.toPoint() - QwtDoublePoint(dx, -dy));
        pos.setRadius(
            map.s1() - map.invTransform(map.p1() + pos.radius()));
    }
    
    const bool doAutoReplot = plot->autoReplot();
    plot->setAutoReplot(false);

    plot->zoom(pos, plot->zoomFactor());

    plot->setAutoReplot(doAutoReplot);
    plot->replot();
}
