/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

// vim: expandtab

#include <math.h>
#include "qwt_polar_plot.h"
#include "qwt_polar_canvas.h"
#include "qwt_scale_div.h"
#include "qwt_polar_magnifier.h"

QwtPolarMagnifier::QwtPolarMagnifier(QwtPolarCanvas *plot):
    QwtMagnifier(plot)
{
}

//! Destructor
QwtPolarMagnifier::~QwtPolarMagnifier()
{
}

QwtPolarCanvas *QwtPolarMagnifier::canvas()
{
    QWidget *w = parentWidget();
    if ( w && w->inherits("QwtPolarCanvas") )
        return (QwtPolarCanvas *)w;

    return NULL;
}

const QwtPolarCanvas *QwtPolarMagnifier::canvas() const
{
    return ((QwtPolarMagnifier *)this)->canvas();
}

QwtPolarPlot *QwtPolarMagnifier::plot()
{
    QwtPolarCanvas *c = canvas();
    if ( c )
        return c->plot();

    return NULL;
}

const QwtPolarPlot *QwtPolarMagnifier::plot() const
{
    return ((QwtPolarMagnifier *)this)->plot();
}

void QwtPolarMagnifier::rescale(double factor)
{
    if ( factor == 1.0 || factor == 0.0 )
        return;

    QwtPolarPlot* plt = plot();

    const bool autoReplot = plt->autoReplot();
    plt->setAutoReplot(false);

    plt->zoom(plt->zoomPos(), plt->zoomFactor() * factor);

    plt->setAutoReplot(autoReplot);
    plt->replot();
}
